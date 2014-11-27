#include <string.h>
#include "login.h"
#include "player.h"

Playlist *plist;
Playlist *first;

void freeSongs(Playlist *list)
{
	while(list != NULL)
	{
		Playlist *temp = list->next;
		free(list->song->songIdentity);
		free(list->song->trackToken);
		free(list->song->songName);
		free(list->song->artistName);
		free(list->song->albumName);
		free(list->song->audioUrl);
		free(list->song);
		free(list);
		list = temp;
	}
}

void DownloadSong(Playlist *list)
{
	SongData *song = malloc(sizeof(SongData));
	song->data = malloc(1);
	song->data[0] = '\0';
	song->length = 0;
	AudioRequest(list->song->audioUrl, song);
	printf("%ld\n", song->length);
	FILE *songFile = fopen("test.aac", "wb");
	int ii;
	for(ii = 0; ii < song->length; ii++)
	{
		fputc(song->data[ii], songFile);
	}
}

void allocInfo(char **info, const char *s, json_t *item)
{
	size_t length = json_string_length(json_object_get(item, s));
	*info = malloc(length + 1);
	strcpy(*info, json_string_value(json_object_get(item, s)));
	(*info)[length] = '\0';
}

void GetSongInfo(Playlist *list, json_t *item)
{
	list->song = malloc(sizeof(Song));
	allocInfo(&(list->song->songIdentity), "songIdentity", item);
	allocInfo(&(list->song->trackToken), "trackToken", item);
	allocInfo(&(list->song->songName), "songName", item);
	allocInfo(&(list->song->artistName), "artistName", item);
	allocInfo(&(list->song->albumName), "albumName", item);

	json_t *map = json_object_get(json_object_get(item, "audioUrlMap"), "mediumQuality");
	allocInfo(&(list->song->audioUrl), "audioUrl", map);
}

void PrintSong(Playlist *list)
{
	while(list != NULL)
	{
		printf("%s -- %s (%s)\n", list->song->songName, list->song->artistName, list->song->albumName);
		list = list->next;
	}
}

void ParsePlaylist(json_t *root)
{
	json_t *items = json_object_get(json_object_get(root, "result"), "items");
	int ii;
	int count = json_array_size(items);
	for(ii = 0; ii < count; ii++)
	{
		json_t *curItem = json_array_get(items, ii);
		if(json_string_value(json_object_get(curItem, "songName")) != NULL)
		{
			if(!plist)
			{
				plist = malloc(sizeof(Playlist));
				plist->next = NULL;
				plist->prev = NULL;
				first = plist;
			}
			else
			{
				plist->next = malloc(sizeof(Playlist));
				plist->next->prev = plist;
				plist = plist->next;
				plist->next = NULL;
			}
			GetSongInfo(plist, json_array_get(items, ii));
		}
	}
	PrintSong(first);
	DownloadSong(first);
	freeSongs(first);
}

void GetPlaylistReturn(char *response)
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
	return ParsePlaylist(root);
}

void GetPlaylist(struct Auth *auth, struct Station station)
{
	char url[200] = "http://tuner.pandora.com/services/json/?method=station.getPlaylist&partner_id=";
	strcat(url, auth->partnerId);
	strcat(url, "&auth_token=");
	strcat(url, auth->escapedToken);
	strcat(url, "&user_id=");
	strcat(url, auth->userId);
	
	char body[500] = "{\"stationToken\": \"";
	strcat(body, station.id);
	strcat(body, "\",\"userAuthToken\":\"");
	strcat(body, auth->token);
	strcat(body, "\",\"syncTime\":");
	sprintf(body, "%s%ld", body, GetCurrentSyncTime(auth->sync));
	strcat(body, "}");
	
	int encLen = Encrypt(body);
	char encBody[encLen * 2 + 1];
	BinaryToHex(body, encBody, encLen);

	char *response = malloc(1);
	response[0] = '\0';
	SendCurlRequest(url, encBody, &response);
	GetPlaylistReturn(response);
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

struct Station GetStationListReturn(char *response)
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
	return ParseStationList(root);
}

void GetStationList(struct Auth *auth)
{
	char url[200] = "http://tuner.pandora.com/services/json/?method=user.getStationList&partner_id=";
	strcat(url, auth->partnerId);
	strcat(url, "&auth_token=");
	strcat(url, auth->escapedToken);
	strcat(url, "&user_id=");
	strcat(url, auth->userId);
	
	char body[500] = "{\"includeStationArtUrl\": true,\"userAuthToken\":\"";
	strcat(body, auth->token);
	strcat(body, "\",\"syncTime\":");
	sprintf(body, "%s%ld", body, GetCurrentSyncTime(auth->sync));
	strcat(body, "}");
	
	int encLen = Encrypt(body);
	char encBody[encLen * 2 + 1];
	BinaryToHex(body, encBody, encLen);

	char *response = malloc(1);
	response[0] = '\0';
	SendCurlRequest(url, encBody, &response);
	GetPlaylist(auth, GetStationListReturn(response));
}
