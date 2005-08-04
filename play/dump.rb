#!/usr/bin/env ruby

require "RRDtool"

name    = "minmax"
rrdname = "randome.rrd"
start   = Time.now.to_i

rrd = RRDtool.new(rrdname)
rrd.dump
