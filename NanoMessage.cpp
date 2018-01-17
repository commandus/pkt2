/*
 *
 * NanoMessage.cpp
 * nanomsg C++ wrapper
 *
 */

#include <sstream>
#include <cstring>

#include "glog/logging.h"

#include "NanoMessage.h"

#include <sys/socket.h>
#include <arpa/inet.h>


#ifdef _MSC_VER
#define ERRNO nn_errno
#else
#define ERRNO errno
#endif

const char *nn_err_strerror (int errnum);

NanoMessage::NanoMessage(int protocol)
	: mFamily(AF_SP), mProtocol(protocol),
	mError(0), mSocket(-1), mReadOffset(0)
{
}

NanoMessage::~NanoMessage() {
}

int NanoMessage::family()
{
	return mFamily;
}

void NanoMessage::setFamily(int value)
{
	mFamily = value;
}

int NanoMessage::error()
{
	return mError;
}

std::string NanoMessage::errorMessage()
{
	return std::string(nn_err_strerror(mError));
}

int NanoMessage::socket()
{
    mSocket = nn_socket(mFamily, mProtocol);
    if (mSocket == -1)
    	mError = ERRNO;
    else
    	mError = 0;
    return mSocket;
}

int NanoMessage::connect(const char *address)
{
    int r = nn_connect(mSocket, address);
    if (r < 0)
    	mError = ERRNO;
    else
    	mError = 0;
    return r;
}

int NanoMessage::bind(const char *address)
{
    int r = nn_bind(mSocket, address);
    if (r < 0)
    	mError = ERRNO;
    else
    	mError = 0;
    return r;
}

int NanoMessage::getsockoption(int level, int option, void *optval, size_t *optlen)
{
	int r = nn_getsockopt(mSocket, level, option, optval, optlen);
	if (r == -1)
		mError = ERRNO;
	else
		mError = 0;
	return r;
}

int NanoMessage::setsockoption(int level, int option, const void *optval, size_t optlen)
{
    int r = nn_setsockopt(mSocket, level, option, optval, optlen);
	if (r < 0)
		mError = ERRNO;
	else
		mError = 0;
	return r;
}

void NanoMessage::close()
{
    int r = nn_close(mSocket);
    if ((r != 0) && (ERRNO != EBADF && ERRNO != ETERM))
    	mError = ERRNO;
    else
    	mError = 0;
}

void NanoMessage::send(const void *buf, size_t bufsize)
{
    int r = nn_send(mSocket, buf, bufsize, 0);
    if (r < 0)
    	mError = ERRNO;
    else
    	if (r != (int) bufsize)
    		mError = EMSGSIZE;
        else
        	mError = 0;
    // flush
    sleep(0);
}

void NanoMessage::send(const std::string &data)
{
	send(data.c_str(), data.length());
}

int NanoMessage::recv(void *buf, size_t bufsize)
{
    int r = nn_recv(mSocket, buf, bufsize, 0);
    if (r < 0)
    	mError = ERRNO;
    else
    	mError = 0;
    return r;
}

/**
 * nn_freemsg()
 */
int NanoMessage::recv(void **buf)
{
    int r = nn_recv(mSocket, buf, NN_MSG, 0);
    if (r < 0)
    	mError = ERRNO;
    else
    	mError = 0;
    return r;
}

std::string NanoMessage::recv()
{
	void *b;
    int r = recv(&b);
    if (r < 0)
    {
    	mError = ERRNO;
    	return "";
    }
    else
    {
    	std::string ret(((char *) b) + mReadOffset, r - mReadOffset);
    	nn_freemsg(b);
    	mError = 0;
    	return ret;
    }
}

void NanoMessage::drop(int err)
{
    char buf[1024];
    int r = nn_recv(mSocket, buf, sizeof(buf), 0);
    if (r < 0 && err != ERRNO)
    	mError = ERRNO;
    else
    	if (r >= 0)
    		mError = r;
    	else
        	mError = 0;
}

const char *nn_err_strerror(int errnum)
{
    switch (errnum) {
#if defined NN_HAVE_WINDOWS
    case ENOTSUP:
        return "Not supported";
    case EPROTONOSUPPORT:
        return "Protocol not supported";
    case ENOBUFS:
        return "No buffer space available";
    case ENETDOWN:
        return "Network is down";
    case EADDRINUSE:
        return "Address in use";
    case EADDRNOTAVAIL:
        return "Address not available";
    case ECONNREFUSED:
        return "Connection refused";
    case EINPROGRESS:
        return "Operation in progress";
    case ENOTSOCK:
        return "Not a socket";
    case EAFNOSUPPORT:
        return "Address family not supported";
    case EPROTO:
        return "Protocol error";
    case EAGAIN:
        return "Resource unavailable, try again";
    case EBADF:
        return "Bad file descriptor";
    case EINVAL:
        return "Invalid argument";
    case EMFILE:
        return "Too many open files";
    case EFAULT:
        return "Bad address";
    case EACCES:
        return "Permission denied";
    case ENETRESET:
        return "Connection aborted by network";
    case ENETUNREACH:
        return "Network unreachable";
    case EHOSTUNREACH:
        return "Host is unreachable";
    case ENOTCONN:
        return "The socket is not connected";
    case EMSGSIZE:
        return "Message too large";
    case ETIMEDOUT:
        return "Timed out";
    case ECONNABORTED:
        return "Connection aborted";
    case ECONNRESET:
        return "Connection reset";
    case ENOPROTOOPT:
        return "Protocol not available";
    case EISCONN:
        return "Socket is connected";
#endif
    case ETERM:
        return "Nanomsg library was terminated";
    case EFSM:
        return "Operation cannot be performed in this state";
    default:
#if defined _MSC_VER
#pragma warning (push)
#pragma warning (disable:4996)
#endif
        return strerror(errnum);
#if defined _MSC_VER
#pragma warning (pop)
#endif
    }
}

NanoPubSub::NanoPubSub(int protocol)
	: NanoMessage(protocol)
{
	socket();
}

NanoPubSub::~NanoPubSub()
{
	close();
}

NanoPub::NanoPub(const char *address)
	: NanoPubSub(NN_PUB), mAddress(address)
{
	if (bind(address) >= 0)
	{
		/*
		if (broadcastaddress)
		{
			int broadcastEnable = 1;
			if (setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
				return;
		}
		*/
	}
}

const std::string &NanoPub::address()
{
	return mAddress;
}

NanoSub::NanoSub(const char *address, void *topic, size_t topic_size)
	: NanoPubSub(NN_SUB)
{
	mTopic = std::string((char *) topic, topic_size);
	mReadOffset = topic_size;
    if (setsockoption(NN_SUB, NN_SUB_SUBSCRIBE, topic, topic_size) >= 0)
    	connect(address);
}

NanoSub::NanoSub(const char *address, const std::string &topic)
	: NanoPubSub(NN_SUB)
{
	mTopic = topic;
    mReadOffset = topic.size();
    if (setsockoption(NN_SUB, NN_SUB_SUBSCRIBE, topic.c_str(), topic.size()) >= 0)
		connect(address);
}

std::string & NanoSub::topic()
{
	return mTopic;
}
