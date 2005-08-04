#!/usr/bin/env ruby

require "RRDtool"

name    = "minmax"
# rrdname = "randome.rrd"
rrdname = "foo.rrd"

rrd = RRDtool.new(rrdname)
rrd.restore("foo.xml", rrdname, [])
