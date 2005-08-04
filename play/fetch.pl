#! /usr/bin/perl

use lib qw( /usr/local/rrdtool-1.2.10/lib/perl );

use RRDs;
my $start=time;
my $rrd="randome.rrd";
my $name = $0;
$name =~ s/\.pl.*//g;

my ($start,$step,$names,$array) = RRDs::fetch $rrd, "AVERAGE";
$ERROR = RRDs::error;
print "ERROR: $ERROR\n" if $ERROR ;
print "start=$start, step=$step\n";
print "                    ";
map {printf("%12s",$_)} @$names ;
print "\n";
foreach my $line (@$array){
    print "".localtime($start),"   ";
    $start += $step;
    foreach my $val (@$line) {
        printf "%12.1f", $val;
    }
    print "\n";
}

