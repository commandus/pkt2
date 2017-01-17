SUBDIRS = .
ACLOCAL_AMFLAGS = -I m4 

commoncppflags = -DDEBUG

#
#	Exclude generated sources from distribution
#
genfiles =
nodist_tcpreceiver_SOURCES = $(genfiles)
BUILT_SOURCES = $(genfiles)
CLEANFILES = $(genfiles)

#
# Binaries
#
bin_PROGRAMS=tcpemitter tcpreceiver pkt2receiver pkt2gateway handlerpq tcptransmitter

#
# Headers
#
nobase_dist_include_HEADERS = \
NanoMessage.h   cpp-syslog.h    daemonize.h   ieee754.h    platform.h  \
utilpriority.h  utilstring.h    utilinet.h    bin2ascii.h \
tcpreceiver-config.h pkt2receiver-config.h pkt2gateway-config.h handlerpq-config.h tcptransmitter-config.h \
tcpemitter-config.h tcpreceivernano.h \
json/json.h  json/json-forwards.h \
rapidjson/allocators.h           rapidjson/encodings.h        rapidjson/fwd.h             rapidjson/memorystream.h    rapidjson/prettywriter.h  rapidjson/schema.h        rapidjson/writer.h \
rapidjson/document.h             rapidjson/filereadstream.h   rapidjson/istreamwrapper.h  rapidjson/ostreamwrapper.h  rapidjson/rapidjson.h     rapidjson/stream.h \
rapidjson/encodedstream.h        rapidjson/filewritestream.h  rapidjson/memorybuffer.h    rapidjson/pointer.h         rapidjson/reader.h        rapidjson/stringbuffer.h \
rapidjson/error/en.h             rapidjson/error/error.h \
rapidjson/internal/biginteger.h  rapidjson/internal/dtoa.h     rapidjson/internal/itoa.h  rapidjson/internal/pow10.h  rapidjson/internal/stack.h    rapidjson/internal/strtod.h \
rapidjson/internal/diyfp.h       rapidjson/internal/ieee754.h  rapidjson/internal/meta.h  rapidjson/internal/regex.h  rapidjson/internal/strfunc.h  rapidjson/internal/swap.h \
rapidjson/msinttypes/inttypes.h  rapidjson/msinttypes/stdint.h

common_src =

commonlibs = -L/usr/local/lib/ -lpthread -ldl -largtable2

#
# tcpemitter
#
tcpemitter_SOURCES = \
	tcpemitter.cpp  tcpemitter-config.cpp  \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)

tcpemitter_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg
tcpemitter_CPPFLAGS = $(commoncppflags) -std=c++11

#
# tcpreceiver
#
tcpreceiver_SOURCES = \
	tcpreceiver.cpp  tcpreceiver-config.cpp tcpreceivernano.cpp \
	daemonize.cpp  ieee754.cpp  \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)

tcpreceiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg
tcpreceiver_CPPFLAGS = $(commoncppflags) -std=c++11

#
#  pkt2receiver
#
pkt2receiver_SOURCES= \
	pkt2receiver.cpp  pkt2receiver-config.cpp  \
	daemonize.cpp  ieee754.cpp  \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)

pkt2receiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg
pkt2receiver_CPPFLAGS = $(commoncppflags) -std=c++11 

#
#  pkt2gateway
#
pkt2gateway_SOURCES= \
	pkt2gateway.cpp  pkt2gateway-config.cpp  \
	daemonize.cpp  ieee754.cpp  \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)

pkt2gateway_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg
pkt2gateway_CPPFLAGS = $(commoncppflags) -std=c++11 

#
# handlerpq
#
handlerpq_SOURCES= \
	handlerpq.cpp  handlerpq-config.cpp  \
	daemonize.cpp  ieee754.cpp  \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)

handlerpq_LDADD = $(commonlibs) -lodb-pgsql -lodb -lglog -lunwind -lnanomsg
handlerpq_CPPFLAGS = $(commoncppflags) -std=c++11

#
# tcptransmitter
#
tcptransmitter_SOURCES = \
	tcptransmitter.cpp  tcptransmitter-config.cpp  \
	daemonize.cpp  ieee754.cpp  
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)

tcptransmitter_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg
tcptransmitter_CPPFLAGS = $(commoncppflags) -std=c++11

#
# Configs, readme, CMake etc.
#
configdir = $(datadir)
dist_config_DATA = README.md HISTORY INSTALL \
	CMakeLists.txt cmake/FindArgtable2.cmake cmake/FindGlog.cmake cmake/FindNanomsg.cmake \
	tools/mkdependencies tools/mkdocker tools/mktools tools/docker/ubuntu/Dockerfile

