#ifndef NOVA_FILES_CORE_H
#define NOVA_FILES_CORE_H
/* NovaOS native read-only directory model. No libc, heap, global FS root,
 * filesystem writes, process spawning, or guest authority inside this core.
 * Filesystem authority is supplied ONLY as a pre-opened directory FD. */
#define NOVA_FILES_MAX_DEPTH 8U
#define NOVA_FILES_MAX_ENTRIES 48U
#define NOVA_FILES_QUERY_BYTES 64U
#define NOVA_FILES_NAME_BYTES 128U
#define NOVA_FILES_PATH_BYTES 256U
#define NOVA_FILES_DTYPE_DIR 4U
#define NOVA_FILES_DTYPE_SYMLINK 10U
#define NOVA_FILES_OK 0
#define NOVA_FILES_BAD -1
#define NOVA_FILES_IO -2
#define NOVA_FILES_FULL -3
#define NOVA_FILES_ESCAPE -4
struct nova_files_entry {
    char name[NOVA_FILES_NAME_BYTES];
    unsigned char type;
};
struct nova_files_ops {
    long (*seek_start)(int fd,void *ctx);
    long (*read_entries)(int fd,void *buffer,unsigned long cap,void *ctx);
    long (*open_child_dir)(int fd,const char *component,void *ctx);
    long (*close_fd)(int fd,void *ctx);
};
struct nova_files_model {
    struct nova_files_ops ops;
    void *ctx;
    int directory_fds[NOVA_FILES_MAX_DEPTH+1U];
    unsigned int depth;
    unsigned int path_marks[NOVA_FILES_MAX_DEPTH+1U];
    char path[NOVA_FILES_PATH_BYTES];
    struct nova_files_entry entries[NOVA_FILES_MAX_ENTRIES];
    unsigned int count;
    unsigned int truncated;
    unsigned int scanned;
    unsigned int has_previous,has_next;
    char listing_query[NOVA_FILES_QUERY_BYTES];
};
int nova_files_init(struct nova_files_model *model,int delegated_directory_fd,
                    const struct nova_files_ops *ops,void *ctx);
int nova_files_list(struct nova_files_model *model);
/* Bounded whole-directory ASCII query and ordered pages; failure is transactional. */
int nova_files_query(struct nova_files_model *model,const char *query);
int nova_files_page(struct nova_files_model *model,int forward);
int nova_files_enter(struct nova_files_model *model,const char *name);
int nova_files_parent(struct nova_files_model *model);
/* Atomic absolute path navigation under the same delegated root; no symlinks or dot components. */
int nova_files_navigate(struct nova_files_model *model,const char *path);
void nova_files_close(struct nova_files_model *model);
#endif
