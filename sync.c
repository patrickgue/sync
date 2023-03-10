#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#include "sync.h"

int main(int argc, char **argv)
{
    struct s_sync state;
    struct s_sync_changes changes_list;
    sync_state_init(&state, NULL);

    sync_collect_changes(state.locations[0].path, &changes_list);
    
    return 0;
}

void sync_collect_changes(char *location, struct s_sync_changes *changes_list)
{
    DIR *dir;
    char full_path[PATH_LEN];
    struct dirent *entry;
    struct stat st;

    dir = opendir(location);

    while ((entry = readdir(dir)) != NULL)
    {
        /* ignore "." and ".." */
        if (DIR_LINKS(entry->d_name)) {
            continue;
        }
        
        snprintf(full_path, PATH_LEN, "%s/%s", location, entry->d_name);
        stat(full_path, &st);
        printf("%ld %s\n", st.st_mtime, full_path);

        if (entry->d_type == DT_DIR)
        {
            sync_collect_changes(full_path, changes_list);
        }
        else
        {

        }
        
    }
}

void sync_state_init(struct s_sync *state, char *path)
{
    FILE *config;
    if (path == NULL)
    {
        path = malloc(PATH_LEN);
        snprintf(path, PATH_LEN, "%s/.sync.conf", getenv("HOME"));
    }
    printf("Config file path %s\n", path);
    // config_file = fopen(path);

    // TODO: read from config and think of sensible default (is $HOME realy best?)
    snprintf(state->tts_file, PATH_LEN, "%s/.sync.tts", getenv("HOME"));

    state->location_count = 1;
    state->locations = malloc(sizeof(struct s_sync_location));
    snprintf(state->locations[0].path, PATH_LEN, "%s/tmp/test", getenv("HOME"));

    // TODO read from tts file; if tts cannot be read, use 0 to sync everything
    state->last_sync = time(NULL) - 1000; 
}

void sync_changes_init(struct s_sync_changes *changes)
{
    changes->locations = malloc(0);
    changes->location_count = 0;
}

void sync_changes_append(struct s_sync_changes *changes, struct s_sync_location *loc)
{
    changes->locations = realloc(changes->locations, sizeof(struct s_sync_location) * (changes->location_count + 1));

    memcpy(changes->locations + changes->location_count, loc, sizeof(struct s_sync_location));
    changes->location_count++;
}
