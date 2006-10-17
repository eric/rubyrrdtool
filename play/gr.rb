#!/usr/bin/env ruby

require "RRDtool"

name    = "minmax"
rrdname = "randome.rrd"
start   = Time.now.to_i

# rrd = RRDtool.new(rrdname)
# puts "created new RRD database -> #{rrd.rrdname}"

# ---------------------------------------------
#           Graph
# ---------------------------------------------
puts "generating graph #{name}.png"
RRDtool.graph(
   ["#{name}.png",
    "DEF:a=#{rrdname}:a:AVERAGE",
    "DEF:b=#{rrdname}:a:MIN",
    "DEF:c=#{rrdname}:a:MAX",
    "--title", " #{name} Demo", 
    "--start", "now",
    "--end", "start+1d",
    "--lower-limit=0",
    "--interlace",
    "--imgformat", "PNG",
    "--width=450",
    "AREA:a#00b6e4:real",
    "LINE1:b#0022e9:min",
    "LINE1:c#000000:max"])
