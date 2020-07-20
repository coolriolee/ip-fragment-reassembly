#include "tag_elem.h"
#include <string.h>

void tag_tree_init(struct tag_tree *tree)
{
    memset(tree,0,sizeof(struct tag_tree));
    tag_tree_init_lock(tree);
}

__always_inline void tag_node_init(struct tag_node *elem)
{
    struct rb_node *rb = &elem->node;
    memset(elem,0,sizeof(struct tag_node));
    RB_CLEAR_NODE(rb);
}

__always_inline struct tag_node *tag_first(struct tag_tree *tree)
{
    struct rb_node *t = rb_first(&tree->root);
    return t ? rb_entry(t,struct tag_node,node) : NULL;
}

__always_inline struct tag_node *tag_next(struct tag_node *node_t)
{
    struct rb_node *t = rb_next(&node_t->node);
    return t ? rb_entry(t,struct tag_node,node) : NULL;
}

struct tag_node *tag_rbinsert(struct tag_tree *tree,const key_type first,void *data,unsigned long tag)
{
    struct rb_root *root = &tree->root;
    struct rb_node **new_node=&(root->rb_node), *parent=NULL;
    /* Figure out where to put new node  */
    while(*new_node) {
        parent = *new_node;
        if(first < rb_entry(parent, struct tag_node,node)->key) {
            new_node = &(parent->rb_left);
        } else if(first > rb_entry(parent, struct tag_node,node)->key) {
            new_node = &(parent->rb_right);
        } else
            return rb_entry(parent,struct tag_node,node);
    }

    /* Add new node and rebalance tree  */
    struct tag_node *node_t=(struct tag_node *)malloc(sizeof(struct tag_node));
    tag_node_init(node_t);
    node_t->private_key=first;  node_t->data=data;    node_t->tag = tag;
    rb_link_node(&node_t->node, parent, new_node);
    rb_insert_color(&node_t->node, root);
    ++tree->ncount;
    return NULL;
}

struct tag_node *tag_rbinsert2(struct tag_tree *tree, const key_type first, unsigned long long second, unsigned long tag)
{
    struct rb_root *root = &tree->root;
    struct rb_node **new_node=&(root->rb_node), *parent=NULL;
    /* Figure out where to put new node  */
    while(*new_node) {
        parent = *new_node;
        if(first < rb_entry(parent, struct tag_node,node)->key) {
            new_node = &(parent->rb_left);
        } else if(first > rb_entry(parent, struct tag_node,node)->key) {
            new_node = &(parent->rb_right);
        } else
            return rb_entry(parent,struct tag_node,node);
    }

    /* Add new node and rebalance tree  */
    struct tag_node *node_t=(struct tag_node *)malloc(sizeof(struct tag_node));
    tag_node_init(node_t);
    node_t->private_key=first;  node_t->second=second;    node_t->tag = tag;
    rb_link_node(&node_t->node, parent, new_node);
    rb_insert_color(&node_t->node, root);
    ++tree->ncount;
    return NULL;
}

struct tag_node *tag_rbinsert_mp(struct tag_tree *tree, const key_type first, void *data, unsigned long tag)
{
    struct rb_root *root;
    struct rb_node **new_node,*parent;
    parent=NULL;
    tag_tree_lock(tree);
    root = &tree->root;
    new_node=&(root->rb_node);

    /* Figure out where to put new node  */
    while(*new_node) {
        parent = *new_node;
        if(first < rb_entry(parent, struct tag_node,node)->key) {
            new_node = &(parent->rb_left);
        } else if(first > rb_entry(parent, struct tag_node,node)->key) {
            new_node = &(parent->rb_right);
        } else {
            tag_tree_unlock(tree);
            return rb_entry(parent,struct tag_node,node);
        }
    }

    /* Add new node and rebalance tree  */
    struct tag_node *node_t=(struct tag_node *)malloc(sizeof(struct tag_node));
    tag_node_init(node_t);
    node_t->private_key=first;  node_t->data=data;    node_t->tag = tag;
    rb_link_node(&node_t->node, parent, new_node);
    rb_insert_color(&node_t->node, root);
    ++tree->ncount;
    tag_tree_unlock(tree);
    return NULL;
}

struct tag_node *tag_rbinsert2_mp(struct tag_tree *tree, const key_type first, unsigned long long second, unsigned long tag)
{
    struct rb_root *root;
    struct rb_node **new_node,*parent;
    parent=NULL;
    tag_tree_lock(tree);
    root = &tree->root;
    new_node=&(root->rb_node);

    /* Figure out where to put new node  */
    while(*new_node) {
        parent = *new_node;
        if(first < rb_entry(parent, struct tag_node,node)->key) {
            new_node = &(parent->rb_left);
        } else if(first > rb_entry(parent, struct tag_node,node)->key) {
            new_node = &(parent->rb_right);
        } else {
            tag_tree_unlock(tree);
            return rb_entry(parent,struct tag_node,node);
        }
    }

    /* Add new node and rebalance tree  */
    struct tag_node *node_t=(struct tag_node *)malloc(sizeof(struct tag_node));
    tag_node_init(node_t);
    node_t->private_key=first;  node_t->second=second;    node_t->tag = tag;
    rb_link_node(&node_t->node, parent, new_node);
    rb_insert_color(&node_t->node, root);
    ++tree->ncount;
    tag_tree_unlock(tree);
    return NULL;
}

struct tag_node *tag_rbsearch(struct tag_tree *tree,const key_type key)
{
    struct rb_root *root = &tree->root;
    struct rb_node *node=root->rb_node;
    while(node) {
        if(key < rb_entry(node, struct tag_node,node)->key)  {
            node = node->rb_left;
        } else if(key > rb_entry(node, struct tag_node,node)->key) {
            node = node->rb_right;
        } else
            return rb_entry(node, struct tag_node,node);
    }
    return NULL;
}

struct tag_node *tag_rberase(struct tag_tree *tree,struct tag_node *node)
{
    struct rb_root *root = &tree->root;
    struct rb_node *n = &node->node;
    struct rb_node *t = rb_next(n);
    rb_erase(n,root);
    free(node);
    --tree->ncount;
    return t ? rb_entry(t, struct tag_node,node) : NULL;//    return node;
}

struct tag_node *tag_rberase_EX(struct tag_tree *tree,struct tag_node *node_it,void *param,void (*freec)(void*,void*))
{
    struct rb_root *root=&tree->root;
    struct rb_node *n = &node_it->node;
    struct rb_node *t = rb_next(n);
    rb_erase(n,root);
    freec(node_it,param);//free(data);
    --tree->ncount;
    return t ? rb_entry(t, struct tag_node,node) : NULL;
}

void *tag_rberase_by_key(struct tag_tree *tree,const key_type key)
{
    struct rb_root *root = &tree->root;
    struct rb_node *node=root->rb_node;
    while(node) {
        if(key < rb_entry(node, struct tag_node,node)->key) {
            node = node->rb_left;
        } else if(key > rb_entry(node, struct tag_node,node)->key) {
            node = node->rb_right;
        } else {
            struct tag_node *elem = rb_entry(node, struct tag_node,node);
            void *pdata=elem->data;
            rb_erase(node,root);
            --tree->ncount;
            free(elem);
            return pdata;
        }
    }
    return NULL;
}

void *tag_rberase_by_key_EX(struct tag_tree *tree,const key_type key,void *param,void (*func)(void*,void*))
{
    struct rb_root *root = &tree->root;
    struct rb_node *node=root->rb_node;
    while(node) {
        if(key < rb_entry(node, struct tag_node,node)->key) {
            node = node->rb_left;
        } else if(key > rb_entry(node, struct tag_node,node)->key) {
            node = node->rb_right;
        } else {
            struct tag_node *elem = rb_entry(node, struct tag_node,node);
            void *pdata=elem->data;
            rb_erase(node,root);
            --tree->ncount;
            func(elem,param);//free(data);
            return pdata;
        }
    }
    return NULL;
}

void tag_rbclear_Ex(struct tag_tree *tree, void (*freedate)(void *))
{
    struct rb_root *root = &tree->root;
    if(freedate) {
        struct rb_node *node;
        struct tag_node *tag;
        void *pdata;
        for(node=rb_first(root);node; ) {
            tag = rb_entry(node,struct tag_node,node);
            pdata = tag->data;
            tag = (tag_rberase(tree,tag));
            node = tag ? &tag->node : NULL;
            freedate(pdata);
        }
    } else {
        struct rb_node *node;
        struct tag_node *tag;
        for(node=rb_first(root);node; ) {
            tag = rb_entry(node,struct tag_node,node);
            tag = tag_rberase(tree,tag);
            node = tag ? &tag->node : NULL;
        }
    }
}

void tag_tree_destroy(struct tag_tree *tree)
{
    tag_rbclear_Ex(tree,NULL);
    tag_tree_destroy_lock(tree);
}

void tag_tree_destroy2(struct tag_tree *tree, void (*freedate)(void *))
{
    tag_rbclear_Ex(tree,freedate);
    tag_tree_destroy_lock(tree);
}

struct tag_node *Tag_FindLastNode_FollowKey(struct tag_tree *tree,const unsigned int key)
{
    struct rb_root *root = &tree->root;
    struct rb_node *last_right=NULL,*last_left=NULL,*last=NULL;
    struct rb_node *node=root->rb_node;
    while(node) {
        last=node;
        if(key < rb_entry(node, struct tag_node,node)->key)  {
            last_left=node;
            node = node->rb_left;
        } else if(key > rb_entry(node, struct tag_node,node)->key) {
            last_right=node;
            node = node->rb_right;
        } else
            break;//return node;
    }
    if(last_left && rb_entry(last_left, struct tag_node,node)->key >=key){
        return rb_entry(last_left, struct tag_node,node);
    } else if(last_right &&  rb_entry(last_right, struct tag_node,node)->key >=key){
        return rb_entry(last_right, struct tag_node,node);
    }

    return last ? rb_entry(last, struct tag_node,node) : NULL;
}
