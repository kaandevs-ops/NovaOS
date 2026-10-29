#include <stdio.h>
#include "nova_ui.h"
int main(void){for(unsigned int t=0;t<3;t++){struct nova_ui_palette p=nova_ui_palette_for(t);printf("%u %u %u %u %u %u\n",t,p.paper,p.sidebar,p.ink,p.muted,p.selection);}return 0;}
