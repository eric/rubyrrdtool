# $Id: extconf.rb,v 1.4 2008/03/11 22:47:06 dbach Exp $

require 'mkmf'

# If there are build errors, be sure to say where your rrdtool lives:
#
# ruby ./extconf.rb --with-rrd-dir=/usr/local/rrdtool-1.2.12

libpaths=%w(/lib /usr/lib /usr/local/lib /sw/lib /opt/local/lib)
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

