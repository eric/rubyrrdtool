#!/usr/bin/env ruby
# $Id: function_test.rb,v 1.2 2006/10/18 21:43:38 dbach Exp $
# Driver does not carry cash.

# insert the lib directory at the front of the path to pick up 
# the latest build -- useful for testing during development 
$:.unshift '../lib'

require 'RRDtool'

name    = "test"
rrdname = "#{name}.rrd"
start   = Time.now.to_i

rrd = RRDtool.new(rrdname)
puts "created new RRD database -> #{rrd.rrdname}"


# ---------------------------------------------
#           Version
# ---------------------------------------------
puts "RRD Version "
n = RRDtool.version
puts "version() returned #{(n.nil?) ? "nil" : n}"
puts




# ---------------------------------------------
#           Create
# ---------------------------------------------
puts "creating #{rrdname}"
n = rrd.create(
      300, start-1, 
      ["DS:a:GAUGE:600:U:U",
      "DS:b:GAUGE:600:U:U",
      "RRA:AVERAGE:0.5:1:300"])
puts "create() returned #{(n.nil?) ? "nil" : n}"
puts


# ---------------------------------------------
#           Update
# ---------------------------------------------
puts "updating #{rrdname}"
val = start.to_i
while (val < (start.to_i+300*300)) do
    rrd.update("a:b", ["#{val}:#{rand()}:#{Math.sin(val / 3000) * 50 + 50}"])
    val += 300
end
puts




# ---------------------------------------------
#           Dump
# ---------------------------------------------
puts "dumping #{rrdname}"
n = rrd.dump
puts "dump() returned #{(n.nil?) ? "nil" : n}"
puts



# ---------------------------------------------
#           Fetch
# ---------------------------------------------
puts "fetching data from #{rrdname}"
arr = rrd.fetch(["AVERAGE"])
data = arr[3]
puts "got #{data.length} data points"
puts



# ---------------------------------------------
#           Info
# ---------------------------------------------
puts "RRD info "
h = rrd.info
puts "info() returned #{h.size} items"
h.each do |k, v|
    puts " #{k} --> #{v}"
end
puts



# ---------------------------------------------
#           Graph
# ---------------------------------------------
puts "generating graph #{name}.png"
rrd.graph(
    ["#{name}.png",
    "--title", " RubyRRD Demo", 
    "--start", "#{start} + 1 h",
    "--end", "#{start} + 1000 min",
    "--interlace", 
    "--imgformat", "PNG",
    "--width=450",
    "DEF:a=#{rrd.rrdname}:a:AVERAGE",
    "DEF:b=#{rrd.rrdname}:b:AVERAGE",
    "CDEF:line=TIME,2400,%,300,LT,a,UNKN,IF",
    "AREA:b#00b6e4:beta",
    "AREA:line#0022e9:alpha",
    "LINE3:line#ff0000"])
puts


exit   # <<<<============== STOP ==================

# ---------------------------------------------
#           First
# ---------------------------------------------
puts "first value for #{rrdname}"
n = rrd.first(nil)
puts "first() returned #{(n.nil?) ? "nil" : n}"
n = rrd.first(0)
puts "first(0) returned #{(n.nil?) ? "nil" : n}"
puts

# ---------------------------------------------
#           Last
# ---------------------------------------------
puts "last value for #{rrdname}"
n = rrd.last
puts "last() returned #{(n.nil?) ? "nil" : n}"
puts


print "This script has created #{name}.png in the current directory\n";
print "This demonstrates the use of the TIME and % RPN operators\n";
