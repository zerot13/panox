make: panox.c crypto.c
	gcc -o panox panox.c crypto.c -lcurl -lrt -ljansson -lmcrypt
