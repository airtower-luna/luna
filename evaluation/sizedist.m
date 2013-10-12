#!/usr/bin/octave -qf

# load common variables and function definitions for LUNA evaluation
source("luna-eval.m");



# function to plot distribution of packet sizes (UDP payload)
function plot_size_dist(varargin)
  for i = 1:length(varargin)
    [u(i), l(i), m(i), s(i)] = basic_metrics(varargin{i});
    printf("\nEvaluation of packet sizes\n");
    printf("Upper limit: %ld byte\n", u(i));
    printf("Lower limit: %ld byte\n", l(i));
    printf("Average: %ld byte\n", mean(varargin{i}));
    printf("Median: %ld byte\n", m(i));
    printf("Standard deviation: %ld byte\n", s(i));
  endfor

  # bin width is at least one, otherwise range is split evenly in
  # max_hist_bins bins
  binwidth = max(1, (max(u) - min(l)) / max_hist_bins());
  # non-integer binwidths lead to weird plots, because packet sizes are integers
  binwidth = round(binwidth);
  # max and min sizes are the plot limits
  range = [min(l):binwidth:max(u)];

  hold on;
  for i = 1:length(varargin)
    h{i} = transparent_hist(varargin{i}, range, binwidth, i);
  endfor
  hold off;

  axis([min((range(1) - binwidth / 2), 0) (range(end) + binwidth / 2)]);
  title("Distribution of packet sizes [byte]");
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

upper = str2num(parser.Results.upper);

# this cell array collects packet sizes for all files
sizes = {};

for i = 1:length(files);
  filename = files{i}
  printf("Reading data from %s: ", filename);
  # read test output
  data = parse_server_log(parser.Results.kutime, filename);
  sizes{i} = data.size;

  chk_seq(data.sequence);
endfor

plot_size_dist(sizes{:});

print_format(strcat(parser.Results.out, "-size.", parser.Results.format),
	     parser.Results.format);
