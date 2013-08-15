#!/usr/bin/octave -qf
1;

# inputParser is used for command line options
pkg load general;

# maximum number of bins to use when plotting with the hist function
global max_hist_bins = 200;

# graphics configuration
graphics_toolkit("fltk");
global colors = {"blue", "red", "cyan", "green", "magenta", "black", "yellow"};



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



# create a parser for the default command line options
function parser = ftg_default_parser()
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



# Column numbers by meaning in fast-tg server tab separated output
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



# Column numbers by meaning in the fast-tg client's echo logs
function cols = echo_column_definitions()
  cols.ktime = 1;
  cols.sequence = 2;
  cols.size = 3;
  cols.rtt = 4;
  return;
endfunction



# data: data array to plot
# range: histogram range, format: [lower_limit:binwidth:upper_limit]
#	Data outside the limit will be lumped into left-/rightmost bin
# binwidth: binwidth for the histogram
# colorindex (optional): index color to use in the global color array
function h = transparent_hist(data, range, binwidth, colorindex)
  # color definitions and optional parameter
  global colors;
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
  h = fill(xs, ys, colors{colorindex});
  set(h, "edgecolor", colors{colorindex}, "facecolor", colors{colorindex},
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
  global max_hist_bins;
  if (!exist("bins", "var"))
    bins = max_hist_bins;
  endif

  # binwidth should not be below one
  binwidth = max(1, (upper - lower) / bins);
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
