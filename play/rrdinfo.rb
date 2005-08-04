#!/usr/bin/env ruby

require "RRDtool"


def usage
    puts "usage: #{$0} <rrdname>"
    exit
end

rrdname = ARGV[0]
usage if rrdname.nil?

rrd = RRDtool.new(rrdname)
puts "Checking RRD database -> #{rrd.rrdname}"

h = rrd.info
puts "info() returned #{h.size} items"
h.each do |k, v|
    puts " #{k} --> #{v}"
end
puts
