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
# Write tabular output for all input files. Assumes that all packets in
# one file have the same size.
parser = parser.addSwitch("tab");
# provide IST for tabular output
parser = parser.addParamValue("ist", "-1", @isdigit);
# parse command line options
parser = parser.parse(opts{:});

if length(files) < 1
  printf("Usage: %s [[OPTIONS] --] FILENAME [MORE FILES]", program_name());
  exit(1);
endif

cols = echo_column_definitions();
upper = str2num(parser.Results.upper);
ist = str2num(parser.Results.ist);

# this cell array collects RTTs for all files so they can
# be compared
rtt = {};
size = {};

for i = 1:length(files);
  filename = files{i};
  if (!parser.Results.tab)
    printf("Reading data from %s: ", filename);
  endif
  # read test output
  data = parse_log(cols, filename);
  if (!parser.Results.tab)
    printf("%i data sets\n", length(data.sequence));
  endif
  rtt{i} = data.rtt;
  size{i} = data.size;

  if (!parser.Results.tab)
    chk_seq(data.sequence);
  endif
endfor

for i = 1:length(rtt)
  [u{i}, l{i}, m{i}, s{i}] = basic_metrics(rtt{i});
endfor

# printing a summary doesn't make sense for more than one data set
if (length(rtt) == 1 && !parser.Results.tab)
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

# tabular output, if requested
if (parser.Results.tab)
  printf("# Size\tIST\tavg(RTT)\tstd(RTT)\tPackets");
  for i = 1:length(rtt)
    printf("\n%ld\t%ld\t%ld\t%ld\t%ld",
	   size{i}(1), ist, mean(rtt{i}), s{i}, length(rtt{i}));
  endfor
  exit(0);
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
