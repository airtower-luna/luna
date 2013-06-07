#!/usr/bin/octave -qf

# inputParser is used for command line options
pkg load general;

# maximum number of bins to use when plotting with the hist function
global max_hist_bins = 200;



function print_format(filename, output_format)
  if (exist("filename", "var") && exist("output_format", "var")
      && ischar(filename) && ischar(output_format))
    print(filename, strcat("-d", output_format));
  endif
endfunction



function [max, min, med, std] = basic_metrics(a)
  max = max(a);
  min = min(a);
  med = median(a);
  std = std(a);
  return;
endfunction



# function to calculate and plot the differences between kernel and user
# space arrival times
function eval_kutime(ktime, utime, filename, output_format)
  timediff = utime .- ktime;
  [u, l, m, s] = basic_metrics(timediff);
  printf("\nEvaluation of differences between kernel and user space arrival times\n");
  printf("Upper limit: %ld µs\n", u);
  printf("Lower limit: %ld µs\n", l);
  printf("Average: %ld µs\n", mean(timediff));
  printf("Median: %ld µs\n", m);
  printf("Standard deviation: %ld µs\n", s);

  global max_hist_bins;
  # upper plot limit (median + 2 * standard deviation)
  ul = (m + 2 * s);
  # bin width is at least one, otherwise range is split evenly in
  # max_hist_bins bins
  binwidth = max(1, (ul - l) / max_hist_bins);
  range = [l:binwidth:ul];
  hist(timediff, range, 1);
  axis([min((range(1) - binwidth / 2), 0) (range(end) + binwidth / 2)]);
  title("Distribution of difference between kernel and user space arrival times [us]");

  print_format(strcat(filename, "-kutime.", output_format), output_format);
endfunction



# calculate inter arrival times
function eval_iat(filename, output_format, varargin)
  for i = 1:length(varargin)
    iats{i} = diff(varargin{i});
    [u{i}, l{i}, m{i}, s{i}] = basic_metrics(iats{i});

    # lower plot limit (median - 2 * standard deviation)
    ll{i} = max(l{i}, (m{i} - 2 * s{i}));
    # upper plot limit (median + 2 * standard deviation)
    ul{i} = min(u{i}, (m{i} + 2 * s{i}));
  endfor

  # printing a summary doesn't make sense for more than one data set
  if (length(varargin) == 1)
    printf("\nEvaluation of inter arrival times\n");
    printf("Upper limit: %ld µs\n", u{1});
    printf("Lower limit: %ld µs\n", l{1});
    printf("Average: %ld µs\n", mean(iats{1}));
    printf("Median: %ld µs\n", m{1});
    printf("Standard deviation: %ld µs\n", s{1});
  endif

  global max_hist_bins;
  # bin width is at least one, otherwise range is split evenly in
  # max_hist_bins bins
  binwidth = max(1, (max(ul{:}) - min(ll{:})) / max_hist_bins);
  range = [min(ll{:}):binwidth:max(ul{:})];
  hist(iats{1}, range, 1);

  [minm, mini] = min([m{:}]);
  if (minm < s{mini})
    axis([0 (range(end) + binwidth / 2)]);
  else
    axis([(range(1) - binwidth / 2) (range(end) + binwidth / 2)]);
  endif

  title("Distribution of inter arrival times [us]");

  print_format(strcat(filename, "-iat.", output_format), output_format);
endfunction



function chk_seq(seq)
  len = length(seq);
  m = max(seq);
  printf("%i data sets present, maximum sequence number is %i", len, m);
  lost = (m + 1) - len;
  if (lost == 0)
    printf(", no packets lost.\n");
  else
    if (lost == 1)
      printf(", %i packet lost.\n", lost);
    else
      printf(", %i packets lost.\n", lost);
    endif
  endif

  maxseq = seq(1);
  for i = 2:(len-1)
    if seq(i) > maxseq
      maxseq = seq(i);
    else
      if seq(i) < maxseq
	printf("Reordering occurred: Packet %i arrived after Packet %i\n",
	       seq(i), maxseq);
      else
	printf("Error: Sequence number %i was detected more than once!\n",
	       seq(i));
      endif
    endif
  endfor
endfunction



# read arguments and get the input file's name
arg_list = argv();

# Search for "--" in the command line arguments, if found, split between
# options and files at that point. Otherwise, all arguments are file names.
files = arg_list;
opts = {};
for i = 1:nargin()
  if strcmp(arg_list(i), "--")
    opts = arg_list(1:i-1);
    files = arg_list(i+1:nargin());
    break;
  endif
endfor

# parse options
parser = inputParser;
parser.CaseSensitive = true;
parser = parser.addParamValue("format", "jpg", @ischar);
parser = parser.addSwitch("kutime");
parser = parser.parse(opts{:});

if length(files) < 1
  printf("Usage: %s [[OPTIONS] --] FILENAME [MORE FILES]", program_name());
  exit(1);
endif

# variables for column meanings
ktime_col = 1;
# if there is no user space time column, the following columns shift
if parser.Results.kutime
  utime_col = 2;
  source_col = 3;
  port_col = 4;
  sequence_col = 5;
  size_col = 6;
else
  source_col = 2;
  port_col = 3;
  sequence_col = 4;
  size_col = 5;
endif

output_format = parser.Results.format;

# this cell array collects kernel arrival times for all files so they can
# be compared
times = {};

for i = 1:length(files);
  filename = files{i}
  printf("Reading data from %s: ", filename);
  # read test output
  A = dlmread(filename, "\t", 1, 0);
  printf("%i data sets\n", length(A));
  ktime = A( :, ktime_col);
  times{i} = ktime;
  # read utime column only if requested (otherwise, it might not exist)
  if exist("utime_col", "var")
    utime = A( :, utime_col);
  endif
  seqnos = A( :, sequence_col);

  chk_seq(seqnos);
  if parser.Results.kutime
    eval_kutime(ktime, utime, filename, output_format);
  endif
  eval_iat(filename, output_format, ktime);
endfor
