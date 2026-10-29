#define _GNU_SOURCE
#include "../src/compat/nova_browser_profile.h"
int main(void) {
 int fd=browser_profile_open();
 if(fd<0){fprintf(stderr,"profile rejected errno=%d status=%d\n",errno,fd);return 2;}
 /* A concurrent opener must not acquire the exclusive lifetime lock. */
 int duplicate=browser_profile_open();
 if(duplicate>=0){close(duplicate);close(fd);return 3;}
 struct stat s;if(fstat(fd,&s)){close(fd);return 4;}
 printf("validated bytes=%lld owner=%u mode=%o lock=exclusive\n",(long long)s.st_size,s.st_uid,s.st_mode&07777);
 close(fd);return 0;
}
