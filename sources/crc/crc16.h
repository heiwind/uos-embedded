/*
 * Computing 16-bit checksum (x16 + x5 + x2 + 1).
 */
#ifndef UOS_CRC16_H
#define UOS_CRC16_H

#ifdef __cplusplus
extern "C" {
#endif



#define CRC16_INIT	0x0000  /* Initial checksum value */
#define CRC16_GOOD	0x0000	/* Good final checksum value */

unsigned short crc16 (unsigned short sum, unsigned const char *buf,
	unsigned len);

unsigned short crc16_byte (unsigned short sum, unsigned char byte);



#ifdef __cplusplus
}
#endif


#endif //UOS_CRC16_H
