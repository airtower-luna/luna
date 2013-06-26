#!/usr/bin/octave -qf

# inputParser is used for command line options
pkg load general;

# maximum number of bins to use when plotting with the hist function
global max_hist_bins = 200;

# graphics configuration
graphics_toolkit("fltk");
global colors = {"blue", "red", "green", "magenta", "black", "cyan", "yellow"};


function print_format(filename, output_format)
  if (exist("filename", "var") && exist("output_format", "var")
      && ischar(filename) && ischar(output_format))
    print(filename, strcat("-d", output_format));
  endif
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
# output format
parser = parser.addParamValue("format", "png", @ischar);
# output file name for comparison (if applicable)
parser = parser.addParamValue("compare_out", "compare", @ischar);
# set this flag if the input file(s) contain(s) user space arrival times
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

ktimes = {};
dur = [];
sizes = {};

for i = 1:length(files);
  filename = files{i}
  printf("Reading data from %s: ", filename);
  # read test output
  A = dlmread(filename, "\t", 1, 0);
  printf("%i data sets\n", length(A));
  ktimes{i} = A( :, ktime_col);
  sizes{i} = A( :, size_col);
  ktimes{i} = ktimes{i} .- ktimes{i}(1);
  dur(i) = ktimes{i}(end);
endfor

# TODO: configurable step size
#step = 1000;
step = max(dur) / 200;
halfstep = step / 2;
points = 0:step:max(dur);
rates = {};
for i = 1:length(files);
  idx = 1;
  rates{i} = [];
  for j = 1:length(points)
    t = points(j);
    rates{i}(j) = 0;
    while idx <= length(ktimes{i}) && ktimes{i}(idx) < (t + halfstep)
      rates{i}(j) = rates{i}(j) + sizes{i}(idx);
      idx = idx + 1;
    endwhile
  endfor
  # byte to bit
  rates{i} = rates{i} .* 8;
  # bit per step to bit per second
  rates{i} = rates{i} ./ step .* 1000000;
endfor

points = points ./ 1000000;
clf;
hold on;
xlabel("Time [s]");
ylabel("Data rate [bit/s]");
h = {};
for i = 1:length(rates);
  h{i} = plot(points, rates{i});
  set(h{i}, "color", colors{i});
endfor
hold off;
print_format(strcat("datarate-", num2str(step, "%d"), ".", output_format),
	     output_format);
