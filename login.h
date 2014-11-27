#include <jansson.h>

struct Credentials
{
	char *username;
	char *password;
};

struct SyncTime
{
	long originalSync;
	long originalTime;
};

struct Auth
{
	char *token;
	char *escapedToken;
	char *partnerId;
	char *userId;
	struct SyncTime *sync;
	struct Credentials *login;
};

void Login(struct Auth *auth);
void allocAuth(struct Auth **auth);
void freeCredentials(struct Credentials *login);
void freeAuth(struct Auth *auth);
long GetCurrentSyncTime(struct SyncTime *sync);
