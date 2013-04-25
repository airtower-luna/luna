#!/usr/bin/octave -qf

# function to calculate and plot the differences between kernel and user
# space arrival times
function eval_kutime(ktime, utime, filename)
  timediff = utime .- ktime;
  u = max(timediff);
  l = min(timediff);
  printf("Upper limit: %ld\n", u);
  printf("Lower limit: %ld\n", l);
  printf("Average: %ld\n", mean(timediff));
  printf("Median: %ld\n", median(timediff));
  printf("Standard deviation: %ld\n", std(timediff));

  hist(timediff, [l:150], 1);
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

eval_kutime(ktime, utime, filename);
eval_iat(ktime, filename);
