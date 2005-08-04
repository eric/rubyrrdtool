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
(start, stop, names, data) = rrd.fetch(["AVERAGE"])
puts ">Names .."
names.each do  |name|
    puts " --> #{name}"
end
puts ">Data .."
t = start
data.each do  |val|
    puts " #{DateTime.new(t)} --> #{val}"
    t += 300  # know this from elsewhere
end


