/*
 * utilsnmp.h
 *
 *  Created on: Jan 23, 2017
 *      Author: andrei
 */

#ifndef UTILSNMP_H_
#define UTILSNMP_H_

typedef void (*initMIBs)();

// {iso(1) identified-organization(3) dod(6) internet(1) private(4) enterprise(1) 46821}
// 1.3.6.1.4.1.46821

int snmpInit
(
	const char *progname,
	bool agentOn,
	int verbosity,
	bool *stopRequest,
	initMIBs cbInitMIBs
);

void snmpDone
(
	const char *progname
);

#endif /* UTILSNMP_H_ */
