#include <curl/curl.h>
#include <jansson.h>
#include <sys/time.h>
#include "Crypto.h"

struct string
{
	char *ptr;
	size_t len;
};

long originalSync;
long originalTime;
char *partnerAuthToken;
char *escapedPAToken;
char *partnerId;
char *userAuthToken;
char *escapedUAToken;
char *userId;
const char *username = "";
const char *password = "";

void BinaryToHex(unsigned char *binary, char *hex, int len);
void init_string(struct string *s);
size_t PartnerLoginReturn(void *ptr, size_t size, size_t nmemb, struct string *s);
void DecryptSyncTime(const char *st);
void ParsePartnerLogin(json_t *root);
void encodeAuthToken(const char *unencToken, char *encToken);
void PartnerLogin();
void UserLogin();
void ParseUserLogin(struct string *s);
void GetStations();

main()
{
	PartnerLogin();
	UserLogin();
	GetStations();
	free(partnerAuthToken);
	free(escapedPAToken);
	free(partnerId);
	free(userAuthToken);
	free(escapedUAToken);
	free(userId);
}

void GetStations()
{
	char url[200] = "https://tuner.pandora.com/services/json/?method=auth.getStationList&partner_id=";
	strcat(url, partnerId);
	strcat(url, "&auth_token=");
	strcat(url, escapedUAToken);
	strcat(url, "&user_id=");
	strcat(url, userId);
	
	char body[500] = "";
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

void init_string(struct string *s)
{
	s->len = 0;
	s->ptr = malloc(1);
	if(s->ptr == NULL)
	{
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[1] = '\0';
}

size_t PartnerLoginReturn(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len+1);
	if(s->ptr == NULL)
	{
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr+s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	json_error_t error;
	json_t *root = json_loads(s->ptr, 0, &error);
	ParsePartnerLogin(root);

	return size*nmemb;
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

void ParsePartnerLogin(json_t *root)
{
	json_t *stat = json_object_get(root, "stat");
	if(strcmp(json_string_value(stat),"ok") != 0)
	{
		printf("OK was not returned.\n");
		return;
	}
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

int SendCurlRequest(char *url, char *body, size_t (*ptrFunc)(void *, size_t, size_t, struct string *))
{
	CURL *curl = curl_easy_init();
	if(curl) 
	{
		CURLcode r = curl_easy_setopt(curl, CURLOPT_URL, url);

		struct curl_slist *headerlist=NULL;
		headerlist = curl_slist_append(headerlist, "Accept: application/json");
		headerlist = curl_slist_append(headerlist, "Content-Type: application/json");
		r |= curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		r |= curl_easy_setopt(curl, CURLOPT_POST, 1);
		r |= curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
		r |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		struct string s;
		init_string(&s);
		r |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ptrFunc);
		r |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		r |= curl_easy_perform(curl);
  		
		free(s.ptr);

		curl_easy_cleanup(curl);
		return r;
	}
	return 1;
}

void PartnerLogin()
{
	char url[] = "https://tuner.pandora.com/services/json/?method=auth.partnerLogin";
	char body[] = "{\"username\":\"android\",\"password\":\"AC7IBG09A3DTSYM4R41UJWL07VLN8JI7\",\"deviceModel\":\"android-generic\",\"version\":\"5\"}";
	printf("PartnerLogin: %i\n", SendCurlRequest(url, body, PartnerLoginReturn));
}

size_t UserLoginReturn(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len+1);
	if(s->ptr == NULL)
	{
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr+s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;
	
	ParseUserLogin(s);

	return size*nmemb;
}

void encodeAuthToken(const char *unencToken, char *encToken)
{
	CURL *curl = curl_easy_init();
	char *tempToken = curl_easy_escape(curl, unencToken, 0);
	strcpy(encToken, tempToken);
	curl_free(tempToken);
	curl_easy_cleanup(curl);
}

void ParseUserLogin(struct string *s)
{
	json_error_t error;
	json_t *root = json_loads(s->ptr, 0, &error);
	json_t *stat = json_object_get(root, "stat");
	if(strcmp(json_string_value(stat),"ok") != 0)
	{
		printf("OK was not returned.\n");
		printf("%s\n", s->ptr);
		return;
	}
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
	strcat(body, username);
	strcat(body, "\",\"password\":\"");
	strcat(body, password);
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

