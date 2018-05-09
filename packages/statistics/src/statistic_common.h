#ifndef __STATISTIC_COMMON_H__
#define __STATISTIC_COMMON_H__

#include <netinet/in.h>

#ifndef MAX_RESOURCE_TO_MONITOR
#define MAX_RESOURCE_TO_MONITOR 10
#endif
//#define MAX_PROTOCOL_TO_MONITOR 21

#define STATISTICS_PATH "/var/run/.statistic_path"

#define PROTOCOL_MSG 1
#define PORT_MSG 2
#define IPV4_MSG 4
#define IPV6_MSG 8

typedef enum {
	INDEX_IP = 0,
	INDEX_SMALL_PKT,
	INDEX_TCP_SYN,
	INDEX_TCP_PSH,
	INDEX_TCP_NOF,
	INDEX_TCP_UP,
	INDEX_TCP_DOWN,
	INDEX_TCP,
	INDEX_UDP_UP,
	INDEX_UDP_DOWN,
	INDEX_UDP,
	INDEX_ICMP_UP,
	INDEX_ICMP_DOWN,
	INDEX_ICMP,
	INDEX_IGMP_UP,
	INDEX_IGMP_DOWN,
	INDEX_IGMP,
	INDEX_SCTP_UP,
	INDEX_SCTP_DOWN,
	INDEX_SCTP,
	INDEX_ARP,
	INDEX_RPC,
	INDEX_HTTP,
	INDEX_FTP,
	INDEX_IMAP,
	INDEX_SNMP,
	INDEX_TELNET,
	INDEX_DNS,
	INDEX_SMTP,
	INDEX_RIP,
	INDEX_RIPNG,
	INDEX_TFTP,
	INDEX_NNTP,
	INDEX_NFS,
	INDEX_NETBIOS,
	INDEX_POP2,
	INDEX_POP3,
	INDEX_HTTPS,
	ProtoTypeIndexCount
} ProtoTypeIndex;

#define MAX_PROTOCOL_TO_MONITOR ProtoTypeIndexCount


/* struct to collect packet statistics for protocol */
typedef struct _ProtocolPacketCount
{
    uint8_t is_set; //this struct has been set?
    char name[16];//protocol name
    uint64_t count;//total number of packets originating from this protocol
    uint32_t Bps; //byte per second
} ProtocolPacketCount;


/* struct to collect packet statistics for a given port */
typedef struct _PortPacketCount
{
    uint8_t is_set; //this struct has been set?
    int16_t port;//port number
    uint64_t count;//total number of packets originating from this port
    uint32_t Bps; //byte per second
} PortPacketCount;

/* struct to collect packet statistics for a given IPv4 address */
typedef struct _IPv4AddressPacketCount
{
    uint8_t is_set;
    struct in_addr ipv4_addr;
    uint64_t count;
    uint32_t Bps; //byte per second
} IPv4AddressPacketCount;

/* struct to collect packet statistics for a given IPv6 address */
typedef struct _IPv6AddressPacketCount
{
    uint8_t is_set;
    struct in6_addr ipv6_addr;
    uint64_t count;
    uint32_t Bps; //byte per second
} IPv6AddressPacketCount;


struct _statisics_msg {
    struct _msg_hdr {
        uint32_t msg_len; //the length of message
        uint32_t msg_type; //message type could be protocal message,port message,ipv4 message or ipv6 message
    }hdr;
    void *content; //message data
};



#endif
