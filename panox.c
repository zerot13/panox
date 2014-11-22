#include <sys/time.h>
#include "crypto.h"
#include "login.h"
#include "curl.h"

long originalSync;
long originalTime;
char *partnerAuthToken;
char *escapedPAToken;
char *partnerId;
char *userAuthToken;
char *escapedUAToken;
char *userId;
struct credentials *login;

struct Station
{
	char *id;
	char *name;
};

void BinaryToHex(unsigned char *binary, char *hex, int len);
void UserLoginReturn(char *response);
void GetStationListReturn(char *response);
void DecryptSyncTime(const char *st);
void encodeAuthToken(const char *unencToken, char *encToken);
void UserLogin();
void ParseUserLogin(json_t *root);
void GetStationList();
struct Station ParseStationList(json_t *root);
long GetCurrentSyncTime();
void initStationList(struct Station **stationList, int count);

main()
{
	login = malloc(sizeof(struct credentials));
	ReadLoginFromFile(login);
	PartnerLogin();
	UserLogin();
	GetStationList();
	free(partnerAuthToken);
	free(escapedPAToken);
	free(partnerId);
	free(userAuthToken);
	free(escapedUAToken);
	free(userId);
	freeCredentials(login);
}

void GetStationList()
{
	char url[200] = "http://tuner.pandora.com/services/json/?method=user.getStationList&partner_id=";
	strcat(url, partnerId);
	strcat(url, "&auth_token=");
	strcat(url, escapedUAToken);
	strcat(url, "&user_id=");
	strcat(url, userId);
	
	char body[500] = "{\"includeStationArtUrl\": true,\"userAuthToken\":\"";
	strcat(body, userAuthToken);
	strcat(body, "\",\"syncTime\":");
	sprintf(body, "%s%ld", body, GetCurrentSyncTime());
	strcat(body, "}");
	
	int encLen = Encrypt(body);
	char encBody[encLen * 2 + 1];
	BinaryToHex(body, encBody, encLen);
	printf("GetStationList: %i\n", SendCurlRequest(url, encBody, GetStationListReturn));
}

void GetStationListReturn(char *response)
{
	json_error_t error;
	json_t *root = json_loads(response, 0, &error);
	if(!root)
	{
		printf("Error: %s\n", error.text);
		exit(EXIT_FAILURE);
	}
	json_t *stat = json_object_get(root, "stat");
	if(strcmp(json_string_value(stat),"ok") != 0)
	{
		printf("OK was not returned while getting station list.\n");
		printf("%s\n", response);
		exit(EXIT_FAILURE);
	}
	ParseStationList(root);

}

struct Station GetStationData(json_t *json)
{
	struct Station *station = malloc(sizeof(struct Station));
	json_t *name = json_object_get(json, "stationName");
	json_t *id = json_object_get(json, "stationId");
	station->name = malloc(json_string_length(name));
	station->id = malloc(json_string_length(id));
	strcpy(station->name, json_string_value(name));
	strcpy(station->id, json_string_value(id));
	return *station;
}

struct Station ParseStationList(json_t *root)
{
	json_t *stations = json_object_get(json_object_get(root, "result"), "stations");
	int ii;
	int count = json_array_size(stations);
	printf("Select a station (1-%i):\n", count - 1);
	for(ii = 0; ii < count - 1; ii++)
	{
		json_t *curStation = json_array_get(stations, ii + 1);
		printf("%i: %s\n", ii + 1, json_string_value(json_object_get(curStation, "stationName")));
	}
	int selection;
	scanf("%i", &selection);
	return GetStationData(json_array_get(stations, selection));
}

long GetCurrentSyncTime()
{
	struct timeval ct;
	gettimeofday(&ct, NULL);
	return originalSync + ct.tv_sec - originalTime;
}

void BinaryToHex(unsigned char *binary, char *hex, int len)
{
	int count;
	for(count = 0; count < len; count++)
	{
		sprintf(hex+count*2, "%02x", binary[count]);
	}
	hex[len*2 + 1] = '\0';
}

void UserLoginReturn(char *response)
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
	ParseUserLogin(root);
}

void ParseUserLogin(json_t *root)
{
	json_t *result = json_object_get(root, "result");
	json_t *userAuth = json_object_get(result, "userAuthToken");
	userAuthToken = malloc(json_string_length(userAuth));
	escapedUAToken = malloc(json_string_length(userAuth)*3);
	encodeAuthToken(json_string_value(userAuth), escapedUAToken);
	strcpy(userAuthToken, json_string_value(userAuth));
	json_t *uid = json_object_get(result, "userId");
	userId = malloc(json_string_length(uid));
	strcpy(userId, json_string_value(uid));
}

void UserLogin()
{
	char url[200] = "https://tuner.pandora.com/services/json/?method=auth.userLogin&partner_id=";
	strcat(url, partnerId);
	strcat(url, "&auth_token=");
	strcat(url, escapedPAToken);	

	unsigned char body[500] = "{\"loginType\":\"user\",\"username\":\"";
	strcat(body, login->username);
	strcat(body, "\",\"password\":\"");
	strcat(body, login->password);
	strcat(body, "\", \"includePandoraOneInfo\": true,\"includeAdAttributes\": true,\"includeSubscriptionExpiration\": true, \"partnerAuthToken\":\"");
	strcat(body, partnerAuthToken);
	strcat(body, "\",\"syncTime\":");
	char syncStr[11];
	sprintf(syncStr, "%ld", originalSync);
	strcat(body, syncStr);
	strcat(body, "}");
	
	int encLen = Encrypt(body);
	char encBody[encLen * 2 + 1];
	BinaryToHex(body, encBody, encLen);
	
	printf("UserLogin: %i\n", SendCurlRequest(url, encBody, UserLoginReturn));
}

