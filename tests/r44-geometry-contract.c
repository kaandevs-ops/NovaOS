#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#define nova_main unused_init_main
#include "../src/init/main.c"
#undef nova_main
int main(int argc,char**argv){assert(argc==3);struct nova_verity_descriptor d;assert(parse_verity_descriptor_file(argv[1],&d));assert(d.payload_size==50331648UL&&d.data_blocks==10240UL);assert(!parse_verity_descriptor_file(argv[2],&d));puts("R44_GEOMETRY_CONTRACT_PASS actual runtime parser accepts signed-layout geometry48MiB and rejects136MiB; signature verification is a separate stage");return 0;}
