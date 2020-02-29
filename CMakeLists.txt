cmake_minimum_required(VERSION 2.8)
# OPTION(WITH_DEBUG_MODE "Build with debug mode" ON)

project(pkt2)
set(pkt2_VERSION_MAJOR 0)
set(pkt2_VERSION_MINOR 1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set (PROTOBUF_ROOT third_party/protobuf)

include_directories(. ${PROTOBUF_ROOT} third_party )
set(LIBS ${LIBS})

# set(CC /usr/bin/clang)
# set(CXX /usr/bin/clang+)

#
# make link to /home/andrei/lib/grpc/third_party/protobuf/src
# ln -s /home/andrei/lib/grpc/third_party/protobuf/src protobuf
#

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
  ${PROTOBUF_ROOT}/google/protobuf/io/coded_stream_inl.h                        
  ${PROTOBUF_ROOT}/google/protobuf/io/strtod.cc                                 
  ${PROTOBUF_ROOT}/google/protobuf/io/zero_copy_stream.cc                       
  ${PROTOBUF_ROOT}/google/protobuf/io/zero_copy_stream_impl_lite.cc
)

set(PROTO_SRC
  ${PROTO_LITE_SRC}
  ${PROTOBUF_ROOT}/google/protobuf/any.pb.cc                                    
  ${PROTOBUF_ROOT}/google/protobuf/api.pb.cc                                    
  ${PROTOBUF_ROOT}/google/protobuf/stubs/mathlimits.cc                          
  ${PROTOBUF_ROOT}/google/protobuf/stubs/mathlimits.h                           
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
)

set(PKT2LIBSRC
	str-pkt2.cpp pbjson.cpp packet2message.cpp pkt2packetvariable.cpp javascript-context.cpp error-printer.cpp
	messageformat.cpp messagecomposer.cpp messagedecomposer.cpp fieldnamevalueindexstrings.cpp pkt2optionscache.cpp
  protobuf-declarations.cpp pkt2.pb.cpp utilfile.cpp utilstring.cpp utilprotobuf.cpp duk/duktape.c
)

set(cpp_pkt2js pkt2js.cpp pkt2js-config.cpp pkt2js-impl.cpp daemonize.cpp 
  third_party/argtable3/argtable3.c
  ${PKT2LIBSRC}
  ${PROTO_SRC}
)

add_definitions(-DHAVE_PTHREAD=1 -DHAVE_ZLIB=1 -DLIB_PBJSON)
add_executable(pkt2js ${cpp_pkt2js})
target_link_libraries(pkt2js glog unwind z)
