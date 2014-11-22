int SendCurlRequest(char *url, char *body, void (*ptrFunc)(char *));
void encodeAuthToken(const char *unencToken, char *encToken);
