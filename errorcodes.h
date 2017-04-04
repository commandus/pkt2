/*
 * errorcodes.h
 *
 */

#ifndef ERRORCODES_H_
#define ERRORCODES_H_

#define MSG_OK									"OK"
#define MSG_FAILED								"failed"
#define MSG_START								"Start"
#define MSG_STOP								"Stopped.."
#define MSG_DONE								"Done"
#define MSG_INTERRUPTED							"Interrupted"
#define MSG_SIGNAL								"Signal "
#define MSG_DAEMONIZE							"Start as daemon, use syslog"
#define MSG_PROCESSED							"successfully processed"
#define MSG_PARSE_FILE 							"parse proto file: "
#define MSG_PARSE_PROTO_COUNT					"Parsed proto file(s): "
#define MSG_EOF									"End of file detected after record "
#define MSG_MESSAGE_DESCRIPTOR_CNT				"Protobuf descriptor(s): "
#define MSG_LOOP_EXIT							"Event loop exit"
#define MSG_PROTO_FILES_HEADER					"Protobuf files: "
#define MSG_MESSAGE_HEADER						"Protobuf messages: "
#define MSG_CONNECTED_TO						"Connected to: "
#define MSG_PACKET_HEX							"Packet (hex): "
#define MSG_SENT								"Sent bytes: "
#define MSG_RECEIVED							"Received bytes: "

#define ERR_OK									0
#define ERRCODE_COMMAND							1
#define ERRCODE_PARSE_COMMAND					2

#define ERRCODE_LMDB_TXN_BEGIN					10
#define ERRCODE_LMDB_TXN_COMMIT					11
#define ERRCODE_LMDB_OPEN						12
#define ERRCODE_LMDB_CLOSE						13
#define ERRCODE_LMDB_PUT						14
#define ERRCODE_LMDB_GET						15

#define ERRCODE_NN_CONNECT	 					16
#define ERRCODE_NN_SUBSCRIBE					17
#define ERRCODE_NN_SHUTDOWN 					18
#define ERRCODE_NN_RECV							19
#define ERRCODE_NN_SEND							20

#define ERRCODE_PACKET_PARSE					21
#define ERRCODE_STOP							22
#define ERRCODE_NO_CONFIG						23

#define ERRCODE_LOAD_PROTO 						24
#define ERRCODE_NO_MEMORY						25
#define ERRCODE_DECODE_MESSAGE					26

#define ERRCODE_SOCKET_SEND						27

#define ERRCODE_MESSAGE_TYPE_NOT_FOUND			28

#define ERRCODE_DECOMPOSE_NO_MESSAGE_DESCRIPTOR	29
#define ERRCODE_DECOMPOSE_NO_FIELD_DESCRIPTOR	30
#define ERRCODE_DECOMPOSE_FATAL					31
#define ERRCODE_DECOMPOSE_NO_REFECTION			32
#define ERRCODE_NO_CALLBACK						33
#define ERRCODE_NOT_IMPLEMENTED					34
#define ERRCODE_DATABASE_NO_CONNECTION			35
#define ERRCODE_DATABASE_STATEMENT_FAIL         36
#define ERRCODE_GET_ADDRINFO                    37
#define ERRCODE_SOCKET_CREATE					38
#define ERRCODE_SOCKET_SET_OPTIONS				39
#define ERRCODE_SOCKET_BIND						40
#define ERRCODE_SOCKET_CONNECT                  41
#define ERRCODE_SOCKET_LISTEN					42
#define ERRCODE_SOCKET_READ						43
#define ERRCODE_SOCKET_WRITE					44

#define ERRCODE_NN_ACCEPT						45
#define ERRCODE_PARSE_PACKET					46
#define ERRCODE_PACKET_TOO_SMALL				47

#define ERR_COMMAND								"Invalid command line options or help requested."
#define ERR_PARSE_COMMAND						"Error parse command line options, possible cause is insufficient memory."
#define ERR_LMDB_TXN_BEGIN						"Can not begin LMDB transaction "
#define ERR_LMDB_TXN_COMMIT						"Can not commit LMDB transaction "
#define ERR_LMDB_OPEN							"Can not open database file "
#define ERR_LMDB_CLOSE							"Can not close database file "
#define ERR_LMDB_PUT							"Can not put LMDB "
#define ERR_LMDB_GET							"Can not get LMDB "

#define ERR_NN_CONNECT							"Can not connect to the IPC url "
#define ERR_NN_SUBSCRIBE						"Can not subscribe to the IPC url "
#define ERR_NN_SHUTDOWN							"Can not shutdown nanomsg socket "
#define ERR_NN_RECV								"Receive nanomsg error "
#define ERR_NN_SEND								"Send nanomsg error "
#define ERR_NN_ACCEPT							"Accept error: "

#define ERR_PACKET_PARSE						"Error parse packet "
#define ERR_STOP								"Can not stop"
#define ERR_NO_CONFIG							"No config provided"

#define ERR_LOAD_PROTO 							"Can not load proto file(s) from "
#define ERR_OPEN_PROTO 							"Can not open proto file from "
#define ERR_PARSE_PROTO							"Cannot parse proto file "
#define ERR_PROTO_GET_DESCRIPTOR 				"Cannot get proto file descriptor from file "
#define ERR_NO_MEMORY							"Can not allocate buffer size "
#define ERR_DECODE_MESSAGE						"Error decode message "
#define ERR_MESSAGE_TYPE_NOT_FOUND				"Protobuf message not found "

#define ERR_SOCKET_SEND							"Error socket send "

#define ERR_NOT_IMPLEMENTED					    "Not implemented"
#define ERR_DATABASE_NO_CONNECTION			    "No database connection. Check credentials."
#define ERR_DATABASE_STATEMENT_FAIL             "SQL command error "

#define ERR_GET_ADDRINFO                        "Address resolve error: "
#define ERR_SOCKET_CREATE 						"Socket create error: "
#define ERR_SOCKET_SET_OPTIONS					"Socket set options error"
#define ERR_SOCKET_BIND							"Socket bind error "
#define ERR_SOCKET_CONNECT                      "Socket connect error "
#define ERR_SOCKET_LISTEN						"Socket listen error "
#define ERR_SOCKET_READ							"Socket read error "
#define ERR_SOCKET_WRITE						"Socket write error "
#define ERR_PARSE_LINE							"Parse message line"
#define ERR_PARSE_PACKET						"Parse packet error "
#define ERR_PACKET_TOO_SMALL					"Packet size is too small, field "

#endif /* ERRORCODES_H_ */
