#!/usr/bin/perl

use strict;
use IO::Socket::INET;
use Time::HiRes qw(usleep ualarm gettimeofday tv_interval);

# Enable autoflush STDOUT
$|=1;

my ($socket,$received_data);
my ($peer_address,$peer_port);

$socket = new IO::Socket::INET (
	LocalPort => '8888',
	LocalAddr => '10.20.20.1',
	Proto => 'udp',
) or die "ERROR in Socket Creation : $!\n";

my $port = undef;

my $counter = 0;
my $s_previous;
my $delta;

my $startime = time;

# Loop over multiple run
while(1) {
   eval {

      while (1) {
	 my ($s, $usec) = gettimeofday();
	 $delta++;
	 if ($s != $s_previous) {
	     $s_previous = $s;
	     print STDERR "received rate: $delta\n";
	     $delta=0;
	 }

	 $socket->recv($received_data,1024);
	 if (length($received_data)==2) {
	    my $a = ord(substr($received_data,0));
	    my $b = ord(substr($received_data,1));
	    printf("%08d;%03d;%03d\n",($s*1000000+$usec)-($startime*1000000),$a,-(255-$b));
	 } else {
	    die ("Errore!");
	 }

	#$peer_address = $socket->peerhost();
	#$peer_port = $socket->peerport();
	#printf("%d.%06d;%03d;%03d\n",$s,$usec,ord($a), ord($b));
      }
    };
    if ($@) {
	print "Errore: [$@]\n";
	sleep 5;
	
    }
}
