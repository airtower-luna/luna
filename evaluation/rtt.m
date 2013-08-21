#!/usr/bin/octave -qf

source("fast-tg-eval.m");



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

cols = echo_column_definitions();
upper = str2num(parser.Results.upper);

# this cell array collects RTTs for all files so they can
# be compared
rtt = {};

for i = 1:length(files);
  filename = files{i}
  printf("Reading data from %s: ", filename);
  # read test output
  A = dlmread(filename, "\t", 1, 0);
  printf("%i data sets\n", length(A));
  ktime = A( :, cols.rtt);
  rtt{i} = ktime;
  seqnos = A( :, cols.sequence);

  chk_seq(seqnos);
  # do individual evaluation only when processing a single file
endfor

for i = 1:length(rtt)
  [u{i}, l{i}, m{i}, s{i}] = basic_metrics(rtt{i});
endfor

# printing a summary doesn't make sense for more than one data set
if (length(rtt) == 1)
  printf("\nEvaluation of inter arrival times\n");
  printf("Upper limit: %ld µs\n", u{1});
  printf("Lower limit: %ld µs\n", l{1});
  printf("Average: %ld µs\n", mean(rtt{1}));
  printf("Median: %ld µs\n", m{1});
  printf("Standard deviation: %ld µs\n", s{1});
else
  metrics = [0, 0, 0, 0];
  for i = 1:length(rtt)
    metrics(i, :) = [u{i}, l{i}, m{i}, s{i}];
  endfor
endif

# basic figure setup
clf;
hold on;
set(gca, "yscale", "log");
title("Distribution of round trip times");
xlabel("RTT [$\\mu s$]");
ylabel("Frequency");
hold off;

if (upper > 0)
  datasets_hist_plot(2, rtt, 0, upper);
else
  datasets_hist_plot(2, rtt, 0);
endif

output_format = parser.Results.format;
print_format(strcat(parser.Results.out, ".", output_format), output_format);