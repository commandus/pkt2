/*
 * errorcodes.h
 *
 */

#ifndef ERRORCODES_H_
#define ERRORCODES_H_

#define MSG_OK						"OK"
#define MSG_FAILED					"failed"
#define MSG_START					"Start"
#define MSG_STOP					"Stopped.."
#define MSG_DONE					"Done"
#define MSG_INTERRUPTED				"Interrupted"
#define MSG_SIGNAL					"Signal "
#define MSG_DAEMONIZE				"Start as daemon, use syslog"
#define MSG_PROCESSED				"successfully processed"
#define MSG_PARSE_FILE 				"parse proto file: "
#define MSG_PARSE_PROTO_COUNT		"Parsed proto file(s): "
#define MSG_EOF						"End of file detected after record "
#define MSG_MESSAGE_DESCRIPTOR_CNT	"Protobuf descriptor(s): "
#define MSG_LOOP_EXIT				"Event loop exit"

#define ERR_OK						0
#define ERRCODE_COMMAND				1
#define ERRCODE_PARSE_COMMAND		2

#define ERRCODE_LMDB_TXN_BEGIN		10
#define ERRCODE_LMDB_TXN_COMMIT		11
#define ERRCODE_LMDB_OPEN			12
#define ERRCODE_LMDB_CLOSE			13

#define ERRCODE_NN_CONNECT	 		14
#define ERRCODE_NN_SHUTDOWN 		15
#define ERRCODE_NN_RECV				16
#define ERRCODE_NN_SEND				17

#define ERRCODE_PACKET_PARSE		18
#define ERRCODE_STOP				19
#define ERRCODE_NO_CONFIG			20

#define ERRCODE_LOAD_PROTO 			21
#define ERRCODE_NO_MEMORY			22
#define ERRCODE_DECODE_MESSAGE		23

#define ERR_COMMAND					"Invalid command line options or help requested."
#define ERR_PARSE_COMMAND			"Error parse command line options, possible cause is insufficient memory."
#define ERR_LMDB_TXN_BEGIN			"Can not begin LMDB transaction "
#define ERR_LMDB_TXN_COMMIT			"Can not commit LMDB transaction "
#define ERR_LMDB_OPEN				"Can not open database file " 
#define ERR_LMDB_CLOSE				"Can not close database file "

#define ERR_NN_CONNECT				"Can not connect to the IPC url "
#define ERR_NN_SHUTDOWN				"Can not shutdown nanomsg socket "
#define ERR_NN_RECV					"Receive nanomsg error "
#define ERR_NN_SEND					"Send nanomsg error "

#define ERR_PACKET_PARSE			"Error parse packet "
#define ERR_STOP					"Can not stop"
#define ERR_NO_CONFIG				"No config provided"

#define ERR_LOAD_PROTO 				"Can not load proto file(s) from "
#define ERR_OPEN_PROTO 				"Can not open proto file from "
#define ERR_PARSE_PROTO				"Cannot parse proto file "
#define ERR_PROTO_GET_DESCRIPTOR 	"Cannot get proto file descriptor from file "
#define ERR_NO_MEMORY				"Can not allocate buffer size "
#define ERR_DECODE_MESSAGE			"Error decode message "

#endif /* ERRORCODES_H_ */
