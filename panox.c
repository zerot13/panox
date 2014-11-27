#include "player.h"
#include "login.h"

main()
{
	struct Auth *auth;
	allocAuth(&auth);
	Login(auth);

	GetStationList(auth);

	freeAuth(auth);
}

