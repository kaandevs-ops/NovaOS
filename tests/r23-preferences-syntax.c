#include "nova_preferences_store.h"
int nova_pref_compile(int fd){struct nova_preferences p={0,1};return nova_preferences_store(fd,0,&p)+nova_preferences_load(fd,0,&p);}
