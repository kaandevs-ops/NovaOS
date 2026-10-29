#ifndef NOVA_WIDGET_H
#define NOVA_WIDGET_H

#define NOVA_WIDGET_MAX_NODES 8U
#define NOVA_WIDGET_FLAG_FOCUSABLE 0x1U

enum {
    NOVA_WIDGET_ROOT = 1U,
    NOVA_WIDGET_TEXT_FIELD = 2U,
    NOVA_WIDGET_BUTTON = 3U,
};

struct nova_widget_node {
    unsigned int id;
    unsigned int type;
    unsigned int parent_id;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    unsigned int flags;
};

struct nova_widget_tree {
    struct nova_widget_node nodes[NOVA_WIDGET_MAX_NODES];
    unsigned int count;
    unsigned int focused_id;
};

void nova_widget_tree_init(struct nova_widget_tree *tree);
int nova_widget_add(struct nova_widget_tree *tree, unsigned int id, unsigned int type,
                    unsigned int parent_id, int x, int y, unsigned int width,
                    unsigned int height, unsigned int flags);
struct nova_widget_node *nova_widget_find(struct nova_widget_tree *tree, unsigned int id);
unsigned int nova_widget_hit_test(const struct nova_widget_tree *tree, int x, int y);
unsigned int nova_widget_focus_first(struct nova_widget_tree *tree);
unsigned int nova_widget_focus_next(struct nova_widget_tree *tree);
int nova_widget_set_focus(struct nova_widget_tree *tree, unsigned int id);

#endif
