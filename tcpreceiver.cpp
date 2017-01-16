#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

#include "tcpreceiver-config.h"

int node1 (
    const char *url,
    const char *msg)
{
    int sz_msg = strlen (msg) + 1; // '\0' too
    int sock = nn_socket (AF_SP, NN_PUSH);
    assert (sock >= 0);
    assert (nn_connect (sock, url) >= 0);
    printf ("NODE1: SENDING \"%s\"\n", msg);
    int bytes = nn_send (sock, msg, sz_msg, 0);
    assert (bytes == sz_msg);
    return nn_shutdown (sock, 0);
}

int main (
    const int argc, 
    const char **argv)
{
    
}
