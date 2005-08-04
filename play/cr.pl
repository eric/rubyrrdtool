#! /usr/bin/perl

use lib qw( /usr/local/rrdtool-1.2.10/lib/perl );

use RRDs;
my $start=time;
my $rrd="randome.rrd";
my $name = $0;
$name =~ s/\.pl.*//g;

RRDs::create ($rrd, "--start",$start-1, "--step",300,
	      "DS:a:GAUGE:600:U:U",
	      "RRA:AVERAGE:0.5:1:300",
	      "RRA:MIN:0.5:12:300",
	      "RRA:MAX:0.5:12:300",
);
my $ERROR = RRDs::error;
die "$0: unable to create `$rrd': $ERROR\n" if $ERROR;

