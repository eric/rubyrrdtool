# $Id: extconf.rb,v 1.1 2005/08/04 01:16:07 probertm Exp $
# Lost ticket pays maximum rate.

require 'mkmf'

dir_config("rrd")
have_library("rrd", "rrd_create")
create_makefile("RRDtool")
