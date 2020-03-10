SUBDIRS = . tests
ACLOCAL_AMFLAGS = -I m4

TESTS = tests/p1.sh tests/t1 

INCS = -I/usr/local/include -Ithird_party

HUFF_CPP_FLAGS = -Ihuff

PROTO_CPPFLAGS = -std=c++11

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
COMMON_CPP_FLAGS = -DENABLE_LOG $(INCS)

LIB_PROTOBUF = -L/usr/local/lib -lprotobuf

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
	pkt2sheet pkt2js
#	pkt2 mqtt-receiver tcpreceiver pkt2receiver pkt2gateway tcptransmitter tcpemitter messageemitter \
#	tcpemitter-example1 tcpemitter-iridium mqtt-emitter-iridium handlerpq pkt2receiver-check repeator \
#	freceiver pkt2js message2gateway protoc-gen-pkt2 example1message example1message1 handlerline \
#	handlerlmdb handler-google-sheets handler-fcm pkt2dumppq pkt2dumpfcm pkt2sheet

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
pkt2js-config.h pkt2js-impl.h str-pkt2.h pkt2sheet-config.h 

huff_src = huff/huffcode.cpp huff/huffmanmodifieddecoder.cpp huff/huffmanmodifiedencoder.cpp \
	huff/internalnode.cpp  huff/leafnode.cpp  huff/node.cpp  huff/devdecoder.cpp \
	third_party/bitstream/bitstream.cpp

common_src = third_party/argtable3/argtable3.c

commonlibs = -L/usr/local/lib/ -lpthread -ldl -L/usr/local/lib

#
#	pkt2
#
pkt2_SOURCES = \
	pkt2.cpp pkt2-config.cpp pkt2-impl.cpp errorcodes.cpp \
	utilstring.cpp daemonize.cpp third_party/duk/duktape.c \
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
	pkt2.pb.cpp utilfile.cpp third_party/duk/duktape.c javascript-context.cpp pbjson.cpp \
	$(common_src)

messageemitter_LDADD = $(commonlibs) -lglog $(LIB_PROTOBUF) -lunwind -lnanomsg $(SNMPLIBS)
messageemitter_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CPPFLAGS)

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
tcpreceiver_CPPFLAGS = $(COMMON_CPP_FLAGS) $(HUFF_CPP_FLAGS) $(PROTO_CPPFLAGS)

#
#	mqtt-receiver
#
mqtt_receiver_SOURCES = \
	mqtt-receiver.cpp   mqtt-receiver-config.cpp mqtt-receivernano.cpp input-packet.cpp \
	daemonize.cpp snmpagentpkt2.c get_rss.c errorcodes.cpp \
	utilpriority.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp \
	$(common_src)
mqtt_receiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg -lpaho-mqtt3c $(SNMPLIBS)
mqtt_receiver_CPPFLAGS = $(COMMON_CPP_FLAGS) $(PROTO_CPPFLAGS)
	
#
#	freceiver
#
freceiver_SOURCES = \
	freceiver.cpp freceiver-config.cpp freceiver-impl.cpp input-packet.cpp \
	daemonize.cpp snmpagentpkt2.c errorcodes.cpp \
	utilpriority.cpp utilstring.cpp utilinet.cpp \
	messageformat.cpp protobuf-declarations.cpp utilprotobuf.cpp error-printer.cpp \
	pbjson.cpp third_party/duk/duktape.c javascript-context.cpp packet2message.cpp utilfile.cpp \
	pkt2.pb.cpp pkt2optionscache.cpp pkt2packetvariable.cpp messagedecomposer.cpp messagecomposer.cpp fieldnamevalueindexstrings.cpp \
	$(common_src)

freceiver_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(LIB_PROTOBUF) $(SNMPLIBS)
freceiver_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CPPFLAGS)

#
#	pkt2receiver
#
pkt2receiver_SOURCES= \
	pkt2receiver.cpp  pkt2receiver-config.cpp pkt2receivernano.cpp input-packet.cpp \
	daemonize.cpp pbjson.cpp utilfile.cpp errorcodes.cpp \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp error-printer.cpp \
	packet2message.cpp messagecomposer.cpp third_party/duk/duktape.c pkt2packetvariable.cpp \
	pkt2optionscache.cpp utilprotobuf.cpp protobuf-declarations.cpp pkt2.pb.cpp \
	javascript-context.cpp \
	$(common_src)

pkt2receiver_LDADD = $(commonlibs) -lglog -lunwind $(LIB_PROTOBUF) -lnanomsg $(SNMPLIBS)
pkt2receiver_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CPPFLAGS)

#
#	pkt2gateway
#
pkt2gateway_SOURCES= \
	pkt2gateway.cpp  pkt2gateway-config.cpp packet2message.cpp \
	daemonize.cpp errorcodes.cpp \
	utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp \
	pkt2optionscache.cpp protobuf-declarations.cpp pkt2packetvariable.cpp pkt2.pb.cpp utilprotobuf.cpp error-printer.cpp utilfile.cpp pbjson.cpp \
	messagecomposer.cpp \
	third_party/duk/duktape.c javascript-context.cpp \
	$(common_src)

pkt2gateway_LDADD = $(commonlibs) $(LIB_PROTOBUF) -lglog -lunwind -lnanomsg $(SNMPLIBS)
pkt2gateway_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CPPFLAGS)

#
#	handlerpq
#
handlerpq_SOURCES= \
	handlerpq.cpp  handlerpq-config.cpp daemonize.cpp pqwriter.cpp errorcodes.cpp \
	pbjson.cpp pkt2.pb.cpp pkt2packetvariable.cpp pkt2optionscache.cpp fieldnamevalueindexstrings.cpp \
	messagedecomposer.cpp utilprotobuf.cpp error-printer.cpp protobuf-declarations.cpp \
	utilpriority.cpp utilfile.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp third_party/duk/duktape.c javascript-context.cpp \
	$(common_src)

handlerpq_LDADD = $(commonlibs) -lglog -lunwind $(LIB_PROTOBUF) -lnanomsg $(SNMPLIBS) -lpq
handlerpq_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON -I/usr/include/postgresql $(PROTO_CPPFLAGS)

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
	messagedecomposer.cpp fieldnamevalueindexstrings.cpp third_party/duk/duktape.c javascript-context.cpp \
	$(common_src)
handlerlmdb_LDADD = $(LIB_PROTOBUF) -lglog -llmdb -lnanomsg $(SNMPLIBS)
handlerlmdb_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CPPFLAGS)

#
#	handlerline
#
handlerline_SOURCES = \
	handlerline.cpp linewriter.cpp handlerline-config.cpp errorcodes.cpp \
	daemonize.cpp protobuf-declarations.cpp utilprotobuf.cpp error-printer.cpp \
	utilfile.cpp utilstring.cpp utilinet.cpp third_party/duk/duktape.c javascript-context.cpp \
	pkt2.pb.cpp pkt2optionscache.cpp pkt2packetvariable.cpp messagedecomposer.cpp fieldnamevalueindexstrings.cpp \
	pbjson.cpp \
	messageformat.cpp \
	$(common_src)

handlerline_LDADD = $(LIB_PROTOBUF) -lglog -lnanomsg $(SNMPLIBS)
handlerline_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CPPFLAGS)

#
#	handler-google-sheets
#
handler_google_sheets_SOURCES = \
	handler-google-sheets.cpp google-sheets-writer.cpp handler-google-sheets-config.cpp \
	daemonize.cpp protobuf-declarations.cpp utilprotobuf.cpp error-printer.cpp \
	utilfile.cpp utilstring.cpp utilinet.cpp errorcodes.cpp \
	jsoncpp.cpp pbjson.cpp third_party/duk/duktape.c javascript-context.cpp \
	google-sheets.cpp sslhelper.cpp \
	pkt2.pb.cpp pkt2optionscache.cpp pkt2packetvariable.cpp messagedecomposer.cpp fieldnamevalueindexstrings.cpp \
	$(common_src)

handler_google_sheets_LDADD = $(LIB_PROTOBUF) -lglog -llmdb -lnanomsg -lcurl $(SNMPLIBS) -L/usr/local/lib -lssl -lcrypto 
handler_google_sheets_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CPPFLAGS)

#
#	handler-fcm
#
handler_fcm_SOURCES = \
	handler-fcm.cpp fcm-writer.cpp handler-fcm-config.cpp \
	daemonize.cpp protobuf-declarations.cpp utilprotobuf.cpp error-printer.cpp \
	utilfile.cpp utilstring.cpp utilinet.cpp errorcodes.cpp \
	jsoncpp.cpp pbjson.cpp third_party/duk/duktape.c javascript-context.cpp \
	messageformat.cpp \
	helper_fcm.cpp \
	pkt2.pb.cpp pkt2optionscache.cpp pkt2packetvariable.cpp messagedecomposer.cpp fieldnamevalueindexstrings.cpp \
	$(common_src)

handler_fcm_LDADD = $(LIB_PROTOBUF) -lglog -lnanomsg -lssl -lcrypto -lcurl -lpq $(SNMPLIBS)
handler_fcm_CPPFLAGS = $(COMMON_CPP_FLAGS) -I /home/andrei/lib/firebase_cpp_sdk/include -D LIB_PBJSON -I/usr/include/postgresql $(PROTO_CPPFLAGS)

#
# pkt2dumpfcm
#
pkt2dumpfcm_SOURCES = \
	pkt2dumpfcm.cpp fcmdumper.cpp pkt2dumpfcm-config.cpp \
	daemonize.cpp input-packet.cpp errorcodes.cpp \
	pbjson.cpp pkt2.pb.cpp pkt2packetvariable.cpp pkt2optionscache.cpp fieldnamevalueindexstrings.cpp \
	messagedecomposer.cpp utilprotobuf.cpp error-printer.cpp protobuf-declarations.cpp \
	utilpriority.cpp utilfile.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp third_party/duk/duktape.c javascript-context.cpp \
	helper_fcm.cpp \
	$(common_src)

pkt2dumpfcm_LDADD = $(LIB_PROTOBUF) -lglog -lnanomsg -lssl -lcrypto -lcurl $(SNMPLIBS) -lpq
pkt2dumpfcm_CPPFLAGS = $(COMMON_CPP_FLAGS) -I /home/andrei/lib/firebase_cpp_sdk/include  -D LIB_PBJSON -I/usr/include/postgresql $(PROTO_CPPFLAGS)

#
#	protoc-gen-pkt2
#
protoc_gen_pkt2_SOURCES = \
	protoc-gen-pkt2.cpp pkt2_code_generator.cpp \
	utilstring.cpp utilinet.cpp pkt2.pb.cpp errorcodes.cpp \
	$(common_src)

protoc_gen_pkt2_LDADD = $(LIB_PROTOBUF) -lprotoc $(SNMPLIBS)
protoc_gen_pkt2_CPPFLAGS = $(COMMON_CPP_FLAGS) $(PROTO_CPPFLAGS)

#
#	message2gateway
#
message2gateway_SOURCES = \
	message2gateway.cpp message2gateway-impl.cpp message2gateway-config.cpp error-printer.cpp \
	daemonize.cpp utilstring.cpp utilinet.cpp utilfile.cpp protobuf-declarations.cpp \
	utilprotobuf.cpp packet2message.cpp errorcodes.cpp \
	pbjson.cpp pkt2.pb.cpp third_party/duk/duktape.c pkt2packetvariable.cpp pkt2optionscache.cpp messagecomposer.cpp \
	javascript-context.cpp \
	$(common_src)

message2gateway_LDADD = $(LIB_PROTOBUF) -lprotoc -lglog -lnanomsg $(SNMPLIBS)
message2gateway_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CPPFLAGS)

#
#	example1message
#
example1message_SOURCES = \
	example1message.cpp utilprotobuf.cpp error-printer.cpp \
	example/example1.pb.cpp pkt2.pb.cpp utilfile.cpp utilstring.cpp protobuf-declarations.cp~/Android/p \
	pbjson.cpp errorcodes.cpp protobuf-declarations.cpp \
	$(common_src)

example1message_LDADD = $(LIB_PROTOBUF) -lglog $(SNMPLIBS)
example1message_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CPPFLAGS)

#
#	example1message1
#
example1message1_SOURCES = \
	example1message1.cpp example/example1.pb.cpp pkt2.pb.cpp utilsnmp.cpp errorcodes.cpp \
	$(common_src)

example1message1_LDADD = $(LIB_PROTOBUF) $(SNMPLIBS)
example1message1_CPPFLAGS = $(COMMON_CPP_FLAGS) $(PROTO_CPPFLAGS)

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
	utilpriority.cpp utilfile.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp third_party/duk/duktape.c javascript-context.cpp \
	$(common_src)

pkt2dumppq_LDADD = $(commonlibs) -lglog -lunwind $(LIB_PROTOBUF) -lnanomsg $(SNMPLIBS) -lpq
pkt2dumppq_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON -I/usr/include/postgresql $(PROTO_CPPFLAGS)

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
	utilpriority.cpp utilfile.cpp utilstring.cpp utilinet.cpp NanoMessage.cpp \~/Android/

repeator_LDADD = $(commonlibs) -lglog -lunwind -lnanomsg $(SNMPLIBS)
repeator_CPPFLAGS = $(COMMON_CPP_FLAGS)

# input-packet.cpp errorcodes.cpp 
PKT2LIBSRC = \
	pbjson.cpp packet2message.cpp pkt2packetvariable.cpp javascript-context.cpp error-printer.cpp \
	messageformat.cpp messagecomposer.cpp messagedecomposer.cpp fieldnamevalueindexstrings.cpp \
	pkt2optionscache.cpp protobuf-declarations.cpp pkt2.pb.cpp \
	utilfile.cpp utilstring.cpp utilprotobuf.cpp third_party/duk/duktape.c \
	$(common_src)

#
# pkt2js
#
pkt2js_SOURCES = \
	pkt2js.cpp  pkt2js-config.cpp pkt2js-impl.cpp daemonize.cpp str-pkt2.cpp \
	$(PKT2LIBSRC)

pkt2js_LDADD = -lglog -lunwind $(LIB_PROTOBUF)
pkt2js_CPPFLAGS = $(COMMON_CPP_FLAGS) -DHAVE_PTHREAD=1 -DHAVE_ZLIB=1 -DLIB_PBJSON=1 $(PROTO_CPPFLAGS) -Ithird_party/protobuf

#
#	pkt2sheet
#
pkt2sheet_SOURCES = \
	pkt2sheet.cpp pkt2sheet-config.cpp str-pkt2.cpp packet2message.cpp messageformat.cpp \
	daemonize.cpp protobuf-declarations.cpp utilprotobuf.cpp error-printer.cpp \
	utilfile.cpp utilstring.cpp utilinet.cpp errorcodes.cpp messagecomposer.cpp \
	jsoncpp.cpp pbjson.cpp third_party/duk/duktape.c javascript-context.cpp \
	google-sheets.cpp sslhelper.cpp google-sheets-writer-from-file.cpp \
	pkt2.pb.cpp pkt2optionscache.cpp pkt2packetvariable.cpp messagedecomposer.cpp fieldnamevalueindexstrings.cpp \
	$(common_src)

pkt2sheet_LDADD = -lglog -lunwind $(LIB_PROTOBUF) -lcurl -L/usr/local/lib -lssl -lcrypto 
pkt2sheet_CPPFLAGS = $(COMMON_CPP_FLAGS) -D LIB_PBJSON $(PROTO_CPPFLAGS)

#
#	Static library
#
lib_LIBRARIES = libpkt2.a
libpkt2_a_SOURCES = str-pkt2.cpp $(PKT2LIBSRC)
libpkt2_a_CPPFLAGS = -D LIB_PBJSON $(PROTO_CPPFLAGS) -I$(INCS)
#libpkt2_a_LDFLAGS = libpkt2.a
#libpkt2_a_CPPFLAGS = -I.

#
#	Shared library
#
#lib_LTLIBRARIES = libpkt2.la
#libpkt2_la_SOURCES = $(common_src)
#libpkt2_la_LDFLAGS = libpkt2.a
#libpkt2_la_CPPFLAGS = -I.

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
