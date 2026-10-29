#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
int nova_upstream_game_main(int,char**);
int main(int argc,char**argv){
 int interactive=argc==1||(argc==2&&(!strcmp(argv[1],"blackwhite")||!strcmp(argv[1],"bluered")||!strcmp(argv[1],"whiteblack")));
 if(interactive){struct winsize w;if(!isatty(0)||ioctl(0,TIOCGWINSZ,&w)){fputs("2048: interactive terminal required\n",stderr);return 2;}if(w.ws_col<30||w.ws_row<20){fputs("2048: needs at least 30 columns and 20 rows; maximize the terminal first.\n",stderr);return 2;}}
 return nova_upstream_game_main(argc,argv);
}
