#!/usr/bin/octave -qf

source("luna-eval.m");



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
# if activated, create a 3D plot (without error bars)
parser = parser.addSwitch("three");
# activate "support" structure in 3D plot
parser = parser.addSwitch("support");
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
  # rtt.m prints an additional empty line for some reason, so skip two lines
  A = dlmread(filename, "\t", 2, 0);
  rtt{i} = A( :, cols.rtt);
  size{i} = A( :, cols.size);
  std{i} = A( :, cols.std);
  ist{i} = A( :, cols.ist);
endfor

# basic figure setup
clf;
hold on;
xlabel("Packet size [byte]");
if parser.Results.three
  ylabel("IST [$\\mu s$]");
  zlabel("RTT [$\\mu s$]");
else
  ylabel("RTT [$\\mu s$]");
endif

if parser.Results.support
  grid("on");
endif

for i = 1:length(rtt);
  if parser.Results.three
    plot3(size{i}, ist{i}, rtt{i}, ".-", "color", colors{i});
    if parser.Results.support
      for j = 1:length(rtt{i});
	plot3([size{i}(j) size{i}(j)], [ist{i}(j) ist{i}(j)], [0 rtt{i}(j)],
	      "-", "color", colors{i});
      endfor
      plot3(size{i}, ist{i}, linspace(0, 0, length(size{i})),
	    "-", "color", colors{i});
    endif
  else
    p = errorbar(size{i}, rtt{i}, std{i}, ".-", "o");
    set(p, "color", colors{i});
  endif
endfor

# use tight axis scaling for x-axis in 2D plots only
if !parser.Results.three
  a1 = axis();
  axis("tight");
  a2 = axis();
  axis([0 a2(2) a1(3) a1(4)]);
endif

hold off;

output_format = parser.Results.format;
print_format(strcat(parser.Results.out, ".", output_format), output_format);
