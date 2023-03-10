#ifndef _sync_h
#define _sync_h

#include <time.h>

#define PATH_LEN 1024

#define DIR_LINKS(PATH) strcmp(PATH, ".") == 0 || strcmp(PATH, "..") == 0

/* generic type for file locations */
struct s_sync_location
{
    char path[PATH_LEN];
    unsigned char d_type;
};

struct s_sync
{
    time_t last_sync;
    char tts_file[PATH_LEN];
    struct s_sync_location *locations;
    int location_count;
    char host[PATH_LEN];
};

struct s_sync_changes
{
    struct s_sync_location *locations;
    int location_count;
};

void sync_state_init(struct s_sync*, char *);
void sync_collect_changes(char*, struct s_sync_changes *);
void sync_changes_init(struct s_sync_changes *);
void sync_changes_append(struct s_sync_changes *, struct s_sync_location *);


#endif
