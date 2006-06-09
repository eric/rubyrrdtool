# -*- ruby -*-

require 'rubygems'
Gem::manage_gems

require 'rake'
require 'rake/testtask'
require 'rake/rdoctask'
require 'rake/gempackagetask'
require 'rbconfig'

$PACKAGE_NAME = 'rubyrrdtool'
$VERSION = '0.5.1'
$VERBOSE = nil

desc 'Build release'
Rake::PackageTask.new($PACKAGE_NAME, $VERSION) do |p|
  p.need_tar = true
  IO.readlines("Manifest.txt").each { |f| p.package_files.include(f.chomp) }
end

#spec = Gem::Specification.new do |s|
#  s.name = 'rubyrrdtool'
#  s.version = $VERSION
#  s.authors = ['Mark Probert', 'David Bacher']
#  s.summary = 'Ruby bindings for RRDtool'
#
#  s.files = IO.readlines("Manifest.txt").map { |f| f.chomp }
#  s.require_path = 'lib'
#
#  if $DEBUG then
#    puts "rubyrrdtool #{s.version}"
#    puts
#    puts s.executables.sort.inspect
#    puts
#    puts "** summary:"
#    puts s.summary
#    puts
#    puts "** description:"
#    puts s.description
#  end
#
#  s.rubyforge_project = "rubyrrdtool"
#  s.has_rdoc = false
#end

#desc 'Build Gem'
#Rake::GemPackageTask.new spec do |pkg|
#  pkg.need_tar = true
#end

desc 'Run tests'
task :default => :test

desc 'Run tests'
Rake::TestTask.new :test do |t|
  t.libs << 'test'
  t.verbose = true
end

#desc 'Update Manifest.txt'
#task :update_manifest => :clean do
#  sh "p4 open Manifest.txt; find . -type f | sed -e 's%./%%' | sort > Manifest.txt"
#end

desc 'Generate RDoc'
Rake::RDocTask.new :rdoc do |rd|
  rd.rdoc_dir = 'doc'
  rd.rdoc_files.add 'lib', 'README'
  rd.main = 'README'
  rd.options << '-t rubyrrdtool RDoc'
end

#$prefix = ENV['PREFIX'] || Config::CONFIG['prefix']
#$bin  = File.join($prefix, 'bin')
#$lib  = Config::CONFIG['sitelibdir']
#$bins = spec.executables
#$libs = spec.files.grep(/^lib\//).map { |f| f.sub(/^lib\//, '') }.sort

task :blah do
    p $bins
    p $libs
end

task :install do
  $bins.each do |f|
    install File.join("bin", f), $bin, :mode => 0555
  end

  $libs.each do |f|
    dir = File.join($lib, File.dirname(f))
    mkdir_p dir unless test ?d, dir
    install File.join("lib", f), dir, :mode => 0444
  end
end

task :uninstall do
  # add old versions
  $libs << "RRDtool.so"  
  $libs << "rrdtool.rb"  

  $bins.each do |f|
    rm_f File.join($bin, f)
  end

  $libs.each do |f|
    rm_f File.join($lib, f)
  end

  rm_rf File.join($lib, "test")
end

desc 'Clean up'
task :clean => [ :clobber_rdoc, :clobber_package ] do
  rm_f Dir["**/*~"]
end
