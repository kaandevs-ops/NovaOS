#include "nova_widget.h"

void nova_widget_tree_init(struct nova_widget_tree *tree) {
    if (!tree) return;
    for (unsigned int i=0;i<NOVA_WIDGET_MAX_NODES;i++) {
        tree->nodes[i].id=0U; tree->nodes[i].type=0U; tree->nodes[i].parent_id=0U;
        tree->nodes[i].x=0; tree->nodes[i].y=0; tree->nodes[i].width=0U; tree->nodes[i].height=0U; tree->nodes[i].flags=0U;
    }
    tree->count=0U; tree->focused_id=0U;
}

int nova_widget_add(struct nova_widget_tree *tree, unsigned int id, unsigned int type,
                    unsigned int parent_id, int x, int y, unsigned int width,
                    unsigned int height, unsigned int flags) {
    if (!tree || !id || !width || !height || tree->count>=NOVA_WIDGET_MAX_NODES) return 0;
    for (unsigned int i=0;i<tree->count;i++) if (tree->nodes[i].id==id) return 0;
    if (parent_id) {
        int found=0; for (unsigned int i=0;i<tree->count;i++) if (tree->nodes[i].id==parent_id) found=1;
        if (!found) return 0;
    }
    struct nova_widget_node *n=&tree->nodes[tree->count++];
    n->id=id; n->type=type; n->parent_id=parent_id; n->x=x; n->y=y; n->width=width; n->height=height; n->flags=flags;
    return 1;
}

struct nova_widget_node *nova_widget_find(struct nova_widget_tree *tree, unsigned int id) {
    if (!tree || !id) return (struct nova_widget_node *)0;
    for (unsigned int i=0;i<tree->count;i++) if (tree->nodes[i].id==id) return &tree->nodes[i];
    return (struct nova_widget_node *)0;
}

unsigned int nova_widget_hit_test(const struct nova_widget_tree *tree, int x, int y) {
    if (!tree) return 0U;
    for (unsigned int i=tree->count;i>0U;i--) {
        const struct nova_widget_node *n=&tree->nodes[i-1U];
        if (!(n->flags&NOVA_WIDGET_FLAG_FOCUSABLE)) continue;
        if (x>=n->x && y>=n->y && x<n->x+(int)n->width && y<n->y+(int)n->height) return n->id;
    }
    return 0U;
}

unsigned int nova_widget_focus_first(struct nova_widget_tree *tree) {
    if (!tree) return 0U;
    for (unsigned int i=0;i<tree->count;i++) if (tree->nodes[i].flags&NOVA_WIDGET_FLAG_FOCUSABLE) {
        tree->focused_id=tree->nodes[i].id; return tree->focused_id;
    }
    tree->focused_id=0U; return 0U;
}

unsigned int nova_widget_focus_next(struct nova_widget_tree *tree) {
    if (!tree || !tree->count) return 0U;
    unsigned int start=0U;
    for (unsigned int i=0;i<tree->count;i++) if (tree->nodes[i].id==tree->focused_id) { start=i+1U; break; }
    for (unsigned int offset=0U;offset<tree->count;offset++) {
        unsigned int i=(start+offset)%tree->count;
        if (tree->nodes[i].flags&NOVA_WIDGET_FLAG_FOCUSABLE) { tree->focused_id=tree->nodes[i].id; return tree->focused_id; }
    }
    tree->focused_id=0U; return 0U;
}

int nova_widget_set_focus(struct nova_widget_tree *tree, unsigned int id) {
    struct nova_widget_node *n=nova_widget_find(tree,id);
    if (!n || !(n->flags&NOVA_WIDGET_FLAG_FOCUSABLE)) return 0;
    tree->focused_id=id; return 1;
}
