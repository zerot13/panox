#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include "curl.h"

size_t CurlReturn(void *ptr, size_t size, size_t nmemb, char **response)
{
	size_t cursize = strlen(*response);
	*response = realloc(*response, cursize + size*nmemb + 1);
	memcpy(&((*response)[cursize]), ptr, size*nmemb);
	(*response)[cursize+size*nmemb] = '\0';
	return size*nmemb;
}

size_t SongReturn(void *ptr, size_t size, size_t nmemb, void *response)
{
	SongData *song = (SongData *)response;
	song->data = realloc(song->data, song->length + size*nmemb + 1);
	memcpy(&(song->data[song->length]), ptr, size*nmemb);
	song->data[song->length + size*nmemb] = '\0';
	song->length += size*nmemb;
	return size*nmemb;
}

int SendCurlRequest(char *url, char *body, char **response)
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
		r |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlReturn);
		r |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
		r |= curl_easy_perform(curl);
		
		curl_easy_cleanup(curl);
		return r;
	}
	return -1;
}

int AudioRequest(char *url, SongData *response)
{
	CURL *curl = curl_easy_init();
	if(curl)
	{
		CURLcode r = curl_easy_setopt(curl, CURLOPT_URL, url);
		r |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, SongReturn);
		r |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
		r |= curl_easy_perform(curl);

		curl_easy_cleanup(curl);
		return r;
	}
	return -1;
}

void encodeAuthToken(const char *unencToken, char *encToken)
{
	CURL *curl = curl_easy_init();
	char *tempToken = curl_easy_escape(curl, unencToken, 0);
	strcpy(encToken, tempToken);
	curl_free(tempToken);
	curl_easy_cleanup(curl);
}
