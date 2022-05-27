/* deshi Networking Module

Index:
    @net_types
        netError | enum
        netContext | struct
        netSocket | struct
        netTCPHeader | struct
        netIPHeader | struct
    @net_state
        net_init() -> void
        net_deinit() -> void
        net_get_state() -> netState

*/

#pragma once
#ifndef DESHI_NETWORKING_H
#define DESHI_NETWORKING_H

#include "kigu/common.h"

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @net_types

typedef u64 netError; enum {
    netError_COUNT,
};

struct netState{
    netError last_error;

};

struct netSocket{
    void* handle;
};

struct netAddress{
    void* handle;
};

struct netIPHeader{
    u8  version : 4;          // indicates protocol version (ipv4 or ipv6)
    u8  ihl : 4;              // ip header length in 32 bit words; min: 5; max: 15;
    u8  type;                 // first 6 bits are DSCP, last 2 are ECN
    u16 length;               // total length of the packet in bytes; 
    u16 id;                   // all fragments of a packet have the same id number to help receiever assemble packets
    u16 flags : 4;            // first 3 bits unused, last bit indicates if th receiever should expect more fragments
    u16 fragment_offset : 12; // indicates where in the packet this fragment belongs
    u8  time_to_live;         // counter that limits the lifetime of a packet and is decreased by every router on the way to the dest; when depleted the router discards it and sends an ICMP error to sender
    u8  protocol;             // inicates next level protocol used in data portion of packet
    u16 header_checksum;      // checksum of only the header
    u32 source_addr;          // source ip address
    u32 dest_addr;            // dest ip address
};

struct netTCPHeader{
    u8  source_port;     // source port
    u8  dest_port;       // destination port
    u32 sequence_num;    // first data byte in this segment; if SYN is set then the seq num is the init seq num (ISN) and the first data byte is ISN + 1  
    u32 acknowledge_num; // if ACK is set this field contains the value of the next seq number the sender of the of the segment is expecting to recieve
    u16 data_offset : 4; // indiciates size of tcp header in 32 bit words; min: 5; max: 15;
    u16 reserved : 3;    // must be 0
    u16 NS  : 1;         // experimental: concealment protection
    u16 CWR : 1;         // congestion window reduced; used for congestion control mechanism
    u16 ECE : 1;         // ECN-Echo; used for congestion control mechanism
    u16 URG : 1;         // indicates urgent pointer field is significant
    u16 ACK : 1;         // indicates acknoledgement field is significant
    u16 PSH : 1;         // asks to push the buffered data to the receieving applicationand not wait for buffer to be filled
    u16 RST : 1;         // reset connection
    u16 SYN : 1;         // syncronize sequence numbers
    u16 FIN : 1;         // no more data from sender
    u16 window_size;     // used for flow control and window scaling, allowing sender to signal the number of window size units they are currently willing to recieve
    u16 checksum;        // for error detection; header payload and a pseudo header are used 
    u16 urgent_ptr;      // if URG is set, this points to the sequence number of the byte following the urgent data
};

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @net_state

// initializes the networking module
void net_init();
// deinitializes the networking module
void net_deinit();
// returns the state of the networking module
netState net_get_state();

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @net_socket

// initializes a socket
netSocket net_socket_init();




#endif