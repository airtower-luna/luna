#!/usr/bin/perl
use strict;
use threads;
use threads::shared;
use Net::OpenSSH;

my @conns;
my @workers;
my %general = ("default_exec" => "/usr/local/bin/fast-tg");

my $conffile = $ARGV[0];
die "Configuration file missing or not readable!\n" if (! -r $conffile);

my $rw = open(CONF, $conffile);
die "Could not open configuration file: $!\n" if(not defined($rw));

# parse configuration
my $current = undef;
my $c = 0;
while(defined(my $i = <CONF>))
{
    $c++;
    # ignore comments and empty lines
    next if ($i =~ /^\#.*$/ or $i =~ /^$/);

    # check for section header
    if ($i =~ /\[(\w*)\]/)
    {
	$current = &init_conn($1);
	push(@conns, $current);
	next;
    }

    # normal variable assignment
    if ($i =~ /^\s*(\w*)=(.*)$/)
    {
	if (not defined $current)
	{
	    $general{$1} = $2;
	}
	else
	{
	    $current->{$1} = $2;
	}
	next;
    }

    # Anything unknown, should never happen with a valid configuration file
    chomp($i);
    die "Configuration file line ".$c." is invalid: \"".$i."\"\n";
}
close(CONF);

# show configuration (just for development)
foreach my $i (keys(%general))
{
    print $i."=".$general{$i}."\n";
}
foreach my $conn (@conns)
{
    print "\n";
    foreach my $i (keys(%{$conn}))
    {
	print $i."=".$conn->{$i}."\n";
    }
}

# Start a worker thread for each connection.
for (my $i = 0; $i < @conns; $i++)
{
     $workers[$i] = threads->create({'context' => 'void'},
				    \&run_connection,
				    $conns[$i]);
}

# Wait until all worker threads have terminated, order doesn't matter.
foreach my $thr (@workers)
{
    $thr->join();
}



# This function runs one connection, passed as a hash reference.
sub run_connection
{
    my $conn = $_[0];
    if (not defined $conn->{target})
    {
	$conn->{target} = $conn->{server};
    }

    $conn->{server_ssh} = &start_ssh($conn->{server});
    $conn->{client_ssh} = &start_ssh($conn->{client});
    # Running these three functions synchronously ensures that the
    # server is (most likely) ready when the client starts running,
    # and the transmission is (certainly) complete before the server
    # is stopped.
    &start_server($conn);
    &run_client($conn);
    &stop_server($conn);
}



# Establish an SSH connection to the given host, passed as a string
# parameter in the usual format: [user@]server[:sshport]
sub start_ssh
{
    my $host = $_[0];
    my $ssh = Net::OpenSSH->new($host);
    $ssh->error and
	die "Couldn't establish SSH connection with ".$host.": ". $ssh->error;
    return $ssh;
}



# Start the server for this connection
# One parameter: a connection hash reference
sub start_server
{
    my $conn = $_[0];
    my $ssh = $conn->{server_ssh};

    # create PID file on the server
    $conn->{server_pidfile} = $ssh->capture("tempfile");
    die "Could not create server PID file: " if $ssh->error;
    chomp($conn->{server_pidfile});
    # create output file on the server
    $conn->{server_outfile} = $ssh->capture("tempfile");
    die "Could not create server output file: " if $ssh->error;
    chomp($conn->{server_outfile});

    # build server start command
    my $command = "/sbin/start-stop-daemon --start";
    # set PID file
    $command = $command." --pidfile ".$conn->{server_pidfile};
    $command = $command." --exec ".$conn->{server_exec}." --background --make-pidfile --";
    # server parameters
    $command = $command." -s -T -p ".$conn->{port}." -o ".$conn->{server_outfile};
    $ssh->system($command);
    print "Command \"".$command."\" failed: ". $ssh->error."\n"
	if ($ssh->error);
}



# Stop the server for this connection
# One parameter: a connection hash reference
sub stop_server
{
    my $conn = $_[0];
    my $ssh = $conn->{server_ssh};

    # stop the server, using the stored PID
    my $command = "kill -TERM \$(cat ".$conn->{server_pidfile}.")";
    $ssh->system($command);
    print "Command \"".$command."\" failed: ". $ssh->error."\n"
	if ($ssh->error);

    # delete PID file
    my $command = "rm ".$conn->{server_pidfile};
    $ssh->system($command);
    print "Command \"".$command."\" failed: ". $ssh->error."\n"
	if ($ssh->error);

    # copy output to local storage
    $ssh->scp_get({async => 0}, $conn->{server_outfile},
		  $conn->{server_output});
    print "Copying server output to ".$conn->{server_output}." failed: ".$ssh->error."\n" if ($ssh->error);

    # delete output file on the server
    my $command = "rm ".$conn->{server_outfile};
    $ssh->system($command);
    print "Command \"".$command."\" failed: ". $ssh->error."\n"
	if ($ssh->error);
}



# Run the client process and transfer the output, if echo was
# requested.
# One parameter: a connection hash reference
sub run_client
{
    my $conn = $_[0];
    my $ssh = $conn->{client_ssh};

    # build client command
    my $command = $conn->{client_exec}." -c ".$conn->{target}.
	" -p ".$conn->{port}." -t ".$conn->{time};

    # add -e and output file, if requested
    if ($conn->{echo} eq "true")
    {
	# crate output file
	$conn->{client_outfile} = $ssh->capture("tempfile");
	die "Could not create client output file: " if $ssh->error;
	chomp($conn->{client_outfile});
	$command = $command." -e -o ".$conn->{client_outfile};
    }

    # add generator settings, if present
    if (defined $conn->{generator})
    {
	$command = $command." -g ".$conn->{generator};
	if (defined $conn->{generator_args})
	{
	    $command = $command." -a ".$conn->{generator_args};
	}
    }

    # run client
    $ssh->system($command);
    print "Command \"".$command."\" failed: ". $ssh->error."\n"
	if ($ssh->error);

    # get output, if any
    if (defined $conn->{client_outfile})
    {
	$ssh->scp_get({async => 0}, $conn->{client_outfile}, $conn->{client_output});
	print "Copying client output to ".$conn->{client_output}." failed: ".$ssh->error."\n" if ($ssh->error);

	my $command = "rm ".$conn->{client_outfile};
	$ssh->system($command);
	print "Command \"".$command."\" failed: ". $ssh->error."\n"
	    if ($ssh->error);
    }
}



# Intialize connection with default values, as far as possible. The
# first and only parameter is the connection identifier (section name
# in the configuration file).
sub init_conn
{
    my %conn =
	(
	 "id" => $_[0],
	 # The following values are configuration options:
	 "server" => undef,
	 "client" => undef,
	 "port" => 4567,
	 # If the client should use a different IP/hostname as the
	 # transmission target than the one specified in server, set
	 # it here.
	 "target" => undef,
	 # generator settings for the client
	 "generator" => undef,
	 "generator_args" => undef,
	 # time for the client to run, in seconds
	 "time" => 2,
	 # set to true to request echo packets
	 "echo" => undef,
	 # commands to run
	 "server_exec" => $general{default_exec},
	 "client_exec" => $general{default_exec},
	 # local output files
	 "server_output" => undef,
	 "client_output" => undef,
	 # Variables below this point are internal, not for configuration
	 "server_ssh" => undef,
	 "server_pidfile" => undef,
	 "server_outfile" => undef,
	 "client_ssh" => undef,
	 "client_outfile" => undef,
	);
    return \%conn;
}
