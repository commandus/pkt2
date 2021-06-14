SUBDIRS = . tests
ACLOCAL_AMFLAGS = -I m4

TESTS = tests/p1.sh tests/t1 

INCS = -I/usr/local/include -Ithird_party

HUFF_CPP_FLAGS = -Ihuff

PROTO_CXXFLAGS = -std=c++11

if DEBUG
	AM_CFLAGS = -g3 -O0 $(INCS)
	AM_CXXFLAGS = -g3 -O0 -DDEBUG $(INCS)
else
	AM_CFLAGS = -O2 $(INCS)
	AM_CXXFLAGS = -O2 $(INCS)
endif

# ----------------- REMOVE IT!!!! ------------------
# -std=c++11
#AM_CXXFLAGS =  -O0 -DDEBUG $(INCS)
#COMMON_CPP_FLAGS = 
# ----------------- REMOVE IT!!!! ------------------

# PROTOBUF_OLD_GCC_COMPAT = -D_GLIBCXX_USE_CXX11_ABI=0
PROTOBUF_OLD_GCC_COMPAT =
# OSX_COMPAT_LIB = -L/usr/local/lib
OSX_COMPAT_LIB =
COMMON_CPP_FLAGS = -DENABLE_LOG $(INCS) $(PROTOBUF_OLD_GCC_COMPAT)

LIB_PROTOBUF = $(OSX_COMPAT_LIB) -lprotobuf

if ENABLE_SNMP
# SNMPLIBS = `net-snmp-config --agent-libs`
	SNMPLIBS = -lnetsnmpagent -lnetsnmp -lnetsnmphelpers -lnetsnmpmibs -lpthread
	SNMP_SRC = snmpagentpkt2.c
else
	SNMPLIBS = 
	SNMP_SRC =
endif

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
	rm -rf google/

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
bin_PROGRAMS = \
	pkt2 tcpreceiver tcptransmitter tcpemitter tcpemitter-example1 \
	tcpemitter-iridium pkt2receiver-check repeator messageemitter pkt2receiver \
	handlerline handlerlmdb handler-fcm pkt2dumppq pkt2dumpfcm \
	pkt2gateway handlerpq freceiver pkt2js \
	message2gateway protoc-gen-pkt2 example1message example1message1 \
	js2sheet handler-google-sheets pkt2sheet

if ENABLE_MQTT
	bin_PROGRAMS += mqtt-receiver mqtt-emitter-iridium
endif

# Static library used for buil binaries, so build library first.
$(bin_PROGRAMS): $(lib_LIBRARIES)

#
#	Headers
#
nobase_dist_include_HEADERS = \
NanoMessage.h        cpp-syslog.h    daemonize.h   ieee754.h    platform.h  packet2message.h \
utilpriority.h       utilstring.h    utilinet.h    bin2ascii.h  utilprotobuf.h utilfile.h \
protoc-gen-pkt2.h    pkt2_code_generator.h snmpagentpkt2.h get_rss.h protobuf-declarations.h \
tcpreceiver-config.h pkt2receiver-config.h pkt2gateway-config.h handlerpq-config.h tcptransmitter-config.h \
message2gateway-config.h helper_socket.h handlerlmdb-config.h lmdbwriter.h error-printer.h \
pkt2receivernano.h output-message.h tcpemitter-config.h tcpreceivernano.h input-packet.h utilsnmp.h \
pkt2packetvariable.h messageemitter-config.h linewriter.h handlerline-config.h messagedecomposer.h \
messagecomposer.h fieldnamevalueindexstrings.h sslhelper.h iridium.h pbjson.hpp errorcodes.h \
pkt2optionscache.h pqwriter.h tcpemitterline.h google-sheets.h \
third_party/rapidjson/allocators.h           third_party/rapidjson/encodings.h        third_party/rapidjson/fwd.h             third_party/rapidjson/memorystream.h    third_party/rapidjson/prettywriter.h  third_party/rapidjson/schema.h        third_party/rapidjson/writer.h \
third_party/rapidjson/document.h             third_party/rapidjson/filereadstream.h   third_party/rapidjson/istreamwrapper.h  third_party/rapidjson/ostreamwrapper.h  third_party/rapidjson/rapidjson.h     third_party/rapidjson/stream.h \
third_party/rapidjson/encodedstream.h        third_party/rapidjson/filewritestream.h  third_party/rapidjson/memorybuffer.h    third_party/rapidjson/pointer.h         third_party/rapidjson/reader.h        third_party/rapidjson/stringbuffer.h \
third_party/rapidjson/error/en.h             third_party/rapidjson/error/error.h \
third_party/rapidjson/internal/biginteger.h  third_party/rapidjson/internal/dtoa.h     third_party/rapidjson/internal/itoa.h  third_party/rapidjson/internal/pow10.h  third_party/rapidjson/internal/stack.h    third_party/rapidjson/internal/strtod.h \
third_party/rapidjson/internal/diyfp.h       third_party/rapidjson/internal/ieee754.h  third_party/rapidjson/internal/meta.h  third_party/rapidjson/internal/regex.h  third_party/rapidjson/internal/strfunc.h  third_party/rapidjson/internal/swap.h \
third_party/rapidjson/msinttypes/inttypes.h  third_party/rapidjson/msinttypes/stdint.h \
third_party/duk/duk_config.h  third_party/duk/duktape.h  javascript-context.h \
handler-google-sheets-config.h google-sheets-writer.h google-sheets-writer-from-file.h pqdumper.h pkt2dumppq-config.h \
mqtt-receivernano.h mqtt-receiver-config.h pkt2-config.h pkt2-impl.h \
pkt2receiver-check-config.h repeator-config.h freceiver-impl.h freceiver-impl.h freceiver-config.h \
huff/huffcode.h  huff/huffmanmodifieddecoder.h huff/huffmanmodifiedencoder.h \
huff/internalnode.h  huff/leafnode.h  huff/node.h huff/devdecoder.h \
third_party/bitstream/bitstream.h third_party/bitstream/varint.h \
messageformat.h messageemitterline.h pkt2receiver-check.h handler-fcm-config.h fcm-writer.h \
fcmdumper.h pkt2dumpfcm-config.h helper_fcm.h third_party/argtable3/argtable3.h \
pkt2js-config.h pkt2js-impl.h str-pkt2.h pkt2sheet-config.h js2sheet-config.h google-sheets-writer-from-js.h \
env-pkt2.h packet-protobuf-message.h database-config.h pg-connect.h

huff_src = huff/huffcode.cpp huff/huffmanmodifieddecoder.cpp huff/huffmanmodifiedencoder.cpp \
	huff/internalnode.cpp  huff/leafnode.cpp  huff/node.cpp  huff/devdecoder.cpp \
	third_party/bitstream/bitstream.cpp

common_src = third_party/argtable3/argtable3.c

commonlibs = -L/usr/local/lib/ -lpthread -ldl -L/usr/local/lib

LIBS_PKT2 = -L. -lpkt2 -lpkt2util

PKT2LIBSRC = \
	pbjson.cpp packet2message.cpp pkt2packetvariable.cpp javascript-context.cpp error-printer.cpp \
	messageformat.cpp messagecomposer.cpp messagedecomposer.cpp fieldnamevalueindexstrings.cpp \
	pkt2optionscache.cpp protobuf-declarations.cpp pkt2.pb.cpp \
	utilfile.cpp utilstring.cpp utilprotobuf.cpp third_party/duk/duktape.c \
	env-pkt2.cpp packet-protobuf-message.cpp database-config.cpp

PKT2LIBUTILSRC = \
	errorcodes.cpp daemonize.cpp \
	jsoncpp.cpp \
	utilpriority.cpp utilinet.cpp \
	NanoMessage.cpp input-packet.cpp \
	google-sheets.cpp sslhelper.cpp helper_fcm.cpp \
	pg-connect.cpp

#
#	pkt2
#
pkt2_SOURCES = \
	pkt2.cpp pkt2-config.cpp pkt2-impl.cpp \
	$(common_src)

pkt2_LDADD = $(commonlibs) -lglog $(SNMPLIBS) $(LIBS_PKT2)
pkt2_CXXFLAGS = $(COMMON_CPP_FLAGS)

#
#	tcpemitter
#
tcpemitter_SOURCES = \
	tcpemitter.cpp tcpemitter-config.cpp tcpemitterline.cpp \
	$(common_src)

tcpemitter_LDADD = $(commonlibs) -lglog $(SNMPLIBS) $(LIBS_PKT2)
tcpemitter_CXXFLAGS = $(COMMON_CPP_FLAGS)

#
#	messageemitter
#
messageemitter_SOURCES = \
	messageemitter.cpp messageemitter-config.cpp messageemitterline.cpp \
	$(common_src)

messageemitter_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS) $(LIBS_PKT2) $(LIB_PROTOBUF)
messageemitter_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	tcpreceiver
#
tcpreceiver_SOURCES = \
	tcpreceiver.cpp tcpreceiver-config.cpp tcpreceivernano.cpp get_rss.c \
	$(SNMP_SRC) \
	$(huff_src) \
	$(common_src)

tcpreceiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS) $(LIBS_PKT2)
tcpreceiver_CXXFLAGS = $(COMMON_CPP_FLAGS) $(HUFF_CPP_FLAGS) $(PROTO_CXXFLAGS)

#
#	mqtt-receiver
#
mqtt_receiver_SOURCES = \
	mqtt-receiver.cpp  mqtt-receiver-config.cpp mqtt-receivernano.cpp get_rss.c \
	$(SNMP_SRC) \
	$(common_src)
mqtt_receiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg -lpaho-mqtt3c $(SNMPLIBS) $(LIBS_PKT2)
mqtt_receiver_CXXFLAGS = $(COMMON_CPP_FLAGS) $(PROTO_CXXFLAGS)
	
#
#	freceiver
#
freceiver_SOURCES = \
	freceiver.cpp freceiver-config.cpp freceiver-impl.cpp \
	$(SNMP_SRC) \
	$(common_src)

freceiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS) $(LIBS_PKT2) $(LIB_PROTOBUF) 
freceiver_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	pkt2receiver
#
pkt2receiver_SOURCES= \
	pkt2receiver.cpp  pkt2receiver-config.cpp pkt2receivernano.cpp \
	$(common_src)

pkt2receiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS) $(LIBS_PKT2) $(LIB_PROTOBUF)
pkt2receiver_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	pkt2gateway
#
pkt2gateway_SOURCES= \
	pkt2gateway.cpp  pkt2gateway-config.cpp  \
	$(common_src)

pkt2gateway_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS) $(LIBS_PKT2) $(LIB_PROTOBUF)
pkt2gateway_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	handlerpq
#
handlerpq_SOURCES= \
	handlerpq.cpp  handlerpq-config.cpp pqwriter.cpp pg-connect.cpp \
	$(common_src)

handlerpq_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS) -lpq $(LIBS_PKT2) $(LIB_PROTOBUF)
handlerpq_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON -I/usr/include/postgresql $(PROTO_CXXFLAGS)

#
#	tcptransmitter
#
tcptransmitter_SOURCES = \
	tcptransmitter.cpp  tcptransmitter-config.cpp \
	$(common_src)

tcptransmitter_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS) $(LIBS_PKT2)
tcptransmitter_CXXFLAGS = $(COMMON_CPP_FLAGS) 

#
#	handlerlmdb
#
handlerlmdb_SOURCES = \
	handlerlmdb.cpp lmdbwriter.cpp handlerlmdb-config.cpp \
	$(common_src)
handlerlmdb_LDADD = -lglog -llmdb -lnanomsg $(SNMPLIBS) $(LIBS_PKT2) $(LIB_PROTOBUF)
handlerlmdb_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	handlerline
#
handlerline_SOURCES = \
	handlerline.cpp linewriter.cpp handlerline-config.cpp \
	$(common_src)

handlerline_LDADD = -lglog -lnanomsg $(SNMPLIBS) $(LIBS_PKT2) $(LIB_PROTOBUF)
handlerline_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	handler-google-sheets
#
handler_google_sheets_SOURCES = \
	handler-google-sheets.cpp google-sheets-writer.cpp handler-google-sheets-config.cpp \
	$(common_src)

handler_google_sheets_LDADD = -lglog -lnanomsg -lcurl $(LIBS_PKT2) $(LIB_PROTOBUF) -lssl -lcrypto
handler_google_sheets_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	handler-fcm
#
handler_fcm_SOURCES = \
	handler-fcm.cpp fcm-writer.cpp handler-fcm-config.cpp \
	$(common_src)

handler_fcm_LDADD = -lglog -lnanomsg -lssl -lcrypto -lcurl -lpq $(SNMPLIBS) $(LIBS_PKT2) $(LIB_PROTOBUF)
handler_fcm_CXXFLAGS = $(COMMON_CPP_FLAGS) -I /home/andrei/lib/firebase_cpp_sdk/include -D LIB_PBJSON -I/usr/include/postgresql $(PROTO_CXXFLAGS)

#
# pkt2dumpfcm
#
pkt2dumpfcm_SOURCES = \
	pkt2dumpfcm.cpp fcmdumper.cpp pkt2dumpfcm-config.cpp \
	$(common_src)

pkt2dumpfcm_LDADD = -lglog -lnanomsg -lssl -lcrypto -lcurl $(SNMPLIBS) -lpq $(LIBS_PKT2) $(LIB_PROTOBUF)
pkt2dumpfcm_CXXFLAGS = $(COMMON_CPP_FLAGS) -I /home/andrei/lib/firebase_cpp_sdk/include  -D LIB_PBJSON -I/usr/include/postgresql $(PROTO_CXXFLAGS)

#
#	protoc-gen-pkt2
#
protoc_gen_pkt2_SOURCES = \
	protoc-gen-pkt2.cpp pkt2_code_generator.cpp \
	$(common_src)

protoc_gen_pkt2_LDADD = -lprotoc $(SNMPLIBS) $(LIBS_PKT2) $(LIB_PROTOBUF)
protoc_gen_pkt2_CXXFLAGS = $(COMMON_CPP_FLAGS) $(PROTO_CXXFLAGS)

#
#	message2gateway
#
message2gateway_SOURCES = \
	message2gateway.cpp message2gateway-impl.cpp message2gateway-config.cpp \
	$(common_src)

message2gateway_LDADD = -lprotoc -lglog -lnanomsg $(SNMPLIBS) $(LIBS_PKT2) $(LIB_PROTOBUF)
message2gateway_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	example1message
#
example1message_SOURCES = \
	example1message.cpp example/example1.pb.cpp \
	$(common_src)

example1message_LDADD = -lglog $(LIBS_PKT2) $(LIB_PROTOBUF)
example1message_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	example1message1
#
example1message1_SOURCES = \
	example1message1.cpp example/example1.pb.cpp \
	$(common_src)

example1message1_LDADD = $(LIB_PROTOBUF) $(LIBS_PKT2) $(SNMPLIBS)
example1message1_CXXFLAGS = $(COMMON_CPP_FLAGS) $(PROTO_CXXFLAGS)

#
#	tcpemitter-example1
#
tcpemitter_example1_SOURCES = \
	tcpemitter-example1.cpp  \
	$(common_src)

tcpemitter_example1_LDADD = $(LIBS_PKT2) 
tcpemitter_example1_CXXFLAGS = $(COMMON_CPP_FLAGS)

#
#	tcpemitter-iridium
#
tcpemitter_iridium_SOURCES = \
	tcpemitter-iridium.cpp \
	$(common_src)

tcpemitter_iridium_LDADD = $(LIBS_PKT2)
tcpemitter_iridium_CXXFLAGS = $(COMMON_CPP_FLAGS)

#
#	mqtt-emitter-iridium
#
mqtt_emitter_iridium_SOURCES = \
	mqtt-emitter-iridium.cpp \
	$(common_src)

mqtt_emitter_iridium_LDADD = -lpaho-mqtt3c $(LIBS_PKT2) 
mqtt_emitter_iridium_CXXFLAGS = $(COMMON_CPP_FLAGS)

#
# pkt2dumppq
#
pkt2dumppq_SOURCES = \
	pkt2dumppq.cpp pqdumper.cpp pkt2dumppq-config.cpp \
	$(common_src)

pkt2dumppq_LDADD = $(commonlibs) -lglog -lunwind $(LIB_PROTOBUF) $(LIBS_PKT2) -lnanomsg $(SNMPLIBS) -lpq
pkt2dumppq_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON -I/usr/include/postgresql $(PROTO_CXXFLAGS)

#
# pkt2receiver-check
#
pkt2receiver_check_SOURCES = \
	pkt2receiver-check.cpp pkt2receiver-check-config.cpp \
	$(common_src)

pkt2receiver_check_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS) $(LIBS_PKT2)
pkt2receiver_check_CXXFLAGS = $(COMMON_CPP_FLAGS)

#
# repeator
#
repeator_SOURCES = \
	repeator.cpp repeator-config.cpp \
	$(common_src)

repeator_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS) $(LIBS_PKT2)
repeator_CXXFLAGS = $(COMMON_CPP_FLAGS)

#
# pkt2js
#
pkt2js_SOURCES = \
	pkt2js.cpp pkt2js-config.cpp pkt2js-impl.cpp \
	$(PKT2LIBSRC) $(common_src)

pkt2js_LDADD = -lglog -lunwind $(LIB_PROTOBUF) $(LIBS_PKT2)
pkt2js_CXXFLAGS = $(COMMON_CPP_FLAGS) -DHAVE_PTHREAD=1 -DHAVE_ZLIB=1 -DLIB_PBJSON=1 $(PROTO_CXXFLAGS) -Ithird_party/protobuf

#
#	pkt2sheet
#
pkt2sheet_SOURCES = \
	pkt2sheet.cpp pkt2sheet-config.cpp google-sheets-writer-from-file.cpp \
	$(common_src)

pkt2sheet_LDADD = -lglog -lunwind $(LIBS_PKT2) $(LIB_PROTOBUF) -lcurl -lssl -lcrypto 
pkt2sheet_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	js2sheet
#
js2sheet_SOURCES = \
	js2sheet.cpp js2sheet-config.cpp google-sheets-writer-from-js.cpp \
	$(common_src)

js2sheet_LDADD = -lglog -lunwind $(LIB_PROTOBUF) $(LIBS_PKT2) -lcurl -lcrypto 
js2sheet_CXXFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CXXFLAGS)

#
#	Static library
#
lib_LIBRARIES = libpkt2.a libpkt2util.a
libpkt2_a_SOURCES = str-pkt2.cpp $(PKT2LIBSRC)
libpkt2_a_CXXFLAGS = -D LIB_PBJSON $(PROTO_CXXFLAGS) -I$(INCS) -fPIC $(PROTOBUF_OLD_GCC_COMPAT)

libpkt2util_a_SOURCES = $(PKT2LIBUTILSRC)
libpkt2util_a_CXXFLAGS = -I/usr/include/postgresql -I$(INCS) -fPIC $(PROTOBUF_OLD_GCC_COMPAT)

#
#	Shared library
#
#lib_LTLIBRARIES = libpkt2.la
#libpkt2_la_SOURCES = $(common_src)
#libpkt2_la_LDFLAGS = libpkt2.a
#libpkt2_la_CXXFLAGS = -I.

#
# Configs, readme, CMake etc.
#
configdir = $(datadir)
dist_config_DATA = README.md HISTORY INSTALL \
	pkt2.js.sample pkt2.js \
	CMakeLists.txt cmake/FindArgtable2.cmake cmake/FindGlog.cmake cmake/FindNanomsg.cmake \
	tools/mkdependencies tools/mkdocker tools/mktools tools/docker/ubuntu/Dockerfile tools/rebuild-nova \
	proto/pkt2.proto proto/google/protobuf/descriptor.proto \
	proto/iridium/gps16.proto proto/iridium/time5.proto proto/iridium/packet8.proto proto/example/example1.proto \
	proto/iridium/ie_ioheader.proto proto/iridium/ie_location.proto proto/iridium/animals.proto  \
	mib/EAS-IKFIA-MIB \
	wireshark/example/Makefile.am wireshark/example/Makefile.in	wireshark/example/irda-appl.h wireshark/example/moduleinfo.h \
	wireshark/example/packet-ircomm.c wireshark/example/packet-irda.c wireshark/example/packet-sir.c wireshark/example/plugin.c \
	wireshark/example/plugin.rc.in 
