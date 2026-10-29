#include <stdio.h>
#include <string.h>
#include "nova_textedit.h"

static int bounds(const struct nova_textedit *e, unsigned int a, unsigned int b) {
    unsigned int s=99U,t=99U; nova_textedit_selection_bounds(e,&s,&t); return s==a && t==b;
}

int main(void) {
    struct nova_textedit e; char clip[64];
    nova_textedit_init(&e,"NOVA");
    if (strcmp(e.bytes,"NOVA") || e.caret!=4U || e.anchor!=4U) return 10;
    nova_textedit_move_home(&e);
    if (!nova_textedit_move_right(&e) || e.caret!=1U || e.anchor!=1U) return 11;
    if (!nova_textedit_move_right_select(&e) || !nova_textedit_move_right_select(&e)) return 12;
    if (!nova_textedit_has_selection(&e) || !bounds(&e,1U,3U)) return 13;
    if (nova_textedit_copy_selection(&e,clip,sizeof(clip))!=2U || strcmp(clip,"OV")) return 14;
    if (!nova_textedit_insert_ascii(&e,'X') || strcmp(e.bytes,"NXA") || e.caret!=2U || e.anchor!=2U) return 15;
    if (!nova_textedit_insert_ascii(&e,'X') || strcmp(e.bytes,"NXXA") || e.caret!=3U) return 16;
    nova_textedit_select_all(&e);
    if (!bounds(&e,0U,4U) || nova_textedit_copy_selection(&e,clip,sizeof(clip))!=4U || strcmp(clip,"NXXA")) return 17;
    if (!nova_textedit_backspace(&e) || strcmp(e.bytes,"") || e.length!=0U || e.caret!=0U || e.anchor!=0U) return 18;
    if (!nova_textedit_insert_ascii_bytes(&e,clip,4U) || strcmp(e.bytes,"NXXA") || e.caret!=4U || e.anchor!=4U) return 19;
    nova_textedit_select_all(&e);
    if (!nova_textedit_delete_selection(&e) || e.length!=0U) return 20;
    if (nova_textedit_insert_ascii_bytes(&e,"A\n",2U)) return 21;
    nova_textedit_init(&e,"AB");
    if (!nova_textedit_move_left_select(&e) || !bounds(&e,1U,2U)) return 22;
    if (!nova_textedit_move_left(&e) || e.caret!=1U || e.anchor!=1U || nova_textedit_has_selection(&e)) return 23;
    puts("Stage-5A Phase-10 selection/replace/copy-cut-paste text-edit runtime unit: PASS");
    return 0;
}
