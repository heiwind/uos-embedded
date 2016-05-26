#ifndef UOS_CRCINET_H
#define UOS_CRCINET_H

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Computing 16-bit checksum (IP-compatible).
 */
#define CRC16_INET_INIT		0x0000  /* Initial checksum value */
#define CRC16_INET_GOOD		0xffff	/* Good final checksum value */

typedef unsigned short crc16_inet_t;
crc16_inet_t crc16_inet (crc16_inet_t sum, const void *buf, unsigned len);
crc16_inet_t crc16_inet_header (const void*src, const void* dest
                                , unsigned char proto
                                , unsigned proto_len);
crc16_inet_t crc16_inet_byte (crc16_inet_t sum, unsigned char data);

/* makes memcpy dst<-src and crc16_inet check sum calculates at once 
 * */
crc16_inet_t memcpy_crc16_inet (crc16_inet_t sum
        , void* dst, const void* buf, unsigned len);



#ifdef __cplusplus
}
#endif

#endif // UOS_CRCINET_H
