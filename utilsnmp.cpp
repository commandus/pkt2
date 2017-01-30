/*
 * utilsnmp.cpp
 *
 *  Created on: Jan 23, 2017
 *      Author: andrei
 */

#include <thread>

#ifdef __cplusplus
extern "C" {
#endif
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#ifdef __cplusplus
}
#endif

#include "utilsnmp.h"

void snmpLoop
(
	bool *stopRequest
)
{
	while(!*stopRequest)
	{
		agent_check_and_process(1); // 0 == don't block
	}
}

int snmpInit
(
	const char *progname,
	bool agentOn,
	int verbosity,
	bool *stopRequest,
	initMIBs cbInitMIBs
)
{
	init_snmp_logging();
	snmp_enable_syslog();
	snmp_enable_stderrlog();      // print log errors to stderr
	if (verbosity)
		snmp_set_do_debugging(verbosity);
	// we're an agentx subagent?
	if (agentOn)
	{
		netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1 );
	}

	SOCK_STARTUP;                 // initialize tcpip, if necessary
	init_agent(progname);         // initialize the agent library
	cbInitMIBs();                  // initialize mib code here...
	init_snmp(progname);
	if (!agentOn)                 // If we're going to be a snmp master agent, initial the ports
	  init_master_agent();        // open the port to listen on (defaults to udp:161)

	*stopRequest = false;
	std::thread t(snmpLoop, stopRequest);
	t.detach();
}

void snmpDone
(
	const char *progname
)
{
	snmp_shutdown(progname);
	SOCK_CLEANUP;
}

