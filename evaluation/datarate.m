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
# output file name for the plot
parser = parser.addParamValue("out", "out", @ischar);
# list of speed values from IPerf, assumed to be kbit/s in 0.5s intervals
parser = parser.addParamValue("iperf", "", @ischar);
# step size for data rate averages (in µs)
parser = parser.addParamValue("step", "0", @isdigit);
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

# Try to read the specified step size, if any
step = str2num(parser.Results.step);
points = [];
# No fixed value was given, split evenly.
if (step == 0)
  points = linspace(0, max(dur), 200);
  step = points(2);
else
  points = 0:step:max(dur);
endif
halfstep = step / 2;
printf("Plotting with step size of %i µs.\n", step);

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
axis([0 ceil(max(dur) ./ 1000000)], "autoy");
h = {};
for i = 1:length(rates);
  h{i} = plot(points, rates{i});
  set(h{i}, "color", colors{i});
endfor
hold off;

# If IPerf data was given, plot it for comparison. The input data is
# expected to be one number per line, in kbit/s in 0.5s intervals
if (!strcmp(parser.Results.iperf, ""))
  hold on;
  printf("Reading IPerf data from %s.", parser.Results.iperf);
  # read test output
  A = dlmread(parser.Results.iperf, "\t", 0, 0);
  # IPerf data rates, assumed to be kbit/s
  iy = A(:, 1);
  iy = iy .* 1000;
  # Create X values centered in 0.5s intervals
  ix(1) = 0.25;
  for i = 2:length(iy);
    ix(i) = ix(i-1) + 0.5;
  endfor
  ih = plot(ix, iy);
  # I admit that setting a fixed color is a dirty hack, but usually I
  # just want to compare one fast-tg measurement with iperf.
  set(ih, "color", colors{2});
  hold off;
endif

print_format(strcat(parser.Results.out, ".", output_format),
	     output_format);
