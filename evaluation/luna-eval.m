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

1;

# inputParser is used for command line options
pkg load general;

# maximum number of bins to use when plotting with the hist function
function m = max_hist_bins()
  m = 200;
  return;
endfunction

# graphics configuration
graphics_toolkit("fltk");
# LUNA default plot colors
function c = luna_colors(a)
  colors = {"blue", "red", "cyan", "green", "magenta", "black", "yellow"};
  c = colors{a};
  return;
endfunction



function [max, min, med, std] = basic_metrics(a)
  max = max(a);
  min = min(a);
  med = median(a);
  std = std(a);
  return;
endfunction



function chk_seq(seq)
  len = length(seq);
  m = max(seq);
  printf("%i data sets present, maximum sequence number is %i", len, m);
  lost = (m + 1) - len;
  if (lost == 0)
    printf(", no packets lost.\n");
  else
    if (lost == 1)
      printf(", %i packet lost.\n", lost);
    else
      printf(", %i packets lost.\n", lost);
    endif
  endif

  maxseq = seq(1);
  for i = 2:(len-1)
    if seq(i) > maxseq
      maxseq = seq(i);
    else
      if seq(i) < maxseq
	printf("Reordering occurred: Packet %i arrived after Packet %i\n",
	       seq(i), maxseq);
      else
	printf("Error: Sequence number %i was detected more than once!\n",
	       seq(i));
      endif
    endif
  endfor
endfunction



function print_format(filename, output_format)
  if (exist("filename", "var") && exist("output_format", "var")
      && ischar(filename) && ischar(output_format))
    print(filename, strcat("-d", output_format));
  endif
endfunction



# read command line arguments and split them into options (all arguments
# up to "--", if any) and filenames (all remaining arguments)
function [opts, files] = split_command_line()
  args = argv();

  # Search for "--" in the command line arguments, if found, split between
  # options and files at that point. Otherwise, all arguments are file names.
  files = args;
  opts = {};
  for i = 1:length(args)
    if strcmp(args(i), "--")
      opts = args(1:i-1);
      files = args(i+1:end);
      break;
    endif
  endfor
  return;
endfunction



# create a parser for the default command line options
function parser = luna_default_parser()
  # parse options
  parser = inputParser;
  parser.CaseSensitive = true;
  # output format
  parser = parser.addParamValue("format", "png", @ischar);
  # output file name (if applicable)
  parser = parser.addParamValue("out", "out", @ischar);
  # upper limit for the plot (x axis)
  parser = parser.addParamValue("upper", "-1", @isdigit);
  # set this flag if the input file(s) contain(s) user space arrival times
  parser = parser.addSwitch("kutime");
  return;
endfunction



# Column numbers by meaning in LUNA server tab separated output
# If the parameter is missing, false is assumed.
function cols = server_column_definitions(kutime)
  cols.ktime = 1;
# if there is a user space time column, the following columns shift
  if (exist("kutime", "var") && kutime)
    cols.utime = 2;
    cols.source = 3;
    cols.port = 4;
    cols.sequence = 5;
    cols.size = 6;
  else
    cols.source = 2;
    cols.port = 3;
    cols.sequence = 4;
    cols.size = 5;
  endif
  return;
endfunction



# Column numbers by meaning in the LUNA client's echo logs
function cols = echo_column_definitions()
  cols.ktime = 1;
  cols.sequence = 2;
  cols.size = 3;
  cols.rtt = 4;
  return;
endfunction




# kutime parameter
# varargin: One or more filenames to read. For one filename, parse_server_log
# returns a structure, otherwise a cell array of structures.
function data = parse_server_log(kutime, varargin)
  cols = server_column_definitions(kutime);
  data = parse_log(cols, varargin{:});
  return;
endfunction



# cols: column definitions
# varargin: One or more filenames to read. For one filename, parse_server_log
# returns a structure, otherwise a cell array of structures.
function data = parse_log(cols, varargin)
  d = {};

  for i = 1:length(varargin);
    A = dlmread(varargin{i}, "\t", 1, 0);

    d{i} = struct();
    for [val, key] = cols;
      d{i} = setfield(d{i}, key, A( :, val));
    endfor
  endfor

  if (length(d) > 1)
    data = d;
  else
    data = d{1};
  endif
  return;
endfunction



# data: data array to plot
# range: histogram range, format: [lower_limit:binwidth:upper_limit]
#	Data outside the limit will be lumped into left-/rightmost bin
# binwidth: binwidth for the histogram
# colorindex (optional): index color to use from LUNA's default selection
function h = transparent_hist(data, range, binwidth, colorindex)
  if (!exist("colorindex", "var"))
    colorindex = 1;
  endif

  # calculate and plot
  [yh xh] = hist(data, range, 1);
  [ys xs] = stairs(yh, xh);
  # create top lines of left-/rightmost columns
  xs = [xs(1) - binwidth; xs(1) - binwidth; xs; xs(end)];
  # histogram gets slightly shifted when getting drawn as stair plot, fix that
  xs = xs .+ (binwidth / 2);
  # create outer socket point of left-/rightmost columns
  ys = [0; ys(1); ys; 0];
  # draw the plot
  h = fill(xs, ys, luna_colors(colorindex));
  set(h, "edgecolor", luna_colors(colorindex),
      "facecolor", luna_colors(colorindex),
      "facealpha", 0.5);
  return;
endfunction



# Calculate upper and lower limits for a plot, based on "factor"
# standard deviations around the median. If more than one data set
# is given, the lowest lower limit and highest upper limit are used.
#
# factor: How many times the standard deviation around the median
#	should define the limits?
# varargin: Any number of data arrays (at least one)
function [lower, upper] = std_plot_range(factor, varargin)
  for i = 1:length(varargin)
    [u{i}, l{i}, m{i}, s{i}] = basic_metrics(varargin{i});

    # lower plot limit (median - factor * standard deviation)
    ll(i) = max(l{i}, (m{i} - factor * s{i}));
    # upper plot limit (median + factor * standard deviation)
    ul(i) = min(u{i}, (m{i} + factor * s{i}));
  endfor

  lower = min(ll);
  upper = max(ul);
  return;
endfunction



# bin width is at least one, otherwise range is split evenly in
# "bins" bins
# lower: lower limit
# upper: upper limit
# bins (optional): maximum number of bins
function [range, binwidth] = hist_range(lower, upper, bins)
  if (!exist("bins", "var"))
    bins = max_hist_bins();
  endif

  # binwidth should not be below one
  binwidth = max(1, (upper - lower) / bins);
  binwidth = round(binwidth);
  range = [lower:binwidth:upper];
  return;
endfunction



# Histogram plot function
#
# Can draw multiple datasets as transparent histograms, enabling easy
# comparisons
#
# factor: How many times the standard deviation around the median
#	should define the limits?
# datasets: datasets to plot (array)
# min: lowest permitted lower plot limit
# max: highest permitted upper plot limit
function h = datasets_hist_plot(factor, datasets, min, max)
  [ll, ul] = std_plot_range(factor, datasets{:});
  # apply give minimum
  if (exist("min", "var"))
    range_lower = max([ll min]);
  else
    range_lower = ll;
  endif
  # apply give maximum
  if (exist("max", "var"))
    range_upper = min([ul max]);
  else
    range_upper = ul;
  endif

  [range, binwidth] = hist_range(range_lower, range_upper);

  # start plotting
  hold on;
  # set figure range
  axis([(range(1) - binwidth / 2) (range(end) + binwidth / 2)], "autoy");

  # plot the histogram(s)
  for i = 1:length(datasets)
    h{i} = transparent_hist(datasets{i}, range, binwidth, i);
  endfor

  # figure complete
  hold off;
endfunction
