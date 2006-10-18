#!/usr/bin/env ruby

require 'rubygems'
require 'hoe'

RUBY_RRDTOOL_VERSION = '0.6.0'

Hoe.new("rrdtool", RUBY_RRDTOOL_VERSION) do |p|
  p.rubyforge_name = "rubyrrdtool"
  p.summary = "Ruby language binding for RRD tool version 1.2+"
end

desc "Build binary driver"
task :build do
  puts `[ -e Makefile ] || ruby extconf.rb; make`
end

desc "Rebuild binary driver"
task :rebuild do
  puts `ruby extconf.rb; make clean all`
end
