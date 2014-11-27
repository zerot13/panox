#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "login.h"
#include "curl.h"
#include "crypto.h"

void ReadLoginFromFile(struct Credentials *login)
{
	login->username = malloc(100);
	login->password = malloc(100);
	FILE *loginFile = fopen("login.pwd", "r");
	if(loginFile == NULL)
	{
		printf("Could not open credential file.\n");
		exit(EXIT_FAILURE);
	}
	if(fscanf(loginFile, "%s\n", login->username) != 1)
	{
		printf("Could not read username.\n");
		exit(EXIT_FAILURE);
	}
	if(fscanf(loginFile, "%s\n", login->password) != 1)
	{
		printf("Could not read password.\n");
		exit(EXIT_FAILURE);
	}

	fclose(loginFile);
}

void allocAuth(struct Auth **auth)
{
	*auth = malloc(sizeof(struct Auth));
	(*auth)->sync = malloc(sizeof(struct SyncTime));
	(*auth)->login = malloc(sizeof(struct Credentials));
}

void freeCredentials(struct Credentials *login)
{
	free(login->username);
	free(login->password);
	free(login);
}

void freeAuth(struct Auth *auth)
{
	free(auth->token);
	free(auth->escapedToken);
	free(auth->partnerId);
	free(auth->userId);
	freeCredentials(auth->login);
	free(auth->sync);
	free(auth);
}

void DecryptSyncTime(const char *st, struct SyncTime *sync)
{
	int syncLength = strlen(st);
	unsigned char *decrypted = malloc(syncLength/2);
	Decrypt(st, decrypted);

	int ii;
	int index = 0;
	unsigned char *plainTime = malloc(syncLength/2 - 4);
	for(ii = 4; ii < syncLength/2; ii++)
	{
		if(decrypted[ii] > 47)
		{
			plainTime[index] = decrypted[ii];
			index++;
		}
	}

	sync->originalSync = strtol(plainTime, NULL, 10);
	struct timeval curTime;
	gettimeofday(&curTime, NULL);
	sync->originalTime = curTime.tv_sec;
	free(decrypted);
	free(plainTime);
}

long GetCurrentSyncTime(struct SyncTime *sync)
{
	struct timeval ct;
	gettimeofday(&ct, NULL);
	return sync->originalSync + ct.tv_sec - sync->originalTime;
}

void ParsePartnerLogin(json_t *root, struct Auth *auth)
{
	json_t *result = json_object_get(root, "result");
	json_t *partnerAuth = json_object_get(result, "partnerAuthToken");
	auth->token = malloc(json_string_length(partnerAuth));
	auth->escapedToken = malloc(json_string_length(partnerAuth)*3);
	encodeAuthToken(json_string_value(partnerAuth), auth->escapedToken);
	strcpy(auth->token, json_string_value(partnerAuth));
	json_t *pid = json_object_get(result, "partnerId");
	auth->partnerId = malloc(json_string_length(pid));
	strcpy(auth->partnerId, json_string_value(pid));
	json_t *syncTime = json_object_get(result, "syncTime");
	DecryptSyncTime(json_string_value(syncTime), auth->sync);
}

void PartnerLoginReturn(char *response, struct Auth *auth)
{
	json_error_t error;
	json_t *root = json_loads(response, 0, &error);
	json_t *stat = json_object_get(root, "stat");
	if(strcmp(json_string_value(stat),"ok") != 0)
	{
		printf("OK was not returned on partner login.\n");
		printf("%s\n", response);
		return;
	}
	ParsePartnerLogin(root, auth);
}

void PartnerLogin(struct Auth *auth)
{
	char url[] = "https://tuner.pandora.com/services/json/?method=auth.partnerLogin";
	char body[] = "{\"username\":\"android\",\"password\":\"AC7IBG09A3DTSYM4R41UJWL07VLN8JI7\",\"deviceModel\":\"android-generic\",\"version\":\"5\"}";
	char *response = malloc(1);
	response[0] = '\0';
	SendCurlRequest(url, body, &response);
	PartnerLoginReturn(response, auth);
}

void ParseUserLogin(json_t *root, struct Auth *auth)
{
	json_t *result = json_object_get(root, "result");
	json_t *userAuth = json_object_get(result, "userAuthToken");
	auth->token = realloc(auth->token, json_string_length(userAuth));
	auth->escapedToken = realloc(auth->escapedToken, json_string_length(userAuth)*3);
	encodeAuthToken(json_string_value(userAuth), auth->escapedToken);
	strcpy(auth->token, json_string_value(userAuth));
	json_t *uid = json_object_get(result, "userId");
	auth->userId = malloc(json_string_length(uid));
	strcpy(auth->userId, json_string_value(uid));
}

void UserLoginReturn(char *response, struct Auth *auth)
{
	json_error_t error;
	json_t *root = json_loads(response, 0, &error);
	json_t *stat = json_object_get(root, "stat");
	if(strcmp(json_string_value(stat),"ok") != 0)
	{
		printf("OK was not returned on user login.\n");
		printf("%s\n", response);
		return;
	}
	ParseUserLogin(root, auth);
}

void UserLogin(struct Auth *auth)
{
	char url[200] = "https://tuner.pandora.com/services/json/?method=auth.userLogin&partner_id=";
	strcat(url, auth->partnerId);
	strcat(url, "&auth_token=");
	strcat(url, auth->escapedToken);	

	unsigned char body[500] = "{\"loginType\":\"user\",\"username\":\"";
	strcat(body, auth->login->username);
	strcat(body, "\",\"password\":\"");
	strcat(body, auth->login->password);
	strcat(body, "\", \"includePandoraOneInfo\": true,\"includeAdAttributes\": true,\"includeSubscriptionExpiration\": true, \"partnerAuthToken\":\"");
	strcat(body, auth->token);
	strcat(body, "\",\"syncTime\":");
	char syncStr[11];
	sprintf(syncStr, "%ld", auth->sync->originalSync);
	strcat(body, syncStr);
	strcat(body, "}");
	
	int encLen = Encrypt(body);
	char encBody[encLen * 2 + 1];
	BinaryToHex(body, encBody, encLen);

	char *response = malloc(1);
	response[0] = '\0';
	SendCurlRequest(url, encBody, &response);
	UserLoginReturn(response, auth);
}

void Login(struct Auth *auth)
{
	ReadLoginFromFile(auth->login);
	PartnerLogin(auth);
	UserLogin(auth);
}
