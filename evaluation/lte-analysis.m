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
  A = dlmread(filename, "\t", 1, 0);
  A = sortrows(A, cols.size);
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
    plot3(size{i}, ist{i}, rtt{i}, ".-", "color", luna_colors(i));
    if parser.Results.support
      for j = 1:length(rtt{i});
	plot3([size{i}(j) size{i}(j)], [ist{i}(j) ist{i}(j)], [0 rtt{i}(j)],
	      "-", "color", luna_colors(i));
      endfor
      plot3(size{i}, ist{i}, linspace(0, 0, length(size{i})),
	    "-", "color", luna_colors(i));
    endif
  else
    p = errorbar(size{i}, rtt{i}, std{i}, ".-", "o");
    set(p, "color", luna_colors(i));
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
