#define VERSION "0.1"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define PATH_MAX 260
#include <utilstring.h>
#else
#include <time.h>
#include <unistd.h>
#endif

#ifdef __cplusplus
#define CALLC extern "C" 
#else
#define CALLC
#endif

#define INIT_LOGGING(PROGRAM_NAME) \
	google::InstallFailureSignalHandler(); \
	FLAGS_logtostderr = !config->verbosity; \
	FLAGS_minloglevel = 2 - config->verbosity; \
	if (config->verbosity > 0) \
	   google::SetLogDestination(google::INFO, PROGRAM_NAME); \
	google::InitGoogleLogging(argv[0]);


#define ENDIAN_NETWORK pkt2::Endian::ENDIAN_BIG_ENDIAN
#define ENDIAN_HOST pkt2::ENDIAN_LITTLE_ENDIAN
#define ENDIAN_NEED_SWAP(v) ((v != pkt2::ENDIAN_NO_MATTER) && (v != ENDIAN_HOST))

#if defined(_WIN32) || defined(_WIN64)

#define SLEEP(seconds) \
    Sleep(seconds *1000);

#define SEND_FLUSH \
    Sleep(100);
#define WAIT_CONNECTION \
	Sleep(1000); // wait for connections
#else
#define SLEEP(seconds) \
    sleep(seconds);
#define SEND_FLUSH(milliseconds) \
{ \
	sleep(0); \
    struct timespec ts; \
    ts.tv_sec = milliseconds / 1000;  \
    ts.tv_nsec = (milliseconds % 1000) * 1000000; \
    nanosleep (&ts, NULL); \
}

#define WAIT_CONNECTION(timeout) \
	sleep(timeout); // wait for connections
#endif


// A macro to disallow operator=
// This should be used in the private: declarations for a class.
#define GTEST_DISALLOW_ASSIGN_(type)\
  void operator=(type const &)

// A macro to disallow copy constructor and operator=
// This should be used in the private: declarations for a class.
#define GTEST_DISALLOW_COPY_AND_ASSIGN_(type)\
  type(type const &);\
  GTEST_DISALLOW_ASSIGN_(type)


#ifndef GOOGLE_PROTOBUF_STUBS_SCOPED_PTR_H_

namespace google {
namespace protobuf {
	template <typename T>
	class scoped_ptr {
	public:
	typedef T element_type;

	explicit scoped_ptr(T* p = NULL) : ptr_(p) {}
	~scoped_ptr() { reset(); }

	T& operator*() const { return *ptr_; }
	T* operator->() const { return ptr_; }
	T* get() const { return ptr_; }

	T* release() {
		T* const ptr = ptr_;
		ptr_ = NULL;
		return ptr;
	}

	void reset(T* p = NULL) {
		if (p != ptr_) {
		delete ptr_;
		ptr_ = p;
		}
	}

	friend void swap(scoped_ptr& a, scoped_ptr& b) {
		using std::swap;
		swap(a.ptr_, b.ptr_);
	}

	private:
	T* ptr_;

	GTEST_DISALLOW_COPY_AND_ASSIGN_(scoped_ptr);
	};
}
}

#endif
