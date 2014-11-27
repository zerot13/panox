struct Station
{
	char *id;
	char *name;
};

typedef struct
{
	unsigned char *data;
	long length;
} SongData;

typedef struct
{
	char *songIdentity;
	char *trackToken;
	char *songName;
	char *artistName;
	char *albumName;
	char *audioUrl;
} Song;

typedef struct Playlist Playlist;

struct Playlist
{
	Song *song;
	Playlist *next;
	Playlist *prev;
};
