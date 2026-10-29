#ifndef NOVA_TEXTEDIT_H
#define NOVA_TEXTEDIT_H

#define NOVA_TEXTEDIT_CAPACITY 32U

struct nova_textedit {
    char bytes[NOVA_TEXTEDIT_CAPACITY];
    unsigned int length;
    unsigned int caret;
    unsigned int anchor;
};

void nova_textedit_init(struct nova_textedit *edit, const char *initial);
int nova_textedit_insert_ascii(struct nova_textedit *edit, unsigned int codepoint);
int nova_textedit_insert_ascii_bytes(struct nova_textedit *edit, const char *bytes, unsigned int length);
int nova_textedit_backspace(struct nova_textedit *edit);
int nova_textedit_delete(struct nova_textedit *edit);
int nova_textedit_delete_selection(struct nova_textedit *edit);
int nova_textedit_move_left(struct nova_textedit *edit);
int nova_textedit_move_right(struct nova_textedit *edit);
int nova_textedit_move_left_select(struct nova_textedit *edit);
int nova_textedit_move_right_select(struct nova_textedit *edit);
void nova_textedit_move_home(struct nova_textedit *edit);
void nova_textedit_move_end(struct nova_textedit *edit);
void nova_textedit_select_all(struct nova_textedit *edit);
int nova_textedit_has_selection(const struct nova_textedit *edit);
void nova_textedit_selection_bounds(const struct nova_textedit *edit, unsigned int *start, unsigned int *end);
unsigned int nova_textedit_copy_selection(const struct nova_textedit *edit, char *out, unsigned int capacity);

#endif
