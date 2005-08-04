#! /usr/bin/perl

use lib qw( /usr/local/rrdtool-1.2.10/lib/perl );

use RRDs;
my $start=time;
my $rrd="randome.rrd";
my $name = $0;
$name =~ s/\.pl.*//g;

# dropt some data into the rrd
my $t;
for ($t=$start; $t<$start+300*300; $t+=300){
  RRDs::update $rrd, "$t:".(sin($t/3000)*50+50);
  if ($ERROR = RRDs::error) {
    die "$0: unable to update `$rrd': $ERROR\n";
  }
}
