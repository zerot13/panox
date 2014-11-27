#include "crypto.h"

void HexToBinary(const char *hex, unsigned char *binary)
{
	int length = strlen(hex)/2;
	int count;
	for(count = 0; count < length; count++)
	{
		sscanf(hex, "%2hhx", &binary[count]);
		hex += 2 * sizeof(char);
	}
}

int Encrypt(char *plain)
{
	int len = strlen(plain) + 8 - (strlen(plain) % 8);
	MCRYPT mc = mcrypt_module_open("blowfish", NULL, "ecb", NULL);
	if(mc == MCRYPT_FAILED)
	{
		printf("Failed\n");
	}
	char *key = "6#26FRL$ZWD";
	mcrypt_generic_init(mc, key, 11, "");
	mcrypt_generic(mc, plain, len);
	return len;
}

void Decrypt(const char *encrypted, unsigned char *decrypted)
{
	int encLength = strlen(encrypted);
	HexToBinary(encrypted, decrypted);

	MCRYPT mc = mcrypt_module_open("blowfish", NULL, "ecb", NULL);
	if(mc == MCRYPT_FAILED)
	{
		printf("Failed\n");
	}
	char *key = "R=U!LH$O2B#";
	mcrypt_generic_init(mc, key, 11, "");
	mdecrypt_generic(mc, decrypted, encLength/2);
}

void BinaryToHex(unsigned char *binary, char *hex, int len)
{
	int count;
	for(count = 0; count < len; count++)
	{
		sprintf(hex+count*2, "%02x", binary[count]);
	}
	hex[len*2 + 1] = '\0';
}
