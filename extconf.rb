# $Id: extconf.rb,v 1.3 2006/10/17 23:34:06 dbach Exp $
# Lost ticket pays maximum rate.

require 'mkmf'

# Be sure to say where you rrdtool lives:
# ruby ./extconf.rb --with-rrd-dir=/usr/local/rrdtool-1.2.12
libpaths=%w(/lib /usr/lib /usr/local/lib)
%w(art_lgpl_2 freetype png z).sort.reverse.each do |lib|
	find_library(lib, nil, *libpaths)
end

dir_config("rrd")
# rrd_first is only defined rrdtool >= 1.2.x
have_library("rrd", "rrd_first")

# rrd_dump_r has 2nd arg in rrdtool >= 1.2.14
if try_link(<<EOF)
#include <rrd.h>
main()
{
  rrd_dump_r("/dev/null", NULL);
}
EOF
    $CFLAGS += " -DHAVE_RRD_DUMP_R_2"
end

create_makefile("RRDtool")

