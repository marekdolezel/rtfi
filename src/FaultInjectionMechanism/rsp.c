/**
 * @file rsp.c
 * @author Marek Dolezel (marekdolezel@gmail.com)
 * @brief GDB Remote serial protocol
 * @version 0.1
 * @date 2018-11-22
 * 
 * @copyright Copyright (c) 2018
 * 
 */

#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h> // for memset()
#include <assert.h>

#include "debug.h"
#include "rsp.h"
#include "error_codes.h"
#include "storage.h"
#include "protocol.h"


char *rsp_command_string[] = {
    "!", "?",
    "a", "A", "b", "B", "c", "C", "d", "D",
    "e", "E", "f", "F", "g", "G", "h", "H",
    "i", "I", "j", "J", "k", "K", "l", "L",
    "m", "M", "n", "N", "o", "O", "p", "P",
    "q", "Q", "r", "R", "s", "S", "t", "T",
    "u", "U", "v", "V", "w", "W", "x", "X",
    "y", "Y", "z", "Z",

    "OK", "+"
};

char hex_chars[] = "0123456789abcdef";

uint32_t 
_rsp_create_packet(struct rsp_connection *conn, struct proto_command *proto_cmd, struct rsp_pkt *rsp_pkt)
{
    if (conn == NULL || rsp_pkt == NULL) {
        debugprintf_ff_null(1, "struct rsp_connection *conn, or, struct rsp_pkt *rsp_pkt\n");
        return ERR_NULL_POINTER;
    }

    /* special case: when we want to stop the target, we need to send only one 0x3 byte, */
    if (proto_cmd->type == PROTO_STOP_EXEC) {
        rsp_pkt->data[0] = 0x3; 
        rsp_pkt->index++;

        debugprintf_ff(1, "sending '0x3 byte' to target\n");
        return ERR_NONE;
    }

    /* format of a packet $[a-Z]#[0-9][0-9], where:
        first letter after $ represents command, 
        2 digits after # represent checksum of a packet */
    rsp_pkt->data[0] = '$';
    rsp_pkt->index++;

    uint32_t sum = 0;
    uint8_t checksum = 0;

    /* Convert command to char */
    switch(proto_cmd->type) {
        case PROTO_GET_REG: rsp_pkt->data[rsp_pkt->index] = 'p'; rsp_pkt->index++; break;
        case PROTO_SET_REG: rsp_pkt->data[rsp_pkt->index] = 'P'; rsp_pkt->index++; break;
        case PROTO_GET_MEM: rsp_pkt->data[rsp_pkt->index] = 'm'; rsp_pkt->index++; break;
        case PROTO_SET_MEM: rsp_pkt->data[rsp_pkt->index] = 'M'; rsp_pkt->index++; break;
        case PROTO_SET_W_WATCHPOINT:
        case PROTO_SET_BREAKPOINT: rsp_pkt->data[rsp_pkt->index] = 'Z'; rsp_pkt->index++; break;
        case PROTO_REMOVE_W_WATCHPOINT:
        case PROTO_REMOVE_BREAKPOINT: rsp_pkt->data[rsp_pkt->index] = 'z'; rsp_pkt->index++; break;
        case PROTO_EXECUTE: rsp_pkt->data[rsp_pkt->index] = 'c'; rsp_pkt->index++; break;
        case PROTO_STOP_EXEC: /* Special case handled above */ break;

    }

    /* Processing arguments */
    conn->prev_cmd = proto_cmd->type;
    switch(proto_cmd->type) {

        case PROTO_GET_REG: // pn... - Read regegister n...
            rsp_pkt->index += sprintf(rsp_pkt->data+rsp_pkt->index, "%x", proto_cmd->reg_id);
        break;

        case PROTO_SET_REG: // Pn...=r... - Write register n with value r 
            rsp_pkt->index += sprintf(rsp_pkt->data+rsp_pkt->index, "%x=", proto_cmd->reg_id);
            for (int byte = 0; byte < proto_cmd->st->size; byte++) {
                char byte_content; storage_get_byte(proto_cmd->st, byte, &byte_content); 
                rsp_pkt->data[rsp_pkt->index] = hex_chars[(byte_content >> 4) & 0xF]; 
                rsp_pkt->data[rsp_pkt->index + 1] = hex_chars[byte_content & 0x0F];
                rsp_pkt->index += 2;
            }
        break;

        case PROTO_GET_MEM: // maddr,length - Read memory starting at address 'addr'
            rsp_pkt->index += sprintf(rsp_pkt->data+rsp_pkt->index, "%x,%x", proto_cmd->mem_addr, proto_cmd->length);
        break;

        case PROTO_SET_MEM: // Maddr,length: XX... - Write memory
            rsp_pkt->index += sprintf(rsp_pkt->data+rsp_pkt->index, "%x,%x:", proto_cmd->mem_addr, proto_cmd->length);
            for (int byte = 0; byte < proto_cmd->st->size; byte++) {
                char byte_content; storage_get_byte(proto_cmd->st, byte, &byte_content); 
                rsp_pkt->data[rsp_pkt->index] = hex_chars[(byte_content >> 4) & 0xF]; 
                rsp_pkt->data[rsp_pkt->index + 1] = hex_chars[byte_content & 0x0F];                
                rsp_pkt->index += 2;
                
            }
        break;
        case PROTO_SET_BREAKPOINT:  // set hw breakpoint Z1, addr,length
        case PROTO_REMOVE_BREAKPOINT: // remove hw breakpoint - z1,addr,length
            rsp_pkt->index += sprintf(rsp_pkt->data+rsp_pkt->index, "1,%x,%x", proto_cmd->mem_addr, proto_cmd->length);
        break;


        case PROTO_SET_W_WATCHPOINT:
        case PROTO_REMOVE_W_WATCHPOINT:
            rsp_pkt->index += sprintf(rsp_pkt->data+rsp_pkt->index, "2,%x,%x", proto_cmd->mem_addr, proto_cmd->length);
        break;
    }

    rsp_pkt->data[rsp_pkt->index] = '#';
    rsp_pkt->index++;

    for (int idx = 1; idx < rsp_pkt->index-1; idx++) {
        sum += rsp_pkt->data[idx];
    }
    checksum = sum % 256;
    rsp_pkt->data[rsp_pkt->index+1] = hex_chars[checksum & 0x0F];
    rsp_pkt->data[rsp_pkt->index] = hex_chars[(checksum >> 4) & 0xF];

    return 0;
}

int
_rsp_hex(int c)
{
    return  (c >= '0') && (c <= '9') ? c - '0' : 
            (c >= 'a') && (c <= 'f') ? c - 'a' + 10 :
            (c >= 'A') && (c <= 'F') ? c - 'A' + 10: -1; 

}

int
_rsp_store_packet_content(struct rsp_pkt *pkt, struct storage *st)
{
    if (pkt == NULL || st == NULL) {
        debugprintf_ff_null(1, "struct rsp_pkt *pkt, or, struct storage *st \n");
        return ERR_NULL_POINTER;
    }

    // +$xxxxx#cheksum

    // set data_idx to first MSB nibble representing data in packet
    int data_idx = 0;
    for (int i=0; i < RSP_GDB_MAX; i++) {
        if (pkt->data[i] == '$') {
            debugprintf_ff(4, "found $ in packet\n");
            data_idx = i+1;
            break;
        }
        if (i + 1 == RSP_GDB_MAX) {
            debugprintf_ff(4, "failed to find $ in packet\n");
            return -1;
        }
    }
    debugprintf_ff(4, "data_idx %d\n", data_idx);
    
    // assert(data_idx-1 != "T");

    uint8_t byte = 0x0;
    uint8_t nibble_MSB = 0x0;
    uint8_t nibble_LSB = 0x0;
    uint8_t byte_idx = 0;

    while (pkt->data[data_idx] != '#') {
        /* TODO: little vs big endian */
        nibble_MSB = _rsp_hex(pkt->data[data_idx]) << 4;
        nibble_LSB = _rsp_hex(pkt->data[data_idx+1]);

        // printf("MSB_nibc %c\n", pkt->data[data_idx]);
        // printf("LSB_nibc %c\n", pkt->data[data_idx+1]);

        // printf("MSB_nib %x\n", nibble_MSB);
        // printf("LSB_nib %x\n", nibble_LSB);
        byte = (uint16_t ) nibble_MSB |  (uint16_t ) nibble_LSB;

        debugprintf(4, "rsp_store: byte[%d]=%d\n", byte_idx, byte);
        if (st->size < byte_idx + 1) {
            debugprintf(2, "rsp_store: Invalid size\n");
            return ERR_INVALID_SIZE;
        }

        // printf("%x\n", byte);
        storage_set_byte(st, byte_idx, byte);
        //st->content[byte_counter] = byte; //TODO: storage_set_byte(...)
        
        data_idx += 2;
        byte_idx++;
    }

    return ERR_NONE;
}

uint32_t 
rsp_init(struct rsp_connection *conn, uint32_t host, uint16_t port)
{
    if (conn == NULL) {
        debugprintf_ff_null(1, "struct rsp_connection *conn\n");
        return ERR_NULL_POINTER;
    }

    conn->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->sockfd < 0) {
        debugprintf_ff(1, "call to socket() has failed with error %d\n", conn->sockfd);
        return ERR_SOCK_CREATE;
    }


    memset(&conn->gdb_server, 0, sizeof(conn->gdb_server));
    conn->gdb_server.sin_family = AF_INET;
    conn->gdb_server.sin_port = htons(port);
    conn->gdb_server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
 
    if (connect(conn->sockfd, (struct sockaddr *)&conn->gdb_server, sizeof(struct sockaddr_in)) == -1) {
        debugprintf_ff(1, "call to connect() has failed with -1. GDB server is not listening on port %d??\n", port);
        return ERR_GDBSERVER_CONNECT;
    }

    return ERR_NONE;
}

uint32_t
rsp_close(struct rsp_connection *conn)
{
    debugprintf_ff(1, "rsp_close\n");
    return close(conn->sockfd);
}

uint32_t
rsp_send_command(struct rsp_connection *conn, struct proto_command *proto_cmd)
{
    uint32_t        ret_code = ERR_NONE;
    struct rsp_pkt  pkt; 

    if (conn == NULL) {
        debugprintf_ff_null(1, "struct rsp_connection *conn\n");
        return ERR_NULL_POINTER;
    }

    if (proto_cmd == NULL) {
        debugprintf_ff_null(1, "struct proto_command *proto_cmd\n");
        return ERR_NULL_POINTER;
    }

    if (proto_cmd->st == NULL && (proto_cmd->type == PROTO_SET_MEM || proto_cmd->type == PROTO_SET_REG)) {
        debugprintf_ff_null(1, "proto_cmd->st\n");
        return ERR_NULL_POINTER;
    }

    debugprintf_ff(4, "conn=%p proto_cmd=%p proto_cmd->st=%p\n", conn, proto_cmd, proto_cmd->st);


    /* Create packet */
    memset(&pkt, 0, sizeof(pkt));
    pkt.index = 0;

    if (proto_cmd->type == PROTO_SET_MEM)
        debugprintf_ff(2, "Memory value is (in rsp-send-cmd) %ld\n", storage_get_value(proto_cmd->st));

    
    ret_code = _rsp_create_packet(conn, proto_cmd, &pkt);
    ON_ERROR_DEBUGMSG_AND_RETURN_ECODE(ret_code, "Packet creating has failed\n");


    /* Sending packet to target */
    debugprintf_ff(1, "sending this packet ''%s'' to target .. \n", pkt.data);

    #ifndef DEBUG
    printf("-> ''%s'' to target \n", pkt.data);
    #endif

    ret_code = write(conn->sockfd, &pkt.data, strlen(pkt.data));
    if (ret_code == strlen(pkt.data)) {
        debugprintf(1, "ok\n");
        return ERR_NONE;
    }
    else {
        debugprintf(1, "err\n");
        return ERR_WRITE;
    }
}

uint32_t
rsp_recv_command(struct rsp_connection *conn, proto_cmd_type *response, struct storage *st)
{
    int             ret_code = ERR_NONE;
    struct rsp_pkt  pkt;
    proto_cmd_type  _cmd_response = PROTO_OK;
    fd_set          input;
    struct timeval  timeout;


    if (conn == NULL) {
        debugprintf_ff_null(1, "struct storage *st, or, struct rsp_connection *conn \n");
        return ERR_NULL_POINTER;
    }


    /* prepare structure for to-be received packet */
    memset(&pkt, 0, sizeof(pkt));
    pkt.index = 0;


    /* stuff for select call */
    FD_ZERO(&input);
    FD_SET(conn->sockfd, &input);
    
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;


    /* Wait for a packet to arrive, store contents of the packet to storage structure */
    int ret_sel = select(conn->sockfd + 1, &input, NULL, NULL, &timeout);
    if (ret_sel == -1) {
        debugprintf_ff(4, "Select returned -1, \n");
        return ERR_SELECT;
    }
    else if (ret_sel > 0) {
        pkt.length = read(conn->sockfd, &pkt.data, sizeof(pkt.data) );

        debugprintf_ff(1, "Received packet(%d) ''%s'' \n", pkt.length, pkt.data);

        #ifndef DEBUG
        if (pkt.length < 10) {
            printf("<- ''%s'' from target \n\n", pkt.data);
        }
        else {
            printf("<- ''");
            for(int i=0; i < 10; i++) 
                printf("%c",pkt.data[i]);
            printf("...too long''\n\n");
        }
        #endif


        int idx = 0;
        bool dolar = false;
        for (; idx < pkt.length; idx++) {
            if (pkt.data[idx] == '$')
                break;
        }

        if (dolar == true) {
            switch (pkt.data[idx])
            {
            case 'O':
                debugprintf_ff(1, "OK packet;\n");
                _cmd_response = PROTO_OK;
            break;

            case 'E':
                debugprintf_ff(1, "Err packet;\n");
                _cmd_response = PROTO_ERR;
            break;

            case 'T':
                debugprintf_ff(1, "T stop-reply packet;\n");
                _cmd_response = PROTO_OK;
            break;

                
            
            }
        } 
        else {
            switch (pkt.data[0])
            {
            case '+':
                debugprintf_ff(4, "+ packet;\n");
                _cmd_response = PROTO_OK;
            break;

            case '-':
                debugprintf_ff(4, "- packet;\n");
                _cmd_response = PROTO_ERR;
            break;
            }

            if (conn->prev_cmd == PROTO_GET_MEM || conn->prev_cmd == PROTO_GET_REG) {
                    if (st == NULL) {
                        debugprintf_ff_null(4, "struct storage *st\n");
                        return ERR_NULL_POINTER;
                    }
                    debugprintf_ff(1, "store packet content\n");
                    ret_code = _rsp_store_packet_content(&pkt, st);
                }


        }   
    }
    else { // timeout
        FD_ZERO(&input);
        debugprintf_ff(4, "Timeout. No response from target.\n");
        return ERR_SELECT_TIMEOUT;
    }
    *response = _cmd_response;
    return ret_code;
}