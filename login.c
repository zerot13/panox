#include <stdio.h>
#include <stdlib.h>
#include "login.h"
#include "curl.h"

void ReadLoginFromFile(struct credentials *login)
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

void freeCredentials(struct credentials *login)
{
	free(login->username);
	free(login->password);
	free(login);
}

void PartnerLogin()
{
	char url[] = "https://tuner.pandora.com/services/json/?method=auth.partnerLogin";
	char body[] = "{\"username\":\"android\",\"password\":\"AC7IBG09A3DTSYM4R41UJWL07VLN8JI7\",\"deviceModel\":\"android-generic\",\"version\":\"5\"}";
	SendCurlRequest(url, body, PartnerLoginReturn);
}

void PartnerLoginReturn(char *response)
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
	ParsePartnerLogin(root);
}

void ParsePartnerLogin(json_t *root)
{
	json_t *result = json_object_get(root, "result");
	json_t *partnerAuth = json_object_get(result, "partnerAuthToken");
	partnerAuthToken = malloc(json_string_length(partnerAuth));
	escapedPAToken = malloc(json_string_length(partnerAuth)*3);
	encodeAuthToken(json_string_value(partnerAuth), escapedPAToken);
	strcpy(partnerAuthToken, json_string_value(partnerAuth));
	json_t *pid = json_object_get(result, "partnerId");
	partnerId = malloc(json_string_length(pid));
	strcpy(partnerId, json_string_value(pid));
	json_t *syncTime = json_object_get(result, "syncTime");
	DecryptSyncTime(json_string_value(syncTime));
}

void DecryptSyncTime(const char *st)
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

	originalSync = strtol(plainTime, NULL, 10);
	struct timeval curTime;
	gettimeofday(&curTime, NULL);
	originalTime = curTime.tv_sec;
	free(decrypted);
	free(plainTime);
}
