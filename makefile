make: panox.c crypto.c login.c curl.c
	gcc -o panox panox.c crypto.c login.c curl.c -lcurl -lrt -ljansson -lmcrypt
