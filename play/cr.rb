#!/usr/bin/env ruby

require "RRDtool"

name    = "minmax"
rrdname = "randome.rrd"
start = Time.now.to_i

rrd = RRDtool.new(rrdname)
puts "createding new RRD database -> #{rrd.rrdname}"

# ---------------------------------------------
#           Create
# ---------------------------------------------
puts " .. creating #{rrdname}"
n = rrd.create(
      300, start-1, 
      ["DS:a:GAUGE:600:U:U",
       "RRA:AVERAGE:0.5:1:300",
       "RRA:MIN:0.5:12:300",
       "RRA:MAX:0.5:12:300"])
