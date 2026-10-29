#ifndef NOVA_LOCAL_CATALOG_H
#define NOVA_LOCAL_CATALOG_H
#include <string.h>
struct nova_catalog_entry {const char *id,*version,*command,*payload,*sha256,*title,*summary,*category;};
#ifndef NOVA_PACKAGE_CATALOG
#error generated trusted catalog required
#endif
#include NOVA_PACKAGE_CATALOG
static inline const struct nova_catalog_entry *find_package(const char *id){
 for(unsigned i=0;i<sizeof(nova_catalog)/sizeof(nova_catalog[0]);i++)
  if(!strcmp(id,nova_catalog[i].id))return &nova_catalog[i];
 return NULL;
}
#endif
