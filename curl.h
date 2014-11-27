#include "player.h"

int SendCurlRequest(char *url, char *body, char **response);
int AudioRequest(char *url, SongData *response);
void encodeAuthToken(const char *unencToken, char *encToken);
