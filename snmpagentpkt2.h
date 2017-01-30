#ifndef SNMPAGETNTPKT2_H
#define SNMPAGETNTPKT2_H

#include <stdint.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

typedef struct SNMP_PKT2 {
        uint32_t pid;
        uint32_t ppid;
        uint32_t state;
        char *procName;
        uint32_t memoryCurrent;
        uint32_t memoryPeak;
} SNMP_PKT2;


#ifdef __cplusplus
extern "C" {
#endif

Netsnmp_Node_Handler handle_pid;
Netsnmp_Node_Handler handle_ppid;
Netsnmp_Node_Handler handle_state;
Netsnmp_Node_Handler handle_procName;
Netsnmp_Node_Handler handle_memoryCurrent;
Netsnmp_Node_Handler handle_memoryPeak;


void init_mibs(void);

void init_pid();
void init_ppid();
void init_state();
void init_procName();
void init_memoryCurrent();
void init_memoryPeak();

#ifdef __cplusplus
}
#endif

#endif /* SNMPAGETNTPKT2_H */
