#!/usr/bin/env ruby

require 'date'
require "RRDtool"

name    = "minmax"
rrdname = "randome.rrd"
start   = Time.now.to_i

rrd = RRDtool.new(rrdname)
puts "created new RRD database -> #{rrd.rrdname}"

# ---------------------------------------------
#           Update
# ---------------------------------------------
puts " .. fetching #{rrdname}"
(start, stop, step, cols, legends, data) = rrd.xport(["-m", 400,
               "--start", "now-1day",
               "--end", "now",
               "DEF:alpha=#{rrdname}:a:AVERAGE",
               "CDEF:calc=alpha,alpha,+,2,/,100,*,102,/",
               "XPORT:alpha:original ds",
               "XPORT:calc:calculated values"])
puts ">Names .."
legends.each do  |legend|
    puts " --> #{legends}"
end
puts ">Data .."
t = start
data.each do  |val|
    puts " #{t} --> #{val}"
    t += 300  # know this from elsewhere
end


