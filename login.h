#include <jansson.h>

struct credentials
{
	char *username;
	char *password;
};

void ReadLoginFromFile(struct credentials *log);
void freeCredentials(struct credentials *login);
void PartnerLogin();
void PartnerLoginReturn(char *response);
void ParsePartnerLogin(json_t *root);
