#include <mcrypt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int Encrypt(char *plain);
void Decrypt(const char *encrypted, unsigned char *decrypted);
void BinaryToHex(unsigned char *binary, char *hex, int len);
