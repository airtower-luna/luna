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

# columns in RTT tabular output
cols.size = 1; # packet size for the measurement set
cols.ist = 2; # scheduled IST for the measurement set
cols.rtt = 3; # average RTT in the measurement set
cols.std = 4; # RTT standard deviation
cols.packets = 5; # number of packets in the measurement set

upper = str2num(parser.Results.upper);

# this cell array collects RTTs for all files so they can
# be compared
rtt = {};
size = {};
std = {};
ist = {};

for i = 1:length(files);
  filename = files{i};
  # read from file
  A = dlmread(filename, "\t", 1, 0);
  rtt{i} = A( :, cols.rtt);
  size{i} = A( :, cols.size);
  std{i} = A( :, cols.std);
  ist{i} = A( :, cols.ist);
endfor

# basic figure setup
clf;
hold on;
xlabel("Packet size [byte]");
ylabel("RTT [$\\mu s$]");

for i = 1:length(rtt);
  p = errorbar(size{i}, rtt{i}, std{i}, ".-", "o");
  set(p, "color", colors{i});
endfor

# use tight axis scaling for x-axis only
a1 = axis();
axis("tight");
a2 = axis();
axis([a2(1) a2(2) a1(3) a1(4)]);

hold off;

output_format = parser.Results.format;
print_format(strcat(parser.Results.out, ".", output_format), output_format);
