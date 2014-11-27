make: panox.c crypto.c login.c curl.c player.c
	gcc -o panox panox.c crypto.c login.c curl.c player.c -lcurl -lrt -ljansson -lmcrypt
