#!/usr/bin/octave -qf

# This file is part of the Lightweight Universal Network Analyzer (LUNA)
#
# Copyright (c) 2013 Fiona Klute
#
# LUNA is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LUNA is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
# License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LUNA. If not, see <http://www.gnu.org/licenses/>.

source("luna-eval.m");



[opts, files] = split_command_line();

# create parser with default options
parser = luna_default_parser();
# list of speed values from IPerf, assumed to be kbit/s in 0.5s intervals
parser = parser.addParamValue("iperf", "", @ischar);
# step size for data rate averages (in µs)
parser = parser.addParamValue("step", "0", @isdigit);
# parse command line options
parser = parser.parse(opts{:});

if length(files) < 1
  printf("Usage: %s [[OPTIONS] --] FILENAME [MORE FILES]", program_name());
  exit(1);
endif

output_format = parser.Results.format;

ktimes = {};
dur = [];
sizes = {};

for i = 1:length(files);
  filename = files{i}
  printf("Reading data from %s: ", filename);
  # read test output
  data = parse_server_log(parser.Results.kutime, filename);
  ktimes{i} = data.ktime;
  sizes{i} = data.size;
  ktimes{i} = ktimes{i} .- ktimes{i}(1);
  dur(i) = ktimes{i}(end);
endfor

# Try to read the specified step size, if any
step = str2num(parser.Results.step);
points = [];
# No fixed value was given, split evenly.
if (step == 0)
  points = linspace(0, max(dur), 300);
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
  set(h{i}, "color", luna_colors(i));
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
  # just want to compare one LUNA measurement with Iperf.
  set(ih, "color", luna_colors(2));
  hold off;
endif

print_format(strcat(parser.Results.out, ".", output_format),
	     output_format);
