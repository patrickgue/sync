#ifndef _sync_h
#define _sync_h

#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>

#define PATH_LEN 128
#define BUFSIZE 1024

#define DIR_LINKS(PATH) strcmp(PATH, ".") == 0 || strcmp(PATH, "..") == 0

/* generic type for file locations */
struct s_sync_location
{
    char                      path[PATH_LEN];
    unsigned char             d_type;
    bool                      delete;
    long                      hash;
};

struct s_sync
{
    time_t                    last_sync;
    char                      tts_file_path[PATH_LEN];
    char                      file_tree_path[PATH_LEN];
    struct s_sync_location   *locations;
    int                       location_count;
    char                      host[BUFSIZE];
    char                      user[BUFSIZE];
    int                       port;
};

struct s_sync_file_list
{
    struct s_sync_location   *locations;
    int                       location_count;
};

void sync_state_init          (struct s_sync*, char *);
void sync_collect_changes     (
    struct s_sync*, char*,
    struct s_sync_file_list *,
    struct s_sync_file_list *);
void sync_changes_init        (struct s_sync_file_list *);
void sync_changes_append      (
    struct s_sync_file_list *,
    char *,
    struct dirent *,
    struct stat *);
void sync_changes_append_loc  (
    struct s_sync_file_list *,
    struct s_sync_location *,
    bool);
void sync_store_tts           (struct s_sync *);
long sync_path_hash           (char *);
void sync_read_file_tree      (struct s_sync *, struct s_sync_file_list *);
void sync_store_file_tree     (struct s_sync *, struct s_sync_file_list *);
void sync_find_missing_files  (
    struct s_sync_file_list *,
    struct s_sync_file_list *,
    struct s_sync_file_list *);

void sync_debug_list_changes  (struct s_sync_file_list *);

long swap_endianness_long(long);
int swap_endianness_int(int);

#endif
