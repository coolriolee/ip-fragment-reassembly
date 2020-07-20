#include "set_elem.h"
#include <string.h>

__always_inline struct set_node *set_first(struct set_tree *tree)
{
    struct rb_node *t = rb_first(&tree->root);
    return t ? rb_entry(t,struct set_node,node) : NULL;
}

__always_inline struct set_node *set_next(struct set_node *node_t)
{
    struct rb_node *t = rb_next(&node_t->node);
    return t ? rb_entry(t,struct set_node,node) : NULL;
}

void set_tree_init(struct set_tree *tree)
{
    memset(tree,0,sizeof(struct set_tree));
    set_tree_init_lock(tree);
}

__always_inline void set_node_init(struct set_node *elem)
{
    memset(elem,0,sizeof(struct set_node));
    struct rb_node *rb = &elem->node;
    RB_CLEAR_NODE(rb);
}

struct set_node *set_rbinsert2(struct set_tree *tree,void *data, int (*cmp)(const void *, const void *),void *param,void *(*mallocfunc)(void *))
{
    struct rb_root *root = &tree->root;
    struct rb_node **new_node=&(root->rb_node), *parent=NULL;
    /* Figure out where to put new node  */
    int r;
    while(*new_node) {
        parent = *new_node;
        r = cmp(data,rb_entry(parent, struct set_node,node)->data);
        if(r < 0) {
            new_node = &(parent->rb_left);
        } else if(r > 0) {
            new_node = &(parent->rb_right);
        } else
            return rb_entry(parent,struct set_node,node);
    }

    /* Add new node and rebalance tree  */
    struct set_node *node_t=(struct set_node *)mallocfunc(param);
    set_node_init(node_t);
    node_t->data=data;
    rb_link_node(&node_t->node, parent, new_node);
    rb_insert_color(&node_t->node, root);
    ++tree->ncount;
    return NULL;
}

struct set_node *set_rbinsert(struct set_tree *tree,void *data, int (*cmp)(const void *, const void *))
{
    struct rb_root *root = &tree->root;
    struct rb_node **new_node=&(root->rb_node), *parent=NULL;
    /* Figure out where to put new node  */
    int r;
    while (*new_node) {
        parent = *new_node;
        r = cmp(data,rb_entry(parent, struct set_node,node)->data);
        if(r < 0) {
            new_node = &(parent->rb_left);
        } else if(r > 0) {
            new_node = &(parent->rb_right);
        } else
            return rb_entry(parent,struct set_node,node);
    }

    /* Add new node and rebalance tree  */
    struct set_node *node_t=(struct set_node *)malloc(sizeof(struct set_node));
    set_node_init(node_t);
    node_t->data=data;
    rb_link_node(&node_t->node, parent, new_node);
    rb_insert_color(&node_t->node, root);
    ++tree->ncount;
    return NULL;
}

struct set_node *set_rbsearch(struct set_tree *tree,void *data, int (*cmp)(const void *, const void *))
{
    struct rb_root *root = &tree->root;
    struct rb_node *node=root->rb_node;
    int r;
    while(node) {
        r = cmp(data, rb_entry(node, struct set_node,node)->data);
        if(r < 0)  {
            node = node->rb_left;
        } else if(r > 0) {
            node = node->rb_right;
        } else
            return rb_entry(node, struct set_node,node);
    }
    return NULL;
}

struct set_node *set_rberase(struct set_tree *tree,struct set_node *elem)
{
    struct rb_root *root = &tree->root;
    struct rb_node *n = &elem->node;
    struct rb_node *t = rb_next(n);
    rb_erase(n,root);
    free(elem);
    --tree->ncount;
    return t ? rb_entry(t, struct set_node,node) : NULL;//    return node;
}

struct set_node *set_rberase_EX(struct set_tree *tree,struct set_node *node_it,void *param,void (*freec)(void*,void*))
{
    struct rb_root *root=&tree->root;
    struct rb_node *n = &node_it->node;
    struct rb_node *t = rb_next(n);
    rb_erase(n,root);
    freec(node_it,param);//free(data);
    --tree->ncount;
    return t ? rb_entry(t, struct set_node,node) : NULL;
}

struct set_node *set_rberase_EX_mc(struct set_tree *tree,struct set_node *node_it,void *param,void (*freec)(void*,void*))
{
    struct rb_node *n,*t;
    struct rb_root *root;
    set_tree_lock(tree);
    root=&tree->root;
    n = &node_it->node;
    t = rb_next(n);
    rb_erase(n,root);
    --tree->ncount;
    set_tree_unlock(tree);
    freec(node_it,param);//free(data);
    return t ? rb_entry(t, struct set_node,node) : NULL;
}

void set_rbclear(struct set_tree *tree)
{
    struct rb_root *root = &tree->root;
    struct rb_node *node;
    struct set_node *set_node;
    for(node=rb_first(root);node; ) {
        set_node = rb_entry(node,struct set_node,node);
        set_node = set_rberase(tree,set_node);
        node = set_node ? &set_node->node : NULL;
    }
}

void set_rbclear_Ex(struct set_tree *tree, void (*freedate)(void *))
{
    struct rb_root *root = &tree->root;
    if(freedate) {
        struct rb_node *node;
        struct set_node *set_node;
        void *pdata;
        for(node=rb_first(root);node; ) {
            set_node = rb_entry(node,struct set_node,node);
            pdata = set_node->data;
            set_node = set_rberase(tree,set_node);
            node =set_node ? &set_node->node : NULL;
            freedate(pdata);
        }
    } else {
        set_rbclear(tree);
    }
}

void *set_rberase_by_data(struct set_tree *tree,void  *data, int (*cmp)(const void *, const void *))
{
    struct rb_root *root = &tree->root;
    struct rb_node *node=root->rb_node;
    int r;
    while(node) {
        r = cmp(data,rb_entry(node, struct set_node,node)->data);
        if(r < 0) {
            node = node->rb_left;
        } else if(r > 0) {
            node = node->rb_right;
        } else {
            struct set_node *elem = rb_entry(node, struct set_node,node);
            void *pdata=elem->data;
            rb_erase(node,root);
            --tree->ncount;
            free(elem);
            return pdata;
        }
    }
    return NULL;
}

void *set_rberase_by_data_Ex(struct set_tree *tree,void  *data, int (*cmp)(const void *, const void *),void *param,void(*func)(void *,void *))
{
    struct rb_root *root = &tree->root;
    struct rb_node *node=root->rb_node;
    int r;
    while(node) {
        r = cmp(data,rb_entry(node, struct set_node,node)->data);
        if(r < 0) {
            node = node->rb_left;
        } else if(r > 0) {
            node = node->rb_right;
        } else {
            struct set_node *elem = rb_entry(node, struct set_node,node);
            void *pdata=elem->data;
            rb_erase(node,root);
            --tree->ncount;
            func(elem,param);//free(data);
            return pdata;
        }
    }
    return NULL;
}

struct set_node *SetTree_FindLastNode_FollowKey(struct set_tree *tree,void *key,int (*cmp)(const void *, const void *))
{
    struct rb_root *root = &tree->root;
    struct rb_node *last_right,*last_left  __attribute__((unused));
    struct rb_node *last;
    struct rb_node *node=root->rb_node;
    void *data;
    int r;
    last_left = NULL;
    last_right = NULL;
    last = NULL;
    while(node) {
        data = rb_entry(node, struct set_node,node)->data;
        r = cmp(key,data);
        last=node;
        if(r<0)  {
            last_left=node;
            node = node->rb_left;
        } else if(r>0) {
            last_right=node;
            node = node->rb_right;
        } else
            return  rb_entry(last, struct set_node,node);
    }

    if(last_right) {
        data = rb_entry(last_right, struct set_node,node)->data;
        if(cmp(key,data) >= 0)
            return rb_entry(last_right, struct set_node,node);
    }
//    if(last_left) {
//        data = rb_entry(last_left, struct set_node,node)->data;
//        if(cmp(data,key) >= 0)
//            return rb_entry(last_left, struct set_node,node);
//    }
    return NULL;
    //return last ? rb_entry(last, struct set_node,node) : NULL;
}


struct set_node *SetTree_FindLastNode_LQ_Key(struct set_tree *tree,void *key,int (*cmp)(const void *, const void *))
{
    struct rb_root *root = &tree->root;
    struct rb_node *last_right,*last_left  __attribute__((unused));
    struct rb_node *last;
    struct rb_node *node=root->rb_node;
    void *data;
    int r;
    last_left = NULL;
    last_right = NULL;
    last = NULL;
    while(node) {
        data = rb_entry(node, struct set_node,node)->data;
        r = cmp(key,data);
        last=node;
        if(r<0)  {
            last_left=node;
            node = node->rb_left;
        } else if(r>0) {
            last_right=node;
            node = node->rb_right;
        } else
            return  rb_entry(last, struct set_node,node);
    }

    if(last_right) {
        data = rb_entry(last_right, struct set_node,node)->data;
        if(cmp(key,data) >= 0)
            return rb_entry(last_right, struct set_node,node);
    }
    if(last_left) {
        data = rb_entry(last_left, struct set_node,node)->data;
        if(cmp(key,data) >= 0)
            return rb_entry(last_left, struct set_node,node);
    }
    return NULL;
    //return last ? rb_entry(last, struct set_node,node) : NULL;
}


void set_tree_destroy(struct set_tree *tree, void(*freec)(void *))
{
    set_rbclear_Ex(tree,freec);
    set_tree_destroy_lock(tree);
}
