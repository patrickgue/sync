#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "sync.h"
#include "log.h"

int main(int argc, char **argv)
{
    struct s_sync state;
    struct s_sync_file_list changes_list,
                            all_files_list,
                            old_files_list,
                            remote_full_file_list,
                            remote_new_file_list;
    struct s_sync_remote remote;
    char path[BUFSIZE], remote_path[BUFSIZE];

    sync_state_init(&state, NULL);
    sync_changes_init(&changes_list);
    sync_changes_init(&all_files_list);
    sync_changes_init(&old_files_list);
    sync_changes_init(&remote_full_file_list);
    sync_changes_init(&remote_new_file_list);

    snprintf(path, BUFSIZE, "%s/%s", state.local_base_dir, state.locations[0].path);
    sync_collect_changes(&state, path, &changes_list, &all_files_list);
    sync_read_file_tree(&state, &old_files_list);

    if (DEBUG)
    {
        LOG("ALL FILES LIST (%d)", all_files_list.location_count);
        sync_debug_list_changes(&all_files_list);
    }

    if (DEBUG)
    {
        LOG("OLD FILES LIST (%d)", old_files_list.location_count);
        sync_debug_list_changes(&old_files_list);
    }
    sync_find_missing_files(&changes_list, &old_files_list, &all_files_list);

    if (DEBUG)
    {
        LOG("LOCAL CHANGES LIST (%d)", changes_list.location_count);
        sync_debug_list_changes(&changes_list);
    }

    sync_remote_init(&state, &remote);

    snprintf(remote_path, BUFSIZE, "%s/%s", state.remote_base_dir, state.locations[0].path);
    LOG("REMOTE PATH: %s", remote_path);

    sync_remote_read_dir(remote_path, &state, &remote, &remote_full_file_list, &remote_new_file_list);

    if (DEBUG)
    {
        LOG("REMOTE FULL FILE LIST (%d)", remote_full_file_list.location_count);
        sync_debug_list_changes(&remote_full_file_list);
        LOG("REMOTE NEW FILE LIST (%d)", remote_new_file_list.location_count);
        sync_debug_list_changes(&remote_new_file_list);
    }

    sync_remote_cleanup(&remote);

    sync_store_tts(&state);
    sync_store_file_tree(&state, &all_files_list);
    
    return 0;
}

void sync_collect_changes(struct s_sync *state,
                          char *location,
                          struct s_sync_file_list *changes_list,
                          struct s_sync_file_list *all_files_list)
{
    DIR *dir;
    char full_path[BUFSIZE];
    struct dirent *entry;
    struct stat st;
    int base_path_len = strlen(state->local_base_dir);

    dir = opendir(location);

    while ((entry = readdir(dir)) != NULL)
    {
        /* ignore "." and ".." */
        if (DIR_LINKS(entry->d_name)) {
            continue;
        }
        
        snprintf(full_path, BUFSIZE, "%s/%s", location, entry->d_name);
        stat(full_path, &st);

        sync_changes_append(all_files_list, full_path + base_path_len, entry, &st);

        if (st.st_mtime > state->last_sync)
        {
            sync_changes_append(changes_list, full_path + base_path_len, entry, &st);
        }

        if (entry->d_type == DT_DIR)
        {
            sync_collect_changes(state, full_path, changes_list, all_files_list);
        }
    }
}

void sync_state_init(struct s_sync *state, char *base_path)
{
    FILE *config_file, *tts_file;
    char config_path[BUFSIZE], lbuf[BUFSIZE], key[BUFSIZE], value[BUFSIZE];
    int i;

    if (DEBUG)
        LOG("PARSE CONFIG");

    state->locations = malloc(0);
    state->location_count = 0;
    strcpy(state->tts_file_path, "");
    strcpy(state->password, "");
    strcpy(state->local_base_dir, "");

    if (base_path == NULL)
    {
        base_path = malloc(sizeof(char) * PATH_LEN);
        snprintf(base_path, PATH_LEN, "%s/.sync", getenv("HOME"));
    }

    if (DEBUG)
        LOG("Config base path %s", base_path);

    snprintf(config_path, PATH_LEN, "%s/sync.conf", base_path);
    snprintf(state->tts_file_path, PATH_LEN, "%s/sync.tts", base_path);
    snprintf(state->file_tree_path, PATH_LEN, "%s/file_tree", base_path);

    config_file = fopen(config_path, "r");

    if (config_file == NULL)
    {
        ERR("Config file not found or could not be read (%s)", config_path);
        exit(1);
    }

    while (fgets(lbuf, BUFSIZE, config_file) != NULL)
    {
        lbuf[strlen(lbuf) - 1] = 0; /* remove newline */
        if (strlen(lbuf) > 0 && lbuf[0] != '#')
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

            if (DEBUG)
                LOG("%16s -> '%s'", key, value);

            if (strcmp(key, "host") == 0)
                strncpy(state->host, value, BUFSIZE);
            else if (strcmp(key, "user") == 0)
                strncpy(state->user, value, BUFSIZE);
            else if (strcmp(key, "port") == 0)
                state->port = atoi(value);
            else if (strcmp(key, "tts:file") == 0)
                strncpy(state->tts_file_path, value, PATH_LEN);
            else if (strcmp(key, "ssh:public:key") == 0)
                strncpy(state->public_key, value, PATH_LEN);
            else if (strcmp(key, "ssh:private:key") == 0)
                strncpy(state->private_key, value, PATH_LEN);
            else if (strcmp(key, "local_base_dir") == 0)
                strncpy(state->local_base_dir, value, PATH_LEN);
            else if (strcmp(key, "remote_base_dir") == 0)
                strncpy(state->remote_base_dir, value, PATH_LEN);
            else if (strcmp(key, "location") == 0)
            {
                state->locations = realloc(state->locations, sizeof(struct s_sync_location) * (state->location_count + 1));
                strncpy(state->locations[state->location_count].path, value, PATH_LEN);
                state->location_count++;
            }
        }
    }
    fclose(config_file);

    if (strcmp(state->tts_file_path, "") == 0)
        snprintf(state->tts_file_path, PATH_LEN, "%s/sync.tts", base_path);

    tts_file = fopen(state->tts_file_path, "r");

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

void sync_changes_init(struct s_sync_file_list *changes)
{
    changes->locations = malloc(0);
    changes->location_count = 0;
}

void sync_changes_append(struct s_sync_file_list *changes,
                         char *path,
                         struct dirent *entry,
                         struct stat *st)
{
    changes->locations = realloc(changes->locations, sizeof(struct s_sync_location) * (changes->location_count + 1));

    changes->locations[changes->location_count].d_type = entry->d_type;
    changes->locations[changes->location_count].hash = sync_path_hash(path);
    strncpy(changes->locations[changes->location_count].path, path, PATH_LEN);
    changes->location_count++;
}

void sync_changes_append_loc (struct s_sync_file_list *list, struct s_sync_location *loc, bool d)
{
    list->locations = realloc(list->locations, sizeof(struct s_sync_location) * (list->location_count + 1));
    memcpy(&(list->locations[list->location_count]), loc, sizeof(struct s_sync_location));
    list->locations[list->location_count].delete = d;
    list->location_count++;
}

void sync_store_tts(struct s_sync *state)
{
    FILE *tts_file = fopen(state->tts_file_path, "w");
    time_t t;
    if (tts_file == NULL)
    {
        ERR("Unable to open Timestamp file for writing (%s)", state->tts_file_path);
        exit(1);
    }
    t = time(NULL);
    fwrite(&t, sizeof(time_t), 1, tts_file);
    fclose(tts_file);
}

void sync_read_file_tree(struct s_sync *state, struct s_sync_file_list *tree)
{
    FILE *tree_file = fopen(state->file_tree_path, "r");
    int i;
    if (tree_file == NULL)
    {
        WARN("File tree file could not be opened, deleted files cannot be tracked");
        return;
    }

    fread(&(tree->location_count), sizeof(int), 1, tree_file);
    tree->locations = malloc(sizeof(struct s_sync_location) * tree->location_count);

    for (i = 0; i < tree->location_count; i++)
    {
        fread(&(tree->locations[i].d_type), sizeof(char), 1, tree_file);
        fread(&(tree->locations[i].hash), sizeof(long), 1, tree_file);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        tree->locations[i].hash = swap_endianness_long(tree->locations[i].hash);
#endif
        fread(tree->locations[i].path, sizeof(char), PATH_LEN, tree_file);
    }
    fclose(tree_file);
}


void sync_store_file_tree(struct s_sync *state, struct s_sync_file_list *tree)
{
    FILE *tree_file = fopen(state->file_tree_path, "w");
    int i;
    if (tree_file == NULL)
    {
        ERR("File tree file could not be opened, deleted files cannot be tracked");
        exit(1);
    }

    fwrite(&(tree->location_count), sizeof(int), 1, tree_file);

    for (i = 0; i < tree->location_count; i++)
    {
        fwrite(&(tree->locations[i].d_type), sizeof(char), 1, tree_file);
        fwrite(&(tree->locations[i].hash), sizeof(long), 1, tree_file);
        fwrite(tree->locations[i].path, sizeof(char), PATH_LEN, tree_file);
    }
    fclose(tree_file);    
}

void sync_find_missing_files(struct s_sync_file_list *target, struct s_sync_file_list *old, struct s_sync_file_list *new)
{
    int i, j;
    bool found;
    for (i = 0; i < old->location_count; i++)
    {
        found = false;
        for (j = 0; j < new->location_count; j++)
        {
            if (old->locations[i].hash == new->locations[j].hash)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            
            sync_changes_append_loc(target, &(old->locations[i]), true);
        }
    }
}

long sync_path_hash (char *str)
{
    int i;
    long hash = 0x1234567890abcdef;
    for (i = 0; i < strlen(str); i++)
    {
        hash = hash + ((str[i] | hash) * str[i]);
    }

    return hash;
}

/*
 * =====================
 * =       REMOTE      =
 * =====================
 */

void sync_remote_init(struct s_sync *state, struct s_sync_remote *remote)
{
    int                   rc;
    struct sockaddr_in    sin;
    
    rc = libssh2_init(0);

    if (rc)
    {
        ERR("Unable to initialize ssh");
        exit(1);
    }
 
    remote->sock =        socket(AF_INET, SOCK_STREAM, 0);
 
    sin.sin_family =      AF_INET;
    sin.sin_port =        htons(state->port);
    sin.sin_addr.s_addr = inet_addr(state->host);

    if (DEBUG)
        LOG("SOCKET: CONNECT");

    if(connect(remote->sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0)
    {
        ERR("Failed to connect");
        exit(1);
    }

    if (DEBUG)
        LOG("SSH: INIT SESSION");
    
    remote->session = libssh2_session_init();

    if (!remote->session)
    {
        ERR("Failed to init session");
        exit(1);
    }

    if (DEBUG)
        LOG("SSH: HANDSHAKE");

    while ((rc = libssh2_session_handshake(remote->session, remote->sock)) == LIBSSH2_ERROR_EAGAIN);

    if(rc)
    {
        ERR("Failure establishing SSH session: %d", rc);
        exit(1);
    }

    libssh2_trace(remote->session, ~0);

    while((rc = libssh2_userauth_publickey_fromfile(remote->session,
                                                    state->user,
                                                    state->public_key,
                                                    state->private_key,
                                                    state->password)) ==
          LIBSSH2_ERROR_EAGAIN);
    if(rc)
    {
        ERR("Authentication by public key failed (%d)", rc);
        exit(1);
    }

    remote->sftp_session = libssh2_sftp_init(remote->session);
    if (!remote->sftp_session)
    {
        ERR("Unable to init sftp session");
        exit(1);
    }

    libssh2_session_set_blocking(remote->session, 1);
}


void sync_remote_read_dir(char *path,
                          struct s_sync *state,
                          struct s_sync_remote *remote,
                          struct s_sync_file_list *list,
                          struct s_sync_file_list *new)
{
    int rc, remote_base_dir_len;
    char new_path[BUFSIZ], mem[512];
    LIBSSH2_SFTP_HANDLE *sftp_handle;
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    remote_base_dir_len = strlen(state->remote_base_dir);
    sftp_handle = libssh2_sftp_opendir(remote->sftp_session, path);

    do
    {
        rc = libssh2_sftp_readdir(sftp_handle, mem, sizeof(mem), &attrs);
        if(rc > 0)
        {
            if (DIR_LINKS(mem))
                continue;

            snprintf(new_path, BUFSIZE, "%s/%s", path, mem);
            sync_remote_changes_append(list, new_path + remote_base_dir_len, attrs.permissions);
            
            if (attrs.permissions & LIBSSH2_SFTP_S_IFDIR)
            {
                LOG("new_path: %s", new_path);
                sync_remote_read_dir(new_path, state, remote, list, new);
            }
            else if (attrs.mtime > state->last_sync)
            {
                sync_remote_changes_append(new, new_path + remote_base_dir_len, attrs.permissions);
            }
        }
        else
            break;

    }
    while(1);
    libssh2_sftp_closedir(sftp_handle);
}


void sync_remote_cleanup(struct s_sync_remote *remote)
{
    libssh2_sftp_shutdown(remote->sftp_session);

    libssh2_session_disconnect(remote->session, "Close Connection");
    libssh2_session_free(remote->session);

    close(remote->sock);
}

void sync_remote_changes_append(struct s_sync_file_list *changes,
                                char *path,
                                long attrs)
{
    unsigned char type;
    changes->locations = realloc(changes->locations, sizeof(struct s_sync_location) * (changes->location_count + 1));

    if (attrs & LIBSSH2_SFTP_S_IFDIR)
        type = DT_DIR;
    else if (attrs & LIBSSH2_SFTP_S_IFREG)
        type = DT_REG;
    else if (attrs & LIBSSH2_SFTP_S_IFLNK)
        type = DT_LNK;

    changes->locations[changes->location_count].d_type = type;
    changes->locations[changes->location_count].delete = false;
    changes->locations[changes->location_count].hash = sync_path_hash(path);
    strncpy(changes->locations[changes->location_count].path, path, PATH_LEN);
    changes->location_count++;
}

/*
 * =====================
 * =  DEBUG FUNCTIONS  =
 * =====================
 */

void sync_debug_list_changes(struct s_sync_file_list *changes)
{
    int i;
    for (i = 0; i < changes->location_count; i++)
    {
        LOG("  %c %c -> %lx %s",
            changes->locations[i].delete ? '-' : '+',
            changes->locations[i].d_type == DT_DIR ? 'D' : 'F',
            changes->locations[i].hash,
            changes->locations[i].path);
    }
}
