#ifndef NOVA_FILES_METADATA_H
#define NOVA_FILES_METADATA_H
struct nova_files_metadata { unsigned long long bytes; unsigned int valid; unsigned int posix_valid,mode,uid,gid; };
typedef int (*nova_files_metadata_reader)(int,const char*,unsigned char,struct nova_files_metadata*);
#endif
