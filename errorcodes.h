/*
 * errorcodes.h
 *
 */

#ifndef ERRORCODES_H_
#define ERRORCODES_H_

#define MSG_STOP				"Stopped.."
#define MSG_DONE				"Done"
#define MSG_INTERRUPTED			"Interrupted"
#define MSG_SIGNAL				"Signal "
#define MSG_DAEMONIZE			"Start as daemon, use syslog"

#define ERR_OK						0
#define ERRCODE_COMMAND				1
#define ERRCODE_PARSE_COMMAND		2
// lmdbwriter

#define ERRCODE_LMDB_OPEN			10
#define ERRCODE_LMDB_CLOSE			11

#define ERRCODE_NN_CONNECT	 		12
#define ERRCODE_NN_SHUTDOWN 		13
#define ERRCODE_NN_RECV				14

#define ERRCODE_PACKET_PARSE		15
#define ERRCODE_STOP				16
#define ERRCODE_NO_CONFIG			17


#define ERR_COMMAND					"Invalid command line options or help requested."
#define ERR_PARSE_COMMAND			"Error parse command line options, possible cause is insuffient memory."
#define ERR_LMDB_OPEN				"Can not open database file " 
#define ERR_LMDB_CLOSE				"Can not close database file "

#define ERR_NN_CONNECT				"Can not connect to the IPC url "
#define ERR_NN_SHUTDOWN				"Can not shutdown nanomsg socket "
#define ERR_NN_RECV					"Receive nanomsg error "

#define ERR_PACKET_PARSE			"Error parse packet "
#define ERR_STOP					"Can not stop"
#define ERR_NO_CONFIG				"No config provided"


#endif /* ERRORCODES_H_ */
