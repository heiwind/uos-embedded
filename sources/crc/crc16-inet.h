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

unsigned short crc16_inet (unsigned short sum, unsigned const char *buf,
	unsigned short len);
unsigned short crc16_inet_header (const unsigned char *src, const unsigned char *dest,
	unsigned char proto, unsigned short proto_len);
unsigned short crc16_inet_byte (unsigned short sum, unsigned char data);

/* makes memcpy dst<-src and crc16_inet check sum calculates at once 
 * */
unsigned short memcpy_crc16_inet (unsigned short sum
        , unsigned char *dst, unsigned const char *buf, unsigned len);



#ifdef __cplusplus
}
#endif

#endif // UOS_CRCINET_H
