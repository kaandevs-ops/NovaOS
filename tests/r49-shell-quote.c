#include <assert.h>
#include <string.h>
#include "nova_shell_quote.h"
int main(void){char b[400];assert(nova_shell_quote("/home/nova/Desktop/a b.txt",b,sizeof b));assert(!strcmp(b,"'/home/nova/Desktop/a b.txt' "));assert(nova_shell_quote("/home/nova/Desktop/a'b;$x",b,sizeof b));assert(!strcmp(b,"'/home/nova/Desktop/a'\\''b;$x' "));assert(!nova_shell_quote("/tmp/bad\ncommand",b,sizeof b));assert(!nova_shell_quote("relative",b,sizeof b));for(unsigned n=0;n<8;n++)assert(!nova_shell_quote("/longpath",b,n));return 0;}
