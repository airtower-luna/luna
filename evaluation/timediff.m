#!/usr/bin/octave -qf

source("fast-tg-eval.m");



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
# upper_limit is the largest permitted X value for the histogram, <= 0
# means "no limit"
function eval_iat(filename, output_format, upper_limit, varargin)
  for i = 1:length(varargin)
    iats{i} = diff(varargin{i});
    [u{i}, l{i}, m{i}, s{i}] = basic_metrics(iats{i});

    # lower plot limit (median - 2 * standard deviation)
    ll(i) = max(l{i}, (m{i} - 2 * s{i}));
    # upper plot limit (median + 2 * standard deviation)
    ul(i) = min(u{i}, (m{i} + 2 * s{i}));
  endfor

  # printing a summary doesn't make sense for more than one data set
  if (length(varargin) == 1)
    printf("\nEvaluation of inter arrival times\n");
    printf("Upper limit: %ld µs\n", u{1});
    printf("Lower limit: %ld µs\n", l{1});
    printf("Average: %ld µs\n", mean(iats{1}));
    printf("Median: %ld µs\n", m{1});
    printf("Standard deviation: %ld µs\n", s{1});
  else
    metrics = [0, 0, 0, 0];
    for i = 1:length(iats)
      metrics(i, :) = [u{i}, l{i}, m{i}, s{i}];
    endfor
  endif

  global max_hist_bins;
  # bin width is at least one, otherwise range is split evenly in
  # max_hist_bins bins
  range_lower = max([min(ll) 0]);
  range_upper = max(ul);
  if (exist("upper_limit", "var") && upper_limit > 0)
    range_upper = min([range_upper upper_limit]);
  endif
  binwidth = max(1, (range_upper - range_lower) / max_hist_bins);
  range = [range_lower:binwidth:range_upper];

  # use global color list for combined diagrams
  global colors;

  # basic figure setup
  clf;
  hold on;
  axis([(range(1) - binwidth / 2) (range(end) + binwidth / 2)], "autoy");
  set(gca, "yscale", "log");
  title("Distribution of inter arrival times");
  xlabel("IAT [$\\mu s$]");
  ylabel("Frequency");

  # plot the histogram(s)
  for i = 1:length(iats)
    [yh xh] = hist(iats{i}, range, 1);
    [ys xs] = stairs(yh, xh);
    xs = [xs(1) - binwidth; xs(1) - binwidth; xs; xs(end)];
    xs = xs .+ (binwidth / 2);
    ys = [0; ys(1); ys; 0];
    h{i} = fill(xs, ys, colors{i});
    set(h{i}, "edgecolor", colors{i}, "facecolor", colors{i}, "facealpha", 0.5);
  endfor

  # figure complete
  hold off;

  print_format(strcat(filename, "-iat.", output_format), output_format);
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
parser = ftg_default_parser();
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

  chk_seq(seqnos);
  # do individual evaluation only when processing a single file
  if (length(files) == 1)
    if parser.Results.kutime
      eval_kutime(ktime, utime, filename, output_format);
    endif
    eval_iat(filename, output_format, upper, ktime);
  endif
endfor

if (length(times) > 1)
  eval_iat(parser.Results.out, output_format, upper, times{:});
endif
