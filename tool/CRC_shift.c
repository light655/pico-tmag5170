#include <stdio.h>

unsigned generateCRC(unsigned data) {
    unsigned CRC = 0xf;

    for(int i = 0; i < 32; i++) {
        printf("CRC: %X\n", CRC);

        unsigned inv = ((data & 0x80000000) >> 31) ^ ((CRC & 0x8) >> 3);
        unsigned poly = (inv << 1) | inv;
        unsigned XORed = (CRC << 1) ^ poly;
        CRC = XORed & 0xf;
        data <<= 1;
    }    

    return CRC;
}

int main(void) {
	unsigned read, offset, data, command;

	printf("Read? (0/1) ");
	scanf("%u", &read);
	printf("offset(hex): ");
	scanf("%x", &offset);
	printf("data(hex): ");
	scanf("%x", &data);
	printf("command(hex): ");
	scanf("%x", &command);

	unsigned before_CRC = (read << 31) | (offset << 24) | (data << 8) | command;
	printf("Before CRC: %.8X\n", before_CRC);

    unsigned CRC = generateCRC(before_CRC);
	printf("After CRC: %.8X\n", before_CRC | CRC);

    return 0;
}
