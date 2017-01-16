#ifndef DAEMONIZE_H
#define DAEMONIZE_H	1

#include <string>

typedef void(*TDaemonRunner)();

/**
 * 	run daemon
 * 	\see http://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux
 */
class Daemonize
{
private:
	int init();
public:
	Daemonize(
		const std::string &daemonName,
		TDaemonRunner runner,				///< function to run as deamon
		TDaemonRunner stopRequest, 			///< function to stop
		TDaemonRunner done					///< function to clean after runner exit
	);
	~Daemonize();
};

#endif
