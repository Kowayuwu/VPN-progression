#pragma once
#include <stdint.h>

/*
RFC doc: https://www.ietf.org/rfc/rfc1035.txt
*/ 

// 12 octects
typedef struct dns_header {
    uint16_t id;         // Just to let you match your query

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t rd :1;       // Recursion Desired, directs the name server to pursue the query recursively
    uint8_t tc :1;       // TrunCation
    uint8_t aa :1;       // Authoritative Answer
    uint8_t opcode :4;   // Purpose of message
    uint8_t qr :1;       // Query (0) / Response (1) flag

    uint8_t rcode :4;    // Response code, 0 indicates no error, 1-5 means not good (see RFC for more details)
    uint8_t z :3;        // Reserved for future use (IN USE now but I won't care about this in this simple project)
    uint8_t ra :1;       // Recursion Available? this bit is set in response
#else
    uint8_t qr :1;       
    uint8_t opcode :4;   
    uint8_t aa :1;       
    uint8_t tc :1;       
    uint8_t rd :1;       

    uint8_t ra :1;       
    uint8_t z :3;        
    uint8_t rcode :4;    
#endif

    uint16_t qdcount;    // Number of entries in the question section
    uint16_t ancount;    // Number of resource records in the answer section
    uint16_t nscount;    // Number of name server resource records in the authority records section
    uint16_t arcount;    // Number of resource records in the additional records section
} dns_header_t;

