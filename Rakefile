#!/usr/bin/env ruby
# -*- ruby -*-

require 'rubygems'
require 'hoe'

RUBY_RRDTOOL_VERSION = '0.6.0'

Hoe.new("RubyRRDtool", RUBY_RRDTOOL_VERSION) do |p|
  p.rubyforge_name = "rubyrrdtool"
  p.summary = "Ruby language binding for RRD tool version 1.2+"
  p.description = p.paragraphs_of('README', 2..3).join("\n\n")

  p.url = "http://rubyforge.org/projects/rubyrrdtool/"

  p.author = [ "David Bacher", "Mark Probert" ]
  p.email = "drbacher @nospam@ alum dot mit dot edu"

  p.spec_extras = { 'extensions' => 'extconf.rb' }

  p.changes =<<EOC
* applied patches (thanks to Hrvoje Marjanovic, Akihiro Sagawa and Thorsten von Eicken)
* changed version, graph, xport methods to be class methods
* removed dump method from pre two-arg-dump rrdtool versions
* fixed compiler warnings in debug output
* hoe-ified Rakefile
EOC
end

desc "Build RRD driver"
task :build do
  puts `[ -e Makefile ] || ruby extconf.rb; make`
end

desc "Rebuild RRD driver"
task :rebuild do
  puts `ruby extconf.rb; make clean all`
end
