#include <stdio.h>
#include <string.h>
#include "nova_textedit.h"
#include "nova_widget.h"

int main(void) {
    struct nova_textedit e;
    nova_textedit_init(&e,"NOVA");
    if (strcmp(e.bytes,"NOVA") || e.length!=4U || e.caret!=4U) return 10;
    if (!nova_textedit_insert_ascii(&e,'H') || !nova_textedit_insert_ascii(&e,'I')) return 11;
    if (strcmp(e.bytes,"NOVAHI") || e.caret!=6U) return 12;
    if (!nova_textedit_backspace(&e) || strcmp(e.bytes,"NOVAH") || e.caret!=5U) return 13;
    if (!nova_textedit_move_left(&e) || e.caret!=4U) return 14;
    if (!nova_textedit_insert_ascii(&e,'X') || strcmp(e.bytes,"NOVAXH") || e.caret!=5U) return 15;
    if (!nova_textedit_delete(&e) || strcmp(e.bytes,"NOVAX") || e.length!=5U || e.caret!=5U) return 16;
    if (nova_textedit_insert_ascii(&e,0x80U)) return 17;

    struct nova_widget_tree t;
    nova_widget_tree_init(&t);
    if (!nova_widget_add(&t,1U,NOVA_WIDGET_ROOT,0U,0,0,200U,100U,0U)) return 20;
    if (!nova_widget_add(&t,2U,NOVA_WIDGET_TEXT_FIELD,1U,10,10,80U,20U,NOVA_WIDGET_FLAG_FOCUSABLE)) return 21;
    if (!nova_widget_add(&t,3U,NOVA_WIDGET_BUTTON,1U,10,40,80U,30U,NOVA_WIDGET_FLAG_FOCUSABLE)) return 22;
    if (nova_widget_focus_first(&t)!=2U || t.focused_id!=2U) return 23;
    if (nova_widget_focus_next(&t)!=3U || nova_widget_focus_next(&t)!=2U) return 24;
    if (nova_widget_hit_test(&t,12,12)!=2U || nova_widget_hit_test(&t,12,42)!=3U || nova_widget_hit_test(&t,150,80)!=0U) return 25;
    if (!nova_widget_set_focus(&t,3U) || t.focused_id!=3U) return 26;
    puts("Stage-5A Phase-9 widget-tree + bounded ASCII/UTF-8-subset text-edit runtime unit: PASS");
    return 0;
}
