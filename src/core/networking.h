/* deshi Networking Module

Notes:
    Currently everything in this interface deals with UDP. We decided on using UDP over TCP 
    because it's simpler, faster, and allows for determining your own rules with communcating
    over networks. I (sushi) am not opposed to someone else implementing TCP though. 

    This interface currently only deals with IPv4 address. I (sushi) am also not oppsed to
    someone else implementing support for IPv6. 

    It is highly recommended to use this interface either with multi-threading or asyncronously.
    Sockets have the option to not be blocking to allow for asyncronous operation. (If I'm
    understanding what async stuff is here).

    There is no error returning because error logging is handled internally. Therefore
    the Logger module must be initialized before using this module.
        NOTE(sushi) I'm not sure if this is how I'd like this to be, but it will stay
                    so for now
    
    Functions that execute successfully return 0 instead of a non-zero number because I (sushi)
    prefer being able to do if(function()) to check for error. If this design choice is unpopular
    I will go through and change it. Exceptions include the send and recv functions as they
    must return how many bytes were sent or recieved. They instead return -1.


Index:
    @net_types
        netSocket | struct
        netTCPHeader | struct
        netIPHeader | struct
    @net_state
        net_init() -> void
        net_deinit() -> void
    @net_socket
        net_socket_open() -> u64
        net_socket_close() -> void
    @net_address
        net_address_init() -> u64
        net_address_str() -> str8

References:
    https://github.com/Smilex/zed_net - interface; some winsock stuff
    https://beej.us/guide/bgnet/ - info about how sockets and networking works as well as programming them

*/

#pragma once
#ifndef DESHI_NETWORKING_H
#define DESHI_NETWORKING_H

#include "kigu/common.h"
#include "kigu/unicode.h"

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @net_types

struct netSocket{
    s32 handle; // handle to OS socket object; win32: SOCKET
    b32 blocking; // determines if the socket blocks when sending/recieving information
    b32 opened;   // indicates if the socket has been opened or not 
};

struct netAddress{
    void* handle;
    u32   host;
    u8    port;
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
// returns 0 if successful or -1 if an error occurs (use net_get_last_error()) 
void net_init();
// deinitializes the networking module
// returns 0 if successful or -1 if an error occurs (use net_get_last_error()) 
void net_deinit();

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @net_socket

//initializes a socket and binds it to a port
// |   socket: a ptr to a netSocket object to bind to a given port
// |     port: port to bind socket to; use 0 to bind to random port
// | blocking: determines if the socket should be blocking or not
// returns 0 if successful, non-zero otherwise
u64 net_socket_open(netSocket* socket, u8 port, b32 blocking);

//closes a previously opened socket
// socket: a ptr to a netSocket obj. if it has not yet been opened the function does nothing
void net_socket_close(netSocket* socket);

//sends some data over an opened socket
// |      socket: a ptr to a netSocket that has been opened using net_socket_open()
// | destination: a netAddress that has been initialized using net_get_address()
// |        data: a ptr to the data to be sent over the network
// |        size: the size of the data to be sent in bytes
// returns 0 if successful, non-zero otherwise
u64 net_socket_send(netSocket* socket, netAddress destination, void* data, s32 size);

//receieves a specified amount of data from sender
//you do not specify the sender you want to recieve from since this is UDP
//we just tell you who sent it
// | socket: a ptr to a netSocket that has been opened with net_socket_open()
// | sender: a ptr to a netAddress where info about the sender of the data is stored
// |   data: an allocated buffer that data recieved is stored in.
// |   size: the amount of data in bytes expected 
// returns the amount of data recieved in bytes, -1 otherwise
u64 net_socket_recv(netSocket* socket, netAddress* sender, void* data, s32 size);

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @net_address

//initializes a netAddress object with information about the given host and port
// | address: a ptr to a netAddress struct to be filled
// |    host: a string containing etiher a decimal IP address ("127.0.0.1") or a human readable name
// |          such as "localhost" or "youtube.com"
// |    port: the port number
// returns 0 if successful, non-zero otherwise
u64 net_address_init(netAddress* address, str8 host, str8 port);

//gets a formatted string representing the host address of a netAddress object
// |      host: a netAddress object that has been filled either by net_scoket_recv() or net_get_address()
// | incl_port: whether or not to include port in output
// returns an allocated str8 representing the host. it is allocated using temp memory, so it is not necessary to free it unless you dont free temp mem
str8 net_address_str(netAddress address, b32 incl_port);


#endif //DESHI_NETWORKING_H