/**
  * nanomsg C++ wrapper
  * \see http://nanomsg.org/
  */

#ifndef NANOMESSAGE_H_
#define NANOMESSAGE_H_

#include <string>
#include "nanomsg/nn.h"
#include "nanomsg/pubsub.h"

/**
  * nanomsg C++ wrapper class
  * \see http://nanomsg.org/
 */
class NanoMessage {
private:
	int mFamily;
	int mProtocol;
	int mError;
protected:
	int mSocket;
	size_t mReadOffset;
	int socket();
	int connect(const char *address);
	int bind(const char *address);
	int setsockoption(int level, int option, const void *optval, size_t optlen);
	void close();
	void drop(int err);
public:
	NanoMessage(
		int protocol	///< nanomsg protocol number
	);
	virtual ~NanoMessage();
	/// last operation error code
	int error();
	/// last operation error text description
	std::string errorMessage();
	/// nanomsg family
	int family();
	/// set nanomsg family
	void setFamily(
		int value
	);

	/// get nanomsg socket option
	int getsockoption(
		int level,
		int option,
		void *optval,
		size_t *optlen			///< size of the buffer (see nanomsg documentation)
	);
	/// send message
	void send(
		const void *buf,		///< buffer
		size_t bufsize			///< buffer size
	);
	/// send string message
	void send(
		const std::string &data	///< data to send represented as string
	);
	/// sync receive into buffer
	int recv(
		void *buf,				///< buffer
		size_t bufsize			///< buffer size
	);
	/// sync receive message of unknown size into buffer. Call nn_freemsg() if result > 0 to free memory.
	int recv(
		void **buf				///< buffer
	);
	/// sync receive string
	std::string recv();
};

/**
 * Publisher or Subscriber abstract class
 */
class NanoPubSub : public NanoMessage {
public:
	NanoPubSub(
		int protocol			///< nanomsg protocol umber
	);
	~NanoPubSub();
};

/**
  * Nanomsg Publisher
  */
class NanoPub : public NanoPubSub {
	std::string mAddress;
public:
	NanoPub(
		const char *address			///< address of the socket
	);
	const std::string &address();
};

/**
  * Nanomsg Subscriber
  */
class NanoSub : public NanoPubSub {
protected:
	std::string mTopic;
public:
	NanoSub(
		const char *address, 		///< address of the socket
		void *topic,				///< topic
		size_t topic_size
	);
	NanoSub(
		const char *address, 		///< address of the socket
		const std::string &topic	///< topic
	);
	/// topic
	std::string &topic();
};

#endif /* NANOMESSAGE_H_ */
