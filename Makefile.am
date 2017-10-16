SUBDIRS = . tests
ACLOCAL_AMFLAGS = -I m4

TESTS = tests/p1.sh tests/t1 

if DEBUG
	AM_CFLAGS = -g3 -O0
	AM_CXXFLAGS = -g3 -O0 -DDEBUG
else
	AM_CFLAGS = -O2
	AM_CXXFLAGS = -O2
endif

# ----------------- REMOVE IT!!!! ------------------
# -std=c++11
AM_CXXFLAGS =  -O0 -DDEBUG
#COMMON_CPP_FLAGS = 
# ----------------- REMOVE IT!!!! ------------------

# SNMPLIBS = `net-snmp-config --agent-libs`
SNMPLIBS = -lnetsnmpagent -lnetsnmp -lnetsnmphelpers -lnetsnmpmibs -lpthread

gengrpcs = pkt2.pb.h pkt2.pb.cpp example/example1.pb.cpp
$(gengrpcs): Makefile
	protoc -I proto --cpp_out=. proto/google/protobuf/descriptor.proto
	protoc -I proto --cpp_out=. proto/pkt2.proto
	protoc -I proto --cpp_out=. proto/example/example1.proto
	sed -i '/#include "descriptor.pb.h"/d' pkt2.pb.h
	sed -i '/protobuf_InitDefaults_descriptor_2eproto();/d' pkt2.pb.cc
	sed -i '/protobuf_AddDesc_descriptor_2eproto();/d' pkt2.pb.cc
	mv pkt2.pb.cc pkt2.pb.cpp
	protoc -I proto --cpp_out=. proto/example/example1.proto
	mv example/example1.pb.cc example/example1.pb.cpp

#
#	Exclude generated sources from distribution
#

# genfiles = $(gengrpcs)
nodist_tcpreceiver_SOURCES = $(genfiles)
BUILT_SOURCES = $(gengrpcs)
CLEANFILES = $(gengrpcs)

#
#	Binaries
#
bin_PROGRAMS = pkt2 mqtt-receiver tcpreceiver pkt2receiver pkt2gateway tcptransmitter \
	tcpemitter messageemitter \
	protoc-gen-pkt2 \
	message2gateway example1message example1message1 \
	tcpemitter-example1 tcpemitter-iridium mqtt-emitter-iridium \
	handlerpq handlerline handlerlmdb handler-google-sheets \
	pkt2receiver-check \
	pkt2dumppq repeator freceiver

#
#	Headers
#
nobase_dist_include_HEADERS = \
NanoMessage.h        cpp-syslog.h    daemonize.h   ieee754.h    platform.h  packet2message.h \
utilpriority.h       utilstring.h    utilinet.h    bin2ascii.h  utilprotobuf.h utilfile.h \
protoc-gen-pkt2.h    pkt2_code_generator.h snmpagentpkt2.h get_rss.h protobuf-declarations.h \
tcpreceiver-config.h pkt2receiver-config.h pkt2gateway-config.h handlerpq-config.h tcptransmitter-config.h \
message2gateway-config.h helper_socket.h \
handlerlmdb-config.h lmdbwriter.h error-printer.h pkt2receivernano.h output-message.h \
tcpemitter-config.h tcpreceivernano.h input-packet.h utilsnmp.h pkt2packetvariable.h \
messageemitter-config.h \
linewriter.h handlerline-config.h messagedecomposer.h messagecomposer.h fieldnamevalueindexstrings.h \
sslhelper.h iridium.h \
pbjson.hpp errorcodes.h pkt2optionscache.h pqwriter.h tcpemitterline.h \
google-sheets.h \
rapidjson/allocators.h           rapidjson/encodings.h        rapidjson/fwd.h             rapidjson/memorystream.h    rapidjson/prettywriter.h  rapidjson/schema.h        rapidjson/writer.h \
rapidjson/document.h             rapidjson/filereadstream.h   rapidjson/istreamwrapper.h  rapidjson/ostreamwrapper.h  rapidjson/rapidjson.h     rapidjson/stream.h \
rapidjson/encodedstream.h        rapidjson/filewritestream.h  rapidjson/memorybuffer.h    rapidjson/pointer.h         rapidjson/reader.h        rapidjson/stringbuffer.h \
rapidjson/error/en.h             rapidjson/error/error.h \
rapidjson/internal/biginteger.h  rapidjson/internal/dtoa.h     rapidjson/internal/itoa.h  rapidjson/internal/pow10.h  rapidjson/internal/stack.h    rapidjson/internal/strtod.h \
rapidjson/internal/diyfp.h       rapidjson/internal/ieee754.h  rapidjson/internal/meta.h  rapidjson/internal/regex.h  rapidjson/internal/strfunc.h  rapidjson/internal/swap.h \
rapidjson/msinttypes/inttypes.h  rapidjson/msinttypes/stdint.h \
duk/duk_config.h  duk/duktape.h  javascript-context.h \
handler-google-sheets-config.h google-sheets-writer.h \
pqdumper.h pkt2dumppq-config.h \
mqtt-receivernano.h mqtt-receiver-config.h \
pkt2-config.h pkt2-impl.h \
pkt2receiver-check-config.h \
repeator-config.h  \
freceiver-impl.h freceiver-impl.h freceiver-config.h \
huff/huffcode.h  huff/huffmanmodifieddecoder.h  huff/internalnode.h  huff/leafnode.h  huff/node.h \
huff/bitstream.h huff/varint.h

huff_src = huff/huffcode.cpp  huff/huffmanmodifieddecoder.cpp  huff/internalnode.cpp  huff/leafnode.cpp  huff/node.cpp huff/bitstream.cpp

common_src =

commonlibs = -L/usr/local/lib/ -lpthread -ldl -largtable2

#
#	pkt2
#
pkt2_SOURCES = \
	pkt2.cpp pkt2-config.cpp pkt2-impl.cpp errorcodes.cpp \
	utilstring.cpp daemonize.cpp duk/duktape.c \
	$(common_src)

pkt2_LDADD = $(commonlibs) -lglog $(SNMPLIBS)
pkt2_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
#	tcpemitter
#
tcpemitter_SOURCES = \
	tcpemitter.cpp tcpemitter-config.cpp tcpemitterline.cpp \
	utilstring.cpp daemonize.cpp errorcodes.cpp \
	$(common_src)

tcpemitter_LDADD = $(commonlibs) -lglog $(SNMPLIBS)
tcpemitter_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
#	messageemitter
#
messageemitter_SOURCES = \
	messageemitter.cpp messageemitter-config.cpp errorcodes.cpp  \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp \
	messagedecomposer.cpp utilprotobuf.cpp error-printer.cpp protobuf-declarations.cpp \
	messageemitterline.cpp daemonize.cpp pkt2packetvariable.cpp pkt2optionscache.cpp \
	pkt2.pb.cpp utilfile.cpp duk/duktape.c javascript-context.cpp pbjson.cpp \
	$(common_src)

messageemitter_LDADD = $(commonlibs) -lglog -lprotobuf -lunwind -lnanomsg $(SNMPLIBS)
messageemitter_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON

#
#	tcpreceiver
#
tcpreceiver_SOURCES = \
	tcpreceiver.cpp tcpreceiver-config.cpp tcpreceivernano.cpp input-packet.cpp \
	daemonize.cpp snmpagentpkt2.c get_rss.c errorcodes.cpp \
	utilpriority.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(huff_src) \
	$(common_src)

tcpreceiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS)
tcpreceiver_CPPFLAGS = $(COMMON_CPP_FLAGS) -Ihuff

#
#	mqtt-receiver
#
mqtt_receiver_SOURCES = \
	mqtt-receiver.cpp   mqtt-receiver-config.cpp mqtt-receivernano.cpp input-packet.cpp \
	daemonize.cpp snmpagentpkt2.c get_rss.c errorcodes.cpp \
	utilpriority.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)
mqtt_receiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg -lpaho-mqtt3c $(SNMPLIBS)
mqtt_receiver_CPPFLAGS = $(COMMON_CPP_FLAGS)
	
#
#	freceiver
#
freceiver_SOURCES = \
	freceiver.cpp freceiver-config.cpp freceiver-impl.cpp input-packet.cpp \
	daemonize.cpp snmpagentpkt2.c errorcodes.cpp \
	utilpriority.cpp utilstring.cpp utilinet.cpp \
	$(common_src)

freceiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS)
freceiver_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
#	pkt2receiver
#
pkt2receiver_SOURCES= \
	pkt2receiver.cpp  pkt2receiver-config.cpp pkt2receivernano.cpp input-packet.cpp \
	daemonize.cpp pbjson.cpp utilfile.cpp errorcodes.cpp \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp error-printer.cpp \
	packet2message.cpp messagecomposer.cpp duk/duktape.c pkt2packetvariable.cpp \
	pkt2optionscache.cpp utilprotobuf.cpp protobuf-declarations.cpp pkt2.pb.cpp \
	javascript-context.cpp \
	$(common_src)

pkt2receiver_LDADD = $(commonlibs) -lglog -lunwind -lprotobuf -lnanomsg $(SNMPLIBS)
pkt2receiver_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON

#
#	pkt2gateway
#
pkt2gateway_SOURCES= \
	pkt2gateway.cpp  pkt2gateway-config.cpp packet2message.cpp \
	daemonize.cpp errorcodes.cpp \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp \
	pkt2optionscache.cpp protobuf-declarations.cpp pkt2packetvariable.cpp pkt2.pb.cpp utilprotobuf.cpp error-printer.cpp utilfile.cpp pbjson.cpp \
	messagecomposer.cpp \
	duk/duktape.c javascript-context.cpp \
	$(common_src)

pkt2gateway_LDADD = $(commonlibs) -lprotobuf -lglog -lunwind -lnanomsg $(SNMPLIBS)
pkt2gateway_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON

#
#	handlerpq
#
handlerpq_SOURCES= \
	handlerpq.cpp  handlerpq-config.cpp daemonize.cpp pqwriter.cpp errorcodes.cpp \
	pbjson.cpp pkt2.pb.cpp pkt2packetvariable.cpp pkt2optionscache.cpp fieldnamevalueindexstrings.cpp \
	messagedecomposer.cpp utilprotobuf.cpp error-printer.cpp protobuf-declarations.cpp \
	utilpriority.cpp utilfile.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp duk/duktape.c javascript-context.cpp \
	$(common_src)

handlerpq_LDADD = $(commonlibs) -lglog -lunwind -lprotobuf -lnanomsg $(SNMPLIBS)
handlerpq_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON -I/usr/include/postgresql

#
#	tcptransmitter
#
tcptransmitter_SOURCES = \
	tcptransmitter.cpp  tcptransmitter-config.cpp  \
	daemonize.cpp errorcodes.cpp \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)

tcptransmitter_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS)
tcptransmitter_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
#	handlerlmdb
#
handlerlmdb_SOURCES = \
	handlerlmdb.cpp lmdbwriter.cpp handlerlmdb-config.cpp \
	daemonize.cpp protobuf-declarations.cpp utilprotobuf.cpp error-printer.cpp \
	utilfile.cpp utilstring.cpp utilinet.cpp errorcodes.cpp \
	pbjson.cpp pkt2.pb.cpp pkt2packetvariable.cpp pkt2optionscache.cpp \
	messagedecomposer.cpp fieldnamevalueindexstrings.cpp duk/duktape.c javascript-context.cpp \
	$(common_src)
handlerlmdb_LDADD = -lprotobuf -largtable2 -lglog -llmdb -lnanomsg $(SNMPLIBS)
handlerlmdb_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON

#
#	handlerline
#
handlerline_SOURCES = \
	handlerline.cpp linewriter.cpp handlerline-config.cpp errorcodes.cpp \
	daemonize.cpp protobuf-declarations.cpp utilprotobuf.cpp error-printer.cpp \
	utilfile.cpp utilstring.cpp utilinet.cpp duk/duktape.c javascript-context.cpp \
	pkt2.pb.cpp pkt2optionscache.cpp pkt2packetvariable.cpp messagedecomposer.cpp fieldnamevalueindexstrings.cpp \
	pbjson.cpp \
	$(common_src)

handlerline_LDADD = -lprotobuf -largtable2 -lglog -llmdb -lnanomsg $(SNMPLIBS)
handlerline_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON

#
#	handler-google-sheets
#
handler_google_sheets_SOURCES = \
	handler-google-sheets.cpp google-sheets-writer.cpp handler-google-sheets-config.cpp \
	daemonize.cpp protobuf-declarations.cpp utilprotobuf.cpp error-printer.cpp \
	utilfile.cpp utilstring.cpp utilinet.cpp errorcodes.cpp \
	jsoncpp.cpp pbjson.cpp duk/duktape.c javascript-context.cpp \
	google-sheets.cpp sslhelper.cpp \
	pkt2.pb.cpp pkt2optionscache.cpp pkt2packetvariable.cpp messagedecomposer.cpp fieldnamevalueindexstrings.cpp \
	$(common_src)

handler_google_sheets_LDADD = -lprotobuf -largtable2 -lglog -llmdb -lnanomsg -lssl -lcrypto -lcurl $(SNMPLIBS)
handler_google_sheets_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON

#
#	protoc-gen-pkt2
#
protoc_gen_pkt2_SOURCES = \
	protoc-gen-pkt2.cpp pkt2_code_generator.cpp \
	utilstring.cpp utilinet.cpp pkt2.pb.cpp errorcodes.cpp \
	$(common_src)

protoc_gen_pkt2_LDADD = -lprotoc -lprotobuf $(SNMPLIBS)
protoc_gen_pkt2_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
#	message2gateway
#
message2gateway_SOURCES = \
	message2gateway.cpp message2gateway-impl.cpp message2gateway-config.cpp error-printer.cpp \
	daemonize.cpp utilstring.cpp utilinet.cpp utilfile.cpp protobuf-declarations.cpp \
	utilprotobuf.cpp packet2message.cpp errorcodes.cpp \
	pbjson.cpp pkt2.pb.cpp duk/duktape.c pkt2packetvariable.cpp pkt2optionscache.cpp messagecomposer.cpp \
	javascript-context.cpp \
	$(common_src)

message2gateway_LDADD = -lprotoc -lprotobuf -lglog -lnanomsg $(SNMPLIBS)
message2gateway_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON

#
#	example1message
#
example1message_SOURCES = \
	example1message.cpp utilprotobuf.cpp error-printer.cpp \
	example/example1.pb.cpp pkt2.pb.cpp utilfile.cpp utilstring.cpp protobuf-declarations.cpp \
	pbjson.cpp errorcodes.cpp \
	$(common_src)

example1message_LDADD = -lprotobuf -lglog $(SNMPLIBS)
example1message_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON

#
#	example1message1
#
example1message1_SOURCES = \
	example1message1.cpp example/example1.pb.cpp pkt2.pb.cpp utilsnmp.cpp errorcodes.cpp \
	$(common_src)

example1message1_LDADD = -lprotobuf $(SNMPLIBS)
example1message1_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
#	tcpemitter-example1
#
tcpemitter_example1_SOURCES = \
	tcpemitter-example1.cpp errorcodes.cpp \
	$(common_src)

tcpemitter_example1_LDADD = 
tcpemitter_example1_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
#	tcpemitter-iridium
#
tcpemitter_iridium_SOURCES = \
	tcpemitter-iridium.cpp utilstring.cpp errorcodes.cpp \
	$(common_src)

tcpemitter_iridium_LDADD = 
tcpemitter_iridium_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
#	mqtt-emitter-iridium
#
mqtt_emitter_iridium_SOURCES = \
	mqtt-emitter-iridium.cpp utilstring.cpp errorcodes.cpp \
	$(common_src)

mqtt_emitter_iridium_LDADD = -lpaho-mqtt3c 
mqtt_emitter_iridium_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
# pkt2dumppq
#
pkt2dumppq_SOURCES = \
	pkt2dumppq.cpp pqdumper.cpp pkt2dumppq-config.cpp daemonize.cpp input-packet.cpp errorcodes.cpp \
	pbjson.cpp pkt2.pb.cpp pkt2packetvariable.cpp pkt2optionscache.cpp fieldnamevalueindexstrings.cpp \
	messagedecomposer.cpp utilprotobuf.cpp error-printer.cpp protobuf-declarations.cpp \
	utilpriority.cpp utilfile.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp duk/duktape.c javascript-context.cpp \
	$(common_src)

pkt2dumppq_LDADD = $(commonlibs) -lglog -lunwind -lprotobuf -lnanomsg $(SNMPLIBS)
pkt2dumppq_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON -I/usr/include/postgresql

#
# pkt2receiver-check
#
pkt2receiver_check_SOURCES = \
	pkt2receiver-check.cpp pkt2receiver-check-config.cpp daemonize.cpp errorcodes.cpp \
	utilpriority.cpp utilfile.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)

pkt2receiver_check_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS)
pkt2receiver_check_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
# repeator
#
repeator_SOURCES = \
	repeator.cpp repeator-config.cpp daemonize.cpp errorcodes.cpp \
	utilpriority.cpp utilfile.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)

repeator_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS)
repeator_CPPFLAGS = $(COMMON_CPP_FLAGS)

#
# Configs, readme, CMake etc.
#
configdir = $(datadir)
dist_config_DATA = README.md HISTORY INSTALL \
	pkt2.js.nova pkt2.js.sample pkt2.js \
	CMakeLists.txt cmake/FindArgtable2.cmake cmake/FindGlog.cmake cmake/FindNanomsg.cmake \
	tools/mkdependencies tools/mkdocker tools/mktools tools/docker/ubuntu/Dockerfile tools/rebuild-nova \
	proto/pkt2.proto proto/google/protobuf/descriptor.proto \
	proto/iridium/gps16.proto proto/iridium/time5.proto proto/iridium/packet8.proto proto/example/example1.proto \
	proto/iridium/ie_ioheader.proto proto/iridium/ie_location.proto proto/iridium/animals.proto  \
	mib/EAS-IKFIA-MIB \
	wireshark/example/Makefile.am wireshark/example/Makefile.in	wireshark/example/irda-appl.h wireshark/example/moduleinfo.h \
	wireshark/example/packet-ircomm.c wireshark/example/packet-irda.c wireshark/example/packet-sir.c wireshark/example/plugin.c \
	wireshark/example/plugin.rc.in 
