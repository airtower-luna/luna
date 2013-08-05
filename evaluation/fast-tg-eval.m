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
