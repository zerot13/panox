#include "player.h"
#include "login.h"

struct Auth *auth;

main()
{
	allocAuth(&auth);
	Login(auth);

	GetStationList(auth);

	freeAuth(auth);
}

