#!/usr/bin/octave -qf

# function to calculate and plot the differences between kernel and user
# space arrival times
function eval_kutime(ktime, utime, filename)
  timediff = utime .- ktime;
  u = max(timediff);
  m = median(timediff);
  l = min(timediff);
  s = std(timediff);
  printf("\nEvaluation of differences between kernel and user space arrival times\n");
  printf("Upper limit: %ld µs\n", u);
  printf("Lower limit: %ld µs\n", l);
  printf("Average: %ld µs\n", mean(timediff));
  printf("Median: %ld µs\n", m);
  printf("Standard deviation: %ld µs\n", s);

  hist(timediff, [l:(m + 2 * s)], 1);
  title("Distribution of difference between kernel and user space arrival times [us]");

  if exist("filename", "var") && ischar(filename)
    plotfile = strcat(filename, "-kutime.jpg");
    print(plotfile, "-djpg");
  endif
endfunction



# calculate inter arrival times
function eval_iat(ktime, filename)
  iats = diff(ktime);
  u = max(iats);
  m = median(iats);
  l = min(iats);
  s = std(iats);
  printf("\nEvaluation of inter arrival times\n");
  printf("Upper limit: %ld µs\n", u);
  printf("Lower limit: %ld µs\n", l);
  printf("Average: %ld µs\n", mean(iats));
  printf("Median: %ld µs\n", m);
  printf("Standard deviation: %ld µs\n", s);

  hist(iats, [(m - 2 * s):(m + 2 * s)], 1);
  title("Distribution of inter arrival times [us]");

  if exist("filename", "var") && ischar(filename)
    plotfile = strcat(filename, "-iat.jpg");
    print(plotfile, "-djpg");
  endif
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
endfunction



# read arguments and get the input file's name
arg_list = argv();
if nargin() < 1
  printf("Usage: %s FILENAME", program_name());
  exit(1);
endif

filename = arg_list{1};
printf("Reading data from %s: ", filename);
# read test output
A = dlmread(filename, "\t", 1, 0);
printf("%i data sets\n", length(A));
# first column: arrival time (kernel)
ktime = A( :, 1);
# second column: arrival time (user space)
utime = A( :, 2);
# 5th column: sequence numbers
seqnos = A( :, 5);

chk_seq(seqnos);
eval_kutime(ktime, utime, filename);
eval_iat(ktime, filename);
