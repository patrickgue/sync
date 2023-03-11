#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "sync.h"
#include "log.h"

int main(int argc, char **argv)
{
    struct s_sync state;
    struct s_sync_changes changes_list;
    sync_state_init(&state, NULL);

    sync_changes_init(&changes_list);
    sync_collect_changes(&state, state.locations[0].path, &changes_list);
    sync_debug_list_changes(&changes_list);
    sync_store_tts(&state);
    
    return 0;
}

void sync_collect_changes(struct s_sync *state,
                          char *location,
                          struct s_sync_changes *changes_list)
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

        if (entry->d_type == DT_DIR)
        {
            sync_collect_changes(state, full_path, changes_list);
        }
        else
        {
            if (st.st_mtime > state->last_sync)
            {
                sync_changes_append(changes_list, full_path, entry, &st);
            }
        }   
    }
}

void sync_state_init(struct s_sync *state, char *path)
{
    FILE *config_file, *tts_file;
    char lbuf[BUFSIZE], key[BUFSIZE], value[BUFSIZE];
    int i;

    state->locations = malloc(0);
    state->location_count = 0;
    strcpy(state->tts_file, "");

    if (path == NULL)
    {
        path = malloc(PATH_LEN);
        snprintf(path, PATH_LEN, "%s/.sync.conf", getenv("HOME"));
    }
    printf("Config file path %s\n", path);
    config_file = fopen(path, "r");

    if (config_file != NULL)
    {
        ERR("Config file not found or could not be read (%s)", path);
    }

    while (fgets(lbuf, BUFSIZE, config_file) != NULL)
    {
        lbuf[strlen(lbuf) - 1] = 0; /* remove newline */
        if (strlen(lbuf) > 0)
        {
            strncpy(key, lbuf, BUFSIZE);
            for (i = 0; i < strlen(key); i++)
            {
                if (key[i] == '=')
                {
                    key[i] = 0;
                    break;
                }
            }
            strncpy(value, lbuf + i + 1, BUFSIZE);

            LOG("%16s -> %s", key, value);
            if (strcmp(key, "host") == 0)
                strncpy(state->host, value, BUFSIZE);
            else if (strcmp(key, "user") == 0)
                strncpy(state->user, value, BUFSIZE);
            else if (strcmp(key, "port") == 0)
                state->port = atoi(value);
            else if (strcmp(key, "tts:file") == 0)
                strncpy(state->tts_file, value, BUFSIZE);
            else if (strcmp(key, "location") == 0)
            {
                state->locations = realloc(state->locations, sizeof(struct s_sync_location) * (state->location_count + 1));
                strncpy(state->locations[state->location_count].path, value, PATH_LEN);
                state->location_count++;
            }

        }
    }
    fclose(config_file);

    if (strcmp(state->tts_file, "") == 0)
        snprintf(state->tts_file, PATH_LEN, "%s/.sync.tts", getenv("HOME")); // TODO: think of sensible default (is $HOME realy best?)


    tts_file = fopen(state->tts_file, "r");

    if (tts_file == NULL)
    {
        state->last_sync = 0; 
    }
    else
    {
        fread(&(state->last_sync), sizeof(time_t), 1, tts_file);
        fclose(tts_file);
    }
}

void sync_changes_init(struct s_sync_changes *changes)
{
    changes->locations = malloc(0);
    changes->location_count = 0;
}

void sync_changes_append(struct s_sync_changes *changes,
                         char *path,
                         struct dirent *entry,
                         struct stat *st)
{
    changes->locations = realloc(changes->locations, sizeof(struct s_sync_location) * (changes->location_count + 1));

    changes->locations[changes->location_count].d_type = entry->d_type;
    strncpy(changes->locations[changes->location_count].path, path, PATH_LEN);
    changes->location_count++;
}


void sync_debug_list_changes(struct s_sync_changes *changes)
{
    int i;
    for (i = 0; i < changes->location_count; i++)
    {
        LOG("%c -> %s", changes->locations[i].d_type == DT_DIR ? 'D' : 'F', changes->locations[i].path);
    }
}

void sync_store_tts(struct s_sync *state)
{
    FILE *tts_file = fopen(state->tts_file, "w");
    time_t t;
    if (tts_file == NULL)
    {
        ERR("Unable to open Timestamp file for writing (%s)", state->tts_file);
        exit(1);
    }
    t = time(NULL);
    fwrite(&t, sizeof(time_t), 1, tts_file);
    fclose(tts_file);
}
