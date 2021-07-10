cmake_minimum_required(VERSION 2.8)

project(pkt2)
set(pkt2_VERSION_MAJOR 0)
set(pkt2_VERSION_MINOR 1)

set(CMAKE_TOOLCHAIN_FILE "D:/bin/scripts/buildsystems/vcpkg.cmake")

#
# make link to /home/andrei/lib/grpc/third_party/protobuf/src
# ln -s /home/andrei/lib/grpc/third_party/protobuf/src protobuf
#
# set(PROTOBUF_ROOT third_party/protobuf)
# set(PROTOBUF_ROOT /home/andrei/lib/grpc/third_party/protobuf/src)
set(PROTOBUF_ROOT /home/andrei/lib/grpc/third_party/protobuf/src)
#set(PROTOBUF_ROOT "D:/bin/buildtrees/protobuf/src/a5c431386a-c9deea9e31.clean/src")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
                  
set(LIBS ${LIBS})

#find_package(protobuf CONFIG REQUIRED)
#target_link_libraries(main PRIVATE protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)

#find_package(Argtable3 CONFIG REQUIRED)
#target_link_libraries(main PRIVATE argtable3 argtable3_static)

#find_package(nanomsg CONFIG REQUIRED)
#target_link_libraries(main PRIVATE nanomsg)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/pkt2.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/pkt2.pb.h
	COMMAND protoc -I proto --cpp_out=. proto/pkt2.proto
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# set(CC /usr/bin/clang)
# set(CXX /usr/bin/clang+)

include_directories(. ${PROTOBUF_ROOT} third_party )

set(PROTO_LITE_SRC
  ${PROTOBUF_ROOT}/google/protobuf/stubs/bytestream.cc                          
  ${PROTOBUF_ROOT}/google/protobuf/stubs/bytestream.h                           
  ${PROTOBUF_ROOT}/google/protobuf/stubs/common.cc                              
  ${PROTOBUF_ROOT}/google/protobuf/stubs/hash.h                                 
  ${PROTOBUF_ROOT}/google/protobuf/stubs/int128.cc                              
  ${PROTOBUF_ROOT}/google/protobuf/stubs/int128.h                               
  ${PROTOBUF_ROOT}/google/protobuf/io/io_win32.cc                               
  ${PROTOBUF_ROOT}/google/protobuf/io/io_win32.h                                
  ${PROTOBUF_ROOT}/google/protobuf/stubs/map_util.h                             
  ${PROTOBUF_ROOT}/google/protobuf/stubs/mathutil.h                             
  ${PROTOBUF_ROOT}/google/protobuf/stubs/status.cc                              
  ${PROTOBUF_ROOT}/google/protobuf/stubs/status.h                               
  ${PROTOBUF_ROOT}/google/protobuf/stubs/status_macros.h                        
  ${PROTOBUF_ROOT}/google/protobuf/stubs/statusor.cc                            
  ${PROTOBUF_ROOT}/google/protobuf/stubs/statusor.h                             
  ${PROTOBUF_ROOT}/google/protobuf/stubs/stringpiece.cc                         
  ${PROTOBUF_ROOT}/google/protobuf/stubs/stringpiece.h                          
  ${PROTOBUF_ROOT}/google/protobuf/stubs/stringprintf.cc                        
  ${PROTOBUF_ROOT}/google/protobuf/stubs/stringprintf.h                         
  ${PROTOBUF_ROOT}/google/protobuf/stubs/structurally_valid.cc                  
  ${PROTOBUF_ROOT}/google/protobuf/stubs/strutil.cc                             
  ${PROTOBUF_ROOT}/google/protobuf/stubs/time.cc                                
  ${PROTOBUF_ROOT}/google/protobuf/stubs/time.h                                 
  ${PROTOBUF_ROOT}/google/protobuf/any_lite.cc                                  
  ${PROTOBUF_ROOT}/google/protobuf/arena.cc                                     
  ${PROTOBUF_ROOT}/google/protobuf/extension_set.cc                             
  ${PROTOBUF_ROOT}/google/protobuf/generated_message_util.cc                    
  ${PROTOBUF_ROOT}/google/protobuf/generated_message_table_driven_lite.h        
  ${PROTOBUF_ROOT}/google/protobuf/generated_message_table_driven_lite.cc       
  ${PROTOBUF_ROOT}/google/protobuf/implicit_weak_message.cc                     
  ${PROTOBUF_ROOT}/google/protobuf/message_lite.cc                              
  ${PROTOBUF_ROOT}/google/protobuf/parse_context.cc                             
  ${PROTOBUF_ROOT}/google/protobuf/repeated_field.cc                            
  ${PROTOBUF_ROOT}/google/protobuf/wire_format_lite.cc                          
  ${PROTOBUF_ROOT}/google/protobuf/io/coded_stream.cc                           
  ${PROTOBUF_ROOT}/google/protobuf/io/strtod.cc                                 
  ${PROTOBUF_ROOT}/google/protobuf/io/zero_copy_stream.cc                       
  ${PROTOBUF_ROOT}/google/protobuf/io/zero_copy_stream_impl_lite.cc
 # ${PROTOBUF_ROOT}/google/protobuf/io/coded_stream_inl.h                        
 )

set(PROTO_SRC
  ${PROTO_LITE_SRC}
  ${PROTOBUF_ROOT}/google/protobuf/any.pb.cc                                    
  ${PROTOBUF_ROOT}/google/protobuf/api.pb.cc                                    
  ${PROTOBUF_ROOT}/google/protobuf/any.cc                                       
  ${PROTOBUF_ROOT}/google/protobuf/descriptor.cc                                
  ${PROTOBUF_ROOT}/google/protobuf/descriptor_database.cc                       
  ${PROTOBUF_ROOT}/google/protobuf/descriptor.pb.cc                             
  ${PROTOBUF_ROOT}/google/protobuf/duration.pb.cc                               
  ${PROTOBUF_ROOT}/google/protobuf/dynamic_message.cc                           
  ${PROTOBUF_ROOT}/google/protobuf/empty.pb.cc                                  
  ${PROTOBUF_ROOT}/google/protobuf/extension_set_heavy.cc                       
  ${PROTOBUF_ROOT}/google/protobuf/field_mask.pb.cc                             
  ${PROTOBUF_ROOT}/google/protobuf/generated_message_reflection.cc              
  ${PROTOBUF_ROOT}/google/protobuf/generated_message_table_driven_lite.h        
  ${PROTOBUF_ROOT}/google/protobuf/generated_message_table_driven.cc            
  ${PROTOBUF_ROOT}/google/protobuf/map_field.cc                                 
  ${PROTOBUF_ROOT}/google/protobuf/message.cc                                   
  ${PROTOBUF_ROOT}/google/protobuf/reflection_internal.h                        
  ${PROTOBUF_ROOT}/google/protobuf/reflection_ops.cc                            
  ${PROTOBUF_ROOT}/google/protobuf/service.cc                                   
  ${PROTOBUF_ROOT}/google/protobuf/source_context.pb.cc                         
  ${PROTOBUF_ROOT}/google/protobuf/struct.pb.cc                                 
  ${PROTOBUF_ROOT}/google/protobuf/stubs/substitute.cc                          
  ${PROTOBUF_ROOT}/google/protobuf/stubs/substitute.h                           
  ${PROTOBUF_ROOT}/google/protobuf/text_format.cc                               
  ${PROTOBUF_ROOT}/google/protobuf/timestamp.pb.cc                              
  ${PROTOBUF_ROOT}/google/protobuf/type.pb.cc                                   
  ${PROTOBUF_ROOT}/google/protobuf/unknown_field_set.cc                         
  ${PROTOBUF_ROOT}/google/protobuf/wire_format.cc                               
  ${PROTOBUF_ROOT}/google/protobuf/wrappers.pb.cc                               
  ${PROTOBUF_ROOT}/google/protobuf/io/gzip_stream.cc                            
  ${PROTOBUF_ROOT}/google/protobuf/io/printer.cc                                
  ${PROTOBUF_ROOT}/google/protobuf/io/tokenizer.cc                              
  ${PROTOBUF_ROOT}/google/protobuf/io/zero_copy_stream_impl.cc                  
  ${PROTOBUF_ROOT}/google/protobuf/compiler/importer.cc                         
  ${PROTOBUF_ROOT}/google/protobuf/compiler/parser.cc                           
  ${PROTOBUF_ROOT}/google/protobuf/util/delimited_message_util.cc               
  ${PROTOBUF_ROOT}/google/protobuf/util/field_comparator.cc                     
  ${PROTOBUF_ROOT}/google/protobuf/util/field_mask_util.cc                      
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/constants.h                    
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/datapiece.cc                   
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/datapiece.h                    
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/default_value_objectwriter.cc  
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/default_value_objectwriter.h   
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/error_listener.cc              
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/error_listener.h               
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/expecting_objectwriter.h       
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/field_mask_utility.cc          
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/field_mask_utility.h           
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/json_escaping.cc               
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/json_escaping.h                
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/json_objectwriter.cc           
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/json_objectwriter.h            
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/json_stream_parser.cc          
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/json_stream_parser.h           
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/location_tracker.h             
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/mock_error_listener.h          
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/object_location_tracker.h      
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/object_source.h                
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/object_writer.cc               
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/object_writer.h                
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/protostream_objectsource.cc    
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/protostream_objectsource.h     
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/protostream_objectwriter.cc    
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/protostream_objectwriter.h     
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/proto_writer.cc                
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/proto_writer.h                 
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/structured_objectwriter.h      
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/type_info.cc                   
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/type_info.h
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/type_info_test_helper.cc       
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/type_info_test_helper.h        
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/utility.cc                     
  ${PROTOBUF_ROOT}/google/protobuf/util/internal/utility.h                      
  ${PROTOBUF_ROOT}/google/protobuf/util/json_util.cc                            
  ${PROTOBUF_ROOT}/google/protobuf/util/message_differencer.cc                  
  ${PROTOBUF_ROOT}/google/protobuf/util/time_util.cc                            
  ${PROTOBUF_ROOT}/google/protobuf/util/type_resolver_util.cc
#  ${PROTOBUF_ROOT}/google/protobuf/stubs/mathlimits.cc                          
#  ${PROTOBUF_ROOT}/google/protobuf/stubs/mathlimits.h                           
)

set(PKT2LIBSRC
	str-pkt2.cpp pbjson.cpp packet2message.cpp pkt2packetvariable.cpp javascript-context.cpp error-printer.cpp
	messageformat.cpp messagecomposer.cpp messagedecomposer.cpp fieldnamevalueindexstrings.cpp pkt2optionscache.cpp
  pkt2optionscache.cpp protobuf-declarations.cpp pkt2.pb.cc
  utilfile.cpp utilstring.cpp utilprotobuf.cpp third_party/duk/duktape.c
  env-pkt2.cpp packet-protobuf-message.cpp database-config.cpp
)

set(PKT2LIBUTILSRC
	errorcodes.cpp daemonize.cpp
	jsoncpp.cpp
	utilpriority.cpp utilinet.cpp
	NanoMessage.cpp input-packet.cpp
	google-sheets.cpp sslhelper.cpp helper_fcm.cpp
	pg-connect.cpp
  third_party/argtable3/argtable3.c
)

set(cpp_pkt2js pkt2js.cpp pkt2js-config.cpp pkt2js-impl.cpp daemonize.cpp 
  third_party/argtable3/argtable3.c
  ${PKT2LIBSRC}
  ${PROTO_SRC}
)

add_library(
  pkt2 STATIC
  ${PKT2LIBSRC}
  ${PROTO_SRC}
)

add_library(
  pkt2util STATIC
  ${PKT2LIBUTILSRC}
)

target_include_directories(pkt2util PUBLIC /usr/include/postgresql)

add_definitions(-DHAVE_PTHREAD=1 -DHAVE_ZLIB=1 -DLIB_PBJSON)

# pkt2
# add_executable(pkt2 pkt2.cpp pkt2-config.cpp pkt2-impl.cpp)
# target_link_libraries(pkt2 glog)

set(HUFF_SRC
  huff/huffcode.cpp huff/huffmanmodifieddecoder.cpp huff/huffmanmodifiedencoder.cpp
	huff/internalnode.cpp huff/leafnode.cpp huff/node.cpp huff/devdecoder.cpp
	third_party/bitstream/bitstream.cpp
)

# tcpreceiver
add_executable(tcpreceiver
  tcpreceiver.cpp tcpreceiver-config.cpp tcpreceivernano.cpp get_rss.c 
  ${HUFF_SRC}
)
target_include_directories(tcpreceiver PUBLIC huff)
target_link_libraries(tcpreceiver glog unwind nanomsg pkt2 pkt2util)

# tcptransmitter
add_executable(tcptransmitter
  tcptransmitter.cpp tcptransmitter-config.cpp
)
target_link_libraries(tcptransmitter glog unwind nanomsg pkt2 pkt2util)

# tcpemitter
add_executable(tcpemitter
  tcpemitter.cpp tcpemitter-config.cpp tcpemitterline.cpp
)
target_link_libraries(tcpemitter glog pkt2 pkt2util)

# pkt2receiver-check
add_executable(pkt2receiver-check
  pkt2receiver-check.cpp pkt2receiver-check-config.cpp
)
target_link_libraries(pkt2receiver-check glog unwind nanomsg pkt2 pkt2util)

# repeator
add_executable(repeator
  repeator.cpp repeator-config.cpp
)
target_link_libraries(repeator glog unwind nanomsg pkt2 pkt2util)

# messageemitter
add_executable(messageemitter
  messageemitter.cpp messageemitter-config.cpp messageemitterline.cpp
)
target_link_libraries(messageemitter glog nanomsg pkt2 pkt2util)

# pkt2receiver
add_executable(pkt2receiver
  pkt2receiver.cpp pkt2receiver-config.cpp pkt2receivernano.cpp
)
target_link_libraries(pkt2receiver glog nanomsg pkt2 pkt2util)

# handlerline
add_executable(handlerline
  handlerline.cpp linewriter.cpp handlerline-config.cpp
)
target_link_libraries(handlerline glog nanomsg pkt2 pkt2util)

# handlerlmdb
add_executable(handlerlmdb
  handlerlmdb.cpp lmdbwriter.cpp handlerlmdb-config.cpp
)
target_link_libraries(handlerlmdb glog nanomsg pkt2 pkt2util lmdb)

# handler-fcm
add_executable(handler-fcm
  handler-fcm.cpp fcm-writer.cpp handler-fcm-config.cpp
)
target_link_libraries(handler-fcm glog nanomsg ssl crypto curl pq pkt2 pkt2util)

# handler-google-sheets
add_executable(handler-google-sheets
  handler-google-sheets.cpp google-sheets-writer.cpp handler-google-sheets-config.cpp
)
target_link_libraries(handler-google-sheets glog unwind nanomsg curl pq pkt2 pkt2util ssl crypto)

# pkt2dumppq
add_executable(pkt2dumppq
  pkt2dumppq.cpp pqdumper.cpp pkt2dumppq-config.cpp
)
target_link_libraries(pkt2dumppq glog unwind nanomsg pq pkt2 pkt2util)

# pkt2dumpfcm
add_executable(pkt2dumpfcm
  pkt2dumpfcm.cpp fcmdumper.cpp pkt2dumpfcm-config.cpp
)
target_link_libraries(pkt2dumpfcm glog unwind nanomsg curl pq pkt2 pkt2util ssl crypto)
target_include_directories(pkt2dumpfcm PUBLIC /home/andrei/lib/firebase_cpp_sdk/include)

# pkt2gateway
add_executable(pkt2gateway
  pkt2gateway.cpp pkt2gateway-config.cpp
)
target_link_libraries(pkt2gateway glog nanomsg pkt2 pkt2util)

# handlerpq
add_executable(handlerpq
  handlerpq.cpp handlerpq-config.cpp pqwriter.cpp pg-connect.cpp
)
target_link_libraries(handlerpq glog nanomsg pkt2 pkt2util pq)

# freceiver
add_executable(freceiver
  freceiver.cpp freceiver-config.cpp freceiver-impl.cpp
)
target_link_libraries(freceiver glog nanomsg pkt2 pkt2util)
# Ws2_32

# pkt2js
add_executable(pkt2js pkt2js.cpp pkt2js-config.cpp pkt2js-impl.cpp )
target_link_libraries(pkt2js glog pkt2 pkt2util)

# message2gateway
add_executable(message2gateway message2gateway.cpp message2gateway-impl.cpp message2gateway-config.cpp )
target_link_libraries(message2gateway protoc glog nanomsg pkt2 pkt2util)

# protoc-gen-pkt2
add_executable(protoc-gen-pkt2 protoc-gen-pkt2.cpp pkt2_code_generator.cpp )
target_link_libraries(protoc-gen-pkt2 protoc pkt2)

# js2sheet
add_executable(js2sheet js2sheet.cpp js2sheet-config.cpp google-sheets-writer-from-js.cpp )
target_link_libraries(js2sheet glog protoc pkt2 pkt2util curl ssl crypto)

# pkt2sheet
add_executable(pkt2sheet pkt2sheet.cpp pkt2sheet-config.cpp google-sheets-writer-from-file.cpp )
target_link_libraries(pkt2sheet glog protoc pkt2 pkt2util curl ssl crypto)
