/**
 * @file rsp.h
 * @author Marek Dolezel (marekdolezel@gmail.com)
 * @brief GDB Remote serial protocol  
 * @version 0.1
 * @date 2018-11-26
 * 
 * @copyright Copyright (c) 2018
 * 
 */

#ifndef RSP_H
#define RSP_H

#include <stdint.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h> // for memset()

#include "storage.h"
#include "protocol.h"
                    
#define RSP_GDB_MAX 4096

struct rsp_connection {
    struct sockaddr_in gdb_server;
    uint32_t sockfd;
    proto_cmd_type prev_cmd;
};



/** TODO: verify this
    format of a packet +$[a-Z]#[0-9][0-9], where:
        - first letter after $ represents command, 
        - 2 digits after # represent checksum of a packet 
*/

struct rsp_pkt {
    char data[RSP_GDB_MAX];
    int index;
    int length;
};


uint32_t rsp_init(struct rsp_connection *conn, uint32_t host, uint16_t port);
uint32_t rsp_close(struct rsp_connection *conn);

uint32_t rsp_send_command(struct rsp_connection *conn, struct proto_command *proto_cmd );
uint32_t rsp_recv_command(struct rsp_connection *conn, proto_cmd_type *type, struct storage *st);
#endif