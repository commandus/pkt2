ACLOCAL_AMFLAGS = -I m4

nobase_dist_include_HEADERS =
 
bin_PROGRAMS = t1

commonlibs = -L/usr/local/lib/ -lnanomsg
commoncppflags = -std=c++11
#
# tcpemitter
#
t1_SOURCES = \
	t1.cpp \
	$(common_src)

t1_LDADD = $(commonlibs)
t1_CPPFLAGS = $(commoncppflags)

configdir = $(datadir)
dist_config_DATA = p1.sh
