CC=gcc

make: panox.c Crypto.c
	gcc -o panox panox.c crypto.c -lcurl -lrt -ljansson -lmcrypt
