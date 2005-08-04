#! /usr/bin/perl

use lib qw( /usr/local/rrdtool-1.2.10/lib/perl );

use RRDs;
my $start=time;
my $rrd="randome.rrd";
my $name = $0;
$name =~ s/\.pl.*//g;

RRDs::graph "$name.png",
  "--title", uc($name)." Demo", 
  "--start", "now",
  "--end", "start+1d",
  "--lower-limit=0",
  "--interlace", 
  "--imgformat","PNG",
  "--width=450",
  "DEF:a=$rrd:a:AVERAGE",
  "DEF:b=$rrd:a:MIN",
  "DEF:c=$rrd:a:MAX",
  "AREA:a#00b6e4:real",
  "LINE1:b#0022e9:min",
  "LINE1:c#00ee22:max",
;

if ($ERROR = RRDs::error) {
  print "ERROR: $ERROR\n";
};

