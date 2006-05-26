# $Id: extconf.rb,v 1.2 2006/05/26 19:38:50 dbach Exp $
# Lost ticket pays maximum rate.

require 'mkmf'

# Be sure to say where you rrdtool lives:
# ruby ./extconf.rb --with-rrd-dir=/usr/local/rrdtool-1.2.12
libpaths=%w(/lib /usr/lib /usr/local/lib)
%w(art_lgpl_2 freetype png z).sort.reverse.each do |lib|
	find_library(lib, nil, *libpaths)
end

dir_config("rrd")
have_library("rrd", "rrd_create")
create_makefile("RRDtool")
