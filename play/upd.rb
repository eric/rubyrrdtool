#!/usr/bin/env ruby

require "RRDtool"

name    = "minmax"
rrdname = "randome.rrd"
start   = Time.now.to_i

rrd = RRDtool.new(rrdname)
puts "created new RRD database -> #{rrd.rrdname}"

# ---------------------------------------------
#           Update
# ---------------------------------------------
puts " .. updating #{rrdname}"
val = start.to_i
while (val < (start.to_i+300*300)) do
    rrd.update("a", ["#{val}:#{Math.sin(val / 3000) * 50 + 50}"])
    val += 300
end

