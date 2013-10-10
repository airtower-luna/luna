#!/usr/bin/octave -qf

# load common variables and function definitions for LUNA evaluation
source("luna-eval.m");



# function to plot distribution of packet sizes (UDP payload)
function eval_size(sizes, filename, output_format)
  [u, l, m, s] = basic_metrics(sizes);
  printf("\nEvaluation of packet sizes\n");
  printf("Upper limit: %ld byte\n", u);
  printf("Lower limit: %ld byte\n", l);
  printf("Average: %ld byte\n", mean(sizes));
  printf("Median: %ld byte\n", m);
  printf("Standard deviation: %ld byte\n", s);

  # bin width is at least one, otherwise range is split evenly in
  # max_hist_bins bins
  binwidth = max(1, (u - l) / max_hist_bins());
  # non-integer binwidths lead to weird plots, because packet sizes are integers
  binwidth = round(binwidth);
  # max and min sizes are the plot limits
  range = [l:binwidth:u];
  transparent_hist(sizes, range, binwidth);
  axis([min((range(1) - binwidth / 2), 0) (range(end) + binwidth / 2)]);
  title("Distribution of packet sizes [byte]");

  print_format(strcat(filename, "-size.", output_format), output_format);
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

# create parser with default options
parser = luna_default_parser();
# parse command line options
parser = parser.parse(opts{:});

if length(files) < 1
  printf("Usage: %s [[OPTIONS] --] FILENAME [MORE FILES]", program_name());
  exit(1);
endif

# load column meanings depending on kutime flag
cols = server_column_definitions(parser.Results.kutime);

output_format = parser.Results.format;
upper = str2num(parser.Results.upper);

# this cell array collects kernel arrival times for all files so they can
# be compared
times = {};
sizes = {};

for i = 1:length(files);
  filename = files{i}
  printf("Reading data from %s: ", filename);
  # read test output
  A = dlmread(filename, "\t", 1, 0);
  printf("%i data sets\n", length(A));
  ktime = A( :, cols.ktime);
  times{i} = ktime;
  # read utime column only if requested (otherwise, it might not exist)
  if isfield(cols, "utime")
    utime = A( :, cols.utime);
  endif
  seqnos = A( :, cols.sequence);
  sizes{i} = A( :, cols.size);

  chk_seq(seqnos);
  # do individual evaluation only when processing a single file
  if (length(files) == 1)
    eval_size(sizes{i}, filename, output_format);
  endif
endfor

#if (length(times) > 1)
#  eval_iat(parser.Results.out, output_format, upper, times{:});
#endif
