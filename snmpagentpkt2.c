#include "snmpagentpkt2.h"

#include "get_rss.h"

static struct SNMP_PKT2 counter;

#ifdef __cplusplus
extern "C" {
#endif

void init_mibs()
{
	void init_pid();
	void init_ppid();
	void init_state();
	void init_procName();
	void init_memoryCurrent();
	void init_memoryPeak();
}

void init_pid()
{
    const oid pid_oid[] = { 1,3,6,1,4,1,46956,1,2,1,1,1,1 };

  DEBUGMSGTL(("pid", "Initializing\n"));

    netsnmp_register_scalar(
        netsnmp_create_handler_registration("pid", handle_pid,
                               pid_oid, OID_LENGTH(pid_oid),
                               HANDLER_CAN_RONLY
        ));
}

int handle_pid(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                     &counter.pid, sizeof(counter.pid));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_pid\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

void init_ppid()
{
    const oid ppid_oid[] = { 1,3,6,1,4,1,46956,1,2,1,1,1,2 };

  DEBUGMSGTL(("ppid", "Initializing\n"));

    netsnmp_register_scalar(
        netsnmp_create_handler_registration("ppid", handle_ppid,
                               ppid_oid, OID_LENGTH(ppid_oid),
                               HANDLER_CAN_RONLY
        ));
}

int handle_ppid(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
            		&counter.ppid, sizeof(counter.ppid));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_ppid\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

void init_state()
{
    const oid state_oid[] = { 1,3,6,1,4,1,46956,1,2,1,1,1,4 };

  DEBUGMSGTL(("state", "Initializing\n"));

    netsnmp_register_scalar(
        netsnmp_create_handler_registration("state", handle_state,
                               state_oid, OID_LENGTH(state_oid),
                               HANDLER_CAN_RONLY
        ));
}

int handle_state(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
            		&counter.state, sizeof(counter.state));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_state\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

void init_procName()
{
    const oid procName_oid[] = { 1,3,6,1,4,1,46956,1,2,1,1,1,3 };

  DEBUGMSGTL(("procName", "Initializing\n"));

    netsnmp_register_scalar(
        netsnmp_create_handler_registration("procName", handle_procName,
                               procName_oid, OID_LENGTH(procName_oid),
                               HANDLER_CAN_RONLY
        ));
}

int handle_procName(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
            		counter.procName, strlen(counter.procName));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_procName\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

void init_memoryCurrent()
{
    const oid memoryCurrent_oid[] = { 1,3,6,1,4,1,46956,1,2,1,1,1,5 };

  DEBUGMSGTL(("memoryCurrent", "Initializing\n"));

    netsnmp_register_scalar(
        netsnmp_create_handler_registration("memoryCurrent", handle_memoryCurrent,
                               memoryCurrent_oid, OID_LENGTH(memoryCurrent_oid),
                               HANDLER_CAN_RONLY
        ));
}

int handle_memoryCurrent(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
            		&counter.memoryCurrent, sizeof(counter.memoryCurrent));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_memoryCurrent\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

void init_memoryPeak(void)
{
    const oid memoryPeak_oid[] = { 1,3,6,1,4,1,46956,1,2,1,1,1,6 };

  DEBUGMSGTL(("memoryPeak", "Initializing\n"));

    netsnmp_register_scalar(
        netsnmp_create_handler_registration("memoryPeak", handle_memoryPeak,
                               memoryPeak_oid, OID_LENGTH(memoryPeak_oid),
                               HANDLER_CAN_RONLY
        ));
}

int handle_memoryPeak(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
            		&counter.memoryPeak, sizeof(counter.memoryPeak));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_memoryPeak\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
