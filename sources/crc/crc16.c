#include <runtime/lib.h>
#include <crc/crc16.h>

#ifdef __AVR__

static const unsigned char polyA001Nible_tab [32] = {
	0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00,

	0x00, 0xCC, 0xD8, 0x14, 0xF0, 0x3C, 0x28, 0xE4,
	0xA0, 0x6C, 0x78, 0xB4, 0x50, 0x9C, 0x88, 0x44,
};

static unsigned short polyA001Nible_tab (unsigned char nibble)
{
	unsigned char low = FETCH_BYTE (&poly_tab_data [nibble]);
	unsigned char high = FETCH_BYTE (&poly_tab_data [16 + nibble]);

	return low | high << 8;
}

#else

// Nibble lookup for 0xA001 polynomial
static const unsigned short polyA001_tab16_data [16] = {
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
	0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400,
};

static unsigned short polyA001Nible_tab (unsigned char nibble)
{
	return polyA001_tab16_data [nibble];
}

#endif

unsigned short 
crc16_byte (unsigned short sum, unsigned char byte)
{
#if 1
    //code from https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/Flat.aspx?RootFolder=%2Fpublic%2FSTe2ecommunities%2Fmcu%2FLists%2Fcortex_mx_stm32%2FCRC%20computation&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=7800
    sum = sum ^ byte;
    sum = (sum>>4) ^ polyA001Nible_tab (sum & 0xF);
    sum = (sum>>4) ^ polyA001Nible_tab (sum & 0xF);
#else
	/* compute checksum of lower four bits of byte */
	sum = (sum >> 4) ^ polyA001Nible_tab (sum & 0xF) ^ polyA001Nible_tab (byte & 0xF);

	/* now compute checksum of upper four bits of byte */
	sum = (sum >> 4) ^ polyA001Nible_tab (sum & 0xF) ^ polyA001Nible_tab (byte >> 4);
#endif
	return sum;
}

/*
 * Calculate a new sum given the current sum and the new data.
 * Use 0xffff as the initial sum value.
 * Do not forget to invert the final checksum value.
 */
unsigned short 
crc16 (unsigned short sum, const void* src, unsigned len)
{
    unsigned const char *buf = (unsigned const char *)src;
	if (len) do
		sum = crc16_byte (sum, *buf++);
	while (--len);
	return sum;
}


#ifdef DEBUG_CRC16
#include <stdio.h>

void test (unsigned const char *buf, unsigned char len)
{
	unsigned char i;
	unsigned short sum;

	for (i=0; i<len-2; ++i)
		printf ("%02x ", buf[i]);
	sum = crc16 (CRC16_INIT, buf, len);
	printf ("(%02x %02x) - %04x", buf[len-2], buf[len-1], sum);
	if (sum == CRC16_GOOD)
		printf (" - GOOD");
	printf ("\n");
}

int main ()
{
	unsigned char array [] = "\x10\x20\x00\x00";
	unsigned short sum;
	int i;

	test (array, 4);
	sum = crc16 (CRC16_INIT, array, 2);
	array[2] = (unsigned char) sum;
	array[3] = (unsigned char) (sum >> 8);
	test (array, 4);

	sum = CRC16_INIT;
	for (i=0; i<4; ++i)
		sum = crc16_byte (sum, array[i]);
	for (i=0; i<2; ++i)
		printf ("%02x ", array[i]);
	printf ("(%02x %02x) - %04x", array[2], array[3], sum);
	if (sum == CRC16_GOOD)
		printf (" - GOOD");
	printf ("\n");

	return 0;
}
#endif /* DEBUG_CRC16 */
