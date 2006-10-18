#!/usr/bin/env ruby

require "RRDtool"

name    = "minmax"
rrdname = "#{name}.rrd"
start   = Time.now.to_i

rrd = RRDtool.new(rrdname)
puts "created new RRD database -> #{rrd.rrdname}"

puts "vers -> #{RRDtool.version}"

# ---------------------------------------------
#           Create
# ---------------------------------------------
puts " .. creating #{rrdname}"
n = rrd.create(
      300, start-1, 
      ["DS:ds_0:GAUGE:600:U:U",
       "RRA:AVERAGE:0.5:1:300",
       "RRA:MIN:0.5:12:300",
       "RRA:MAX:0.5:12:300"])

# ---------------------------------------------
#           Update
# ---------------------------------------------
puts " .. updating #{rrdname}"
val = start.to_i
while (val < (start.to_i+300*300)) do
    rrd.update("ds_0", ["#{val}:#{Math.sin(val / 3000) * 50 + 50}"])
    val += 300
    # sleep(1)
end



# ---------------------------------------------
#           Graph
# ---------------------------------------------
puts "generating graph #{name}.png"
RRDtool.graph(
   ["#{name}.png",
    "DEF:a=#{rrdname}:ds_0:AVERAGE",
    "DEF:b=#{rrdname}:ds_0:MIN",
    "DEF:c=#{rrdname}:ds_0:MAX",
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


puts "This script has created #{rrdname}.png in the current directory"
puts "This demonstrates the use of the TIME and % RPN operators"
