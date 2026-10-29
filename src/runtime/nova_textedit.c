#include "nova_textedit.h"

static void clamp_edit(struct nova_textedit *edit) {
    if (!edit) return;
    if (edit->length>=NOVA_TEXTEDIT_CAPACITY) edit->length=NOVA_TEXTEDIT_CAPACITY-1U;
    if (edit->caret>edit->length) edit->caret=edit->length;
    if (edit->anchor>edit->length) edit->anchor=edit->length;
    edit->bytes[edit->length]=0;
}

void nova_textedit_selection_bounds(const struct nova_textedit *edit, unsigned int *start, unsigned int *end) {
    unsigned int a=0U,b=0U;
    if (edit) { a=edit->anchor; b=edit->caret; if (a>b) { unsigned int t=a; a=b; b=t; } }
    if (start) *start=a;
    if (end) *end=b;
}

int nova_textedit_has_selection(const struct nova_textedit *edit) {
    return edit && edit->anchor!=edit->caret;
}

void nova_textedit_init(struct nova_textedit *edit, const char *initial) {
    if (!edit) return;
    for (unsigned int i=0;i<NOVA_TEXTEDIT_CAPACITY;i++) edit->bytes[i]=0;
    edit->length=0U; edit->caret=0U; edit->anchor=0U;
    if (!initial) return;
    while (initial[edit->length] && edit->length+1U<NOVA_TEXTEDIT_CAPACITY) {
        unsigned char c=(unsigned char)initial[edit->length];
        if (c<0x20U || c>0x7eU) break;
        edit->bytes[edit->length]=(char)c; edit->length++;
    }
    edit->bytes[edit->length]=0; edit->caret=edit->length; edit->anchor=edit->caret;
}

int nova_textedit_delete_selection(struct nova_textedit *edit) {
    if (!edit || !nova_textedit_has_selection(edit)) return 0;
    unsigned int start,end; nova_textedit_selection_bounds(edit,&start,&end);
    unsigned int removed=end-start;
    for (unsigned int i=start;i+removed<=edit->length;i++) edit->bytes[i]=edit->bytes[i+removed];
    edit->length-=removed; edit->caret=start; edit->anchor=start; clamp_edit(edit); return 1;
}

int nova_textedit_insert_ascii(struct nova_textedit *edit, unsigned int codepoint) {
    if (!edit || codepoint<0x20U || codepoint>0x7eU) return 0;
    unsigned int selected=0U;
    if (nova_textedit_has_selection(edit)) { unsigned int s,e; nova_textedit_selection_bounds(edit,&s,&e); selected=e-s; }
    if (edit->length-selected+1U>=NOVA_TEXTEDIT_CAPACITY) return 0;
    (void)nova_textedit_delete_selection(edit); clamp_edit(edit);
    for (unsigned int i=edit->length;i>edit->caret;i--) edit->bytes[i]=edit->bytes[i-1U];
    edit->bytes[edit->caret]=(char)codepoint; edit->length++; edit->caret++; edit->anchor=edit->caret;
    clamp_edit(edit); return 1;
}

int nova_textedit_insert_ascii_bytes(struct nova_textedit *edit, const char *bytes, unsigned int length) {
    if (!edit || (!bytes && length)) return 0;
    unsigned int selected=0U;
    if (nova_textedit_has_selection(edit)) { unsigned int s,e; nova_textedit_selection_bounds(edit,&s,&e); selected=e-s; }
    if (edit->length-selected+length>=NOVA_TEXTEDIT_CAPACITY) return 0;
    for (unsigned int i=0;i<length;i++) { unsigned char c=(unsigned char)bytes[i]; if (c<0x20U || c>0x7eU) return 0; }
    (void)nova_textedit_delete_selection(edit); clamp_edit(edit);
    unsigned int tail=edit->length-edit->caret;
    for (unsigned int i=tail;i>0;i--) edit->bytes[edit->caret+length+i-1U]=edit->bytes[edit->caret+i-1U];
    for (unsigned int i=0;i<length;i++) edit->bytes[edit->caret+i]=bytes[i];
    edit->length+=length; edit->caret+=length; edit->anchor=edit->caret; clamp_edit(edit); return length?1:0;
}

int nova_textedit_backspace(struct nova_textedit *edit) {
    if (!edit) return 0;
    if (nova_textedit_delete_selection(edit)) return 1;
    if (!edit->caret || !edit->length) return 0;
    unsigned int remove=edit->caret-1U;
    for (unsigned int i=remove;i<edit->length;i++) edit->bytes[i]=edit->bytes[i+1U];
    edit->length--; edit->caret--; edit->anchor=edit->caret; clamp_edit(edit); return 1;
}

int nova_textedit_delete(struct nova_textedit *edit) {
    if (!edit) return 0;
    if (nova_textedit_delete_selection(edit)) return 1;
    if (edit->caret>=edit->length) return 0;
    for (unsigned int i=edit->caret;i<edit->length;i++) edit->bytes[i]=edit->bytes[i+1U];
    edit->length--; edit->anchor=edit->caret; clamp_edit(edit); return 1;
}

int nova_textedit_move_left(struct nova_textedit *edit) {
    if (!edit) return 0;
    if (nova_textedit_has_selection(edit)) { unsigned int s,e; nova_textedit_selection_bounds(edit,&s,&e); (void)e; edit->caret=s; edit->anchor=s; return 1; }
    if (!edit->caret) return 0;
    edit->caret--; edit->anchor=edit->caret; return 1;
}

int nova_textedit_move_right(struct nova_textedit *edit) {
    if (!edit) return 0;
    if (nova_textedit_has_selection(edit)) { unsigned int s,e; nova_textedit_selection_bounds(edit,&s,&e); (void)s; edit->caret=e; edit->anchor=e; return 1; }
    if (edit->caret>=edit->length) return 0;
    edit->caret++; edit->anchor=edit->caret; return 1;
}

int nova_textedit_move_left_select(struct nova_textedit *edit) {
    if (!edit || !edit->caret) return 0;
    edit->caret--; return 1;
}

int nova_textedit_move_right_select(struct nova_textedit *edit) {
    if (!edit || edit->caret>=edit->length) return 0;
    edit->caret++; return 1;
}

void nova_textedit_move_home(struct nova_textedit *edit) { if (!edit) return; edit->caret=0U; edit->anchor=0U; }
void nova_textedit_move_end(struct nova_textedit *edit) { if (!edit) return; edit->caret=edit->length; edit->anchor=edit->caret; }
void nova_textedit_select_all(struct nova_textedit *edit) { if (!edit) return; edit->anchor=0U; edit->caret=edit->length; }

unsigned int nova_textedit_copy_selection(const struct nova_textedit *edit, char *out, unsigned int capacity) {
    if (!edit || !out || !capacity || !nova_textedit_has_selection(edit)) { if (out && capacity) out[0]=0; return 0U; }
    unsigned int start,end; nova_textedit_selection_bounds(edit,&start,&end); unsigned int n=end-start;
    if (n>=capacity) n=capacity-1U;
    for (unsigned int i=0;i<n;i++) out[i]=edit->bytes[start+i];
    out[n]=0; return n;
}
