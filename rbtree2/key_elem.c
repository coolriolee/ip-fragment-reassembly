#include "key_elem.h"
#include <string.h>

void key_tree_init(struct key_tree *tree)
{
    memset(tree,0,sizeof(struct key_tree));
    key_tree_init_lock(tree);
}

__always_inline void key_node_init(struct key_node *elem)
{
    struct rb_node *rb = &elem->node;
    memset(elem,0,sizeof(struct key_node));
    RB_CLEAR_NODE(rb);
}

__always_inline struct key_node *key_first(struct key_tree *tree)
{
    struct rb_node *t=rb_first(&tree->root);
    return  t ? rb_entry(t,struct key_node,node) : NULL;
}

__always_inline struct key_node *key_last(struct key_tree *tree)
{
    struct rb_node *t=rb_last(&tree->root);
    return  t ? rb_entry(t,struct key_node,node) : NULL;
}


__always_inline struct key_node *key_next(struct key_node *node_t)
{
    struct rb_node *t = rb_next(&node_t->node);
    return t ? rb_entry(t,struct key_node,node) : NULL;
}

__always_inline struct key_node *key_prev(struct key_node *node_t)
{
    struct rb_node *t = rb_prev(&node_t->node);
    return t ? rb_entry(t,struct key_node,node) : NULL;
}

struct key_node *key_rbinsert(struct key_tree *tree,const key_type first,void *second)
{
    struct rb_root *root = &tree->root;
    struct rb_node **new_node=&(root->rb_node), *parent=NULL;
    /* Figure out where to put new node  */
    while(*new_node) {
        parent = *new_node;
        if(first < rb_entry(parent, struct key_node,node)->key) {
            new_node = &(parent->rb_left);
        } else if(first > rb_entry(parent, struct key_node,node)->key) {
            new_node = &(parent->rb_right);
        } else
            return rb_entry(parent,struct key_node,node);
    }

    /* Add new node and rebalance tree  */
    struct key_node *node_t=(struct key_node *)malloc(sizeof(struct key_node));
    key_node_init(node_t);
    node_t->private_key=first;  node_t->data=second;
    rb_link_node(&node_t->node, parent, new_node);
    rb_insert_color(&node_t->node, root);
    ++tree->ncount;
    return NULL;
}

struct key_node *key_rbinsert_mp(struct key_tree *tree,const key_type first,void *second)
{
    struct rb_root *root;
    struct rb_node **new_node, *parent;
    parent=NULL;

    key_tree_lock(tree);
    root = &tree->root;
    new_node=&(root->rb_node);

    /* Figure out where to put new node  */
    while(*new_node) {
        parent = *new_node;
        if(first < rb_entry(parent, struct key_node,node)->key) {
            new_node = &(parent->rb_left);
        } else if(first > rb_entry(parent, struct key_node,node)->key) {
            new_node = &(parent->rb_right);
        } else {
            key_tree_unlock(tree);
            return rb_entry(parent,struct key_node,node);
        }
    }

    /* Add new node and rebalance tree  */
    struct key_node *node_t=(struct key_node *)malloc(sizeof(struct key_node));
    key_node_init(node_t);
    node_t->private_key=first;  node_t->data=second;
    rb_link_node(&node_t->node, parent, new_node);
    rb_insert_color(&node_t->node, root);
    ++tree->ncount;
    key_tree_unlock(tree);
    return NULL;
}

struct key_node* key_rbinsert_u(struct key_tree *tree,const key_type first,unsigned long long second)
{
    struct rb_root *root = &tree->root;
    struct rb_node **new_node=&(root->rb_node), *parent=NULL;
    /* Figure out where to put new node  */
    while(*new_node) {
        parent = *new_node;
        if(first < rb_entry(parent, struct key_node,node)->key) {
            new_node = &(parent->rb_left);
        } else if(first > rb_entry(parent, struct key_node,node)->key) {
            new_node = &(parent->rb_right);
        } else
            return rb_entry(parent,struct key_node,node);
    }

    /* Add new node and rebalance tree  */
    struct key_node *node_t=(struct key_node *)malloc(sizeof(struct key_node));
    key_node_init(node_t);
    node_t->private_key=first;  node_t->second=second;
    rb_link_node(&node_t->node, parent, new_node);
    rb_insert_color(&node_t->node, root);
    ++tree->ncount;
    return NULL;
}

struct key_node* key_rbsearch(struct key_tree *tree,const key_type key)
{
    struct rb_root *root = &tree->root;
    struct rb_node *node=root->rb_node;
    struct key_node *tmp;
    while(node) {
        tmp = rb_entry(node, struct key_node,node);
        if(key < tmp->key)  {
            node = node->rb_left;
        } else if(key > tmp->key) {
            node = node->rb_right;
        } else
            return tmp;
    }
    return NULL;
}

struct key_node *key_rberase(struct key_tree *tree,struct key_node *elem)
{
    struct rb_root *root = &tree->root;
    struct rb_node *n = &elem->node;
    struct rb_node *t = rb_next(n);
    rb_erase(n,root);
    free(elem);
    --tree->ncount;
    return t ? rb_entry(t, struct key_node,node) : NULL;//    return node;
}
struct key_node *key_rberase_EX(struct key_tree *tree,struct key_node *node_it,void *param,void (*freec)(void*,void*))
{
    struct rb_root *root=&tree->root;
    struct rb_node *n = &node_it->node;
    struct rb_node *t = rb_next(n);
    rb_erase(n,root);
    freec(node_it,param);//free(data);
    --tree->ncount;
    return t ? rb_entry(t, struct key_node,node) : NULL;
}

void *key_rberase_by_key(struct key_tree *tree,const key_type key)
{
    struct rb_root *root = &tree->root;
    struct rb_node *node=root->rb_node;
    while(node) {
        if(key < rb_entry(node, struct key_node,node)->key) {
            node = node->rb_left;
        } else if(key > rb_entry(node, struct key_node,node)->key) {
            node = node->rb_right;
        } else {
            struct key_node *elem = rb_entry(node, struct key_node,node);
            void *pdata=elem->data;
            rb_erase(node,root);
            --tree->ncount;
            free(elem);
            return pdata;
        }
    }
    return NULL;
}

void *key_rberase_by_key_EX(struct key_tree *tree,const key_type key,void *param,void (*func)(void*,void*))
{
    struct rb_root *root = &tree->root;
    struct rb_node *node=root->rb_node;
    while(node) {
        if(key < rb_entry(node, struct key_node,node)->key) {
            node = node->rb_left;
        } else if(key > rb_entry(node, struct key_node,node)->key) {
            node = node->rb_right;
        } else {
            struct key_node *elem = rb_entry(node, struct key_node,node);
            void *pdata=elem->data;
            rb_erase(node,root);
            --tree->ncount;
            func(elem,param);//free(data);
            return pdata;
        }
    }
    return NULL;
}

void key_rbclear_Ex(struct key_tree *tree, void (*freedate)(void *))
{
    struct rb_root *root = &tree->root;
    if(freedate) {
        struct rb_node *node;
        void *pdata;
        struct key_node *key_node;
        for(node=rb_first(root);node; ) {
            key_node = rb_entry(node,struct key_node,node);
            pdata = key_node->data;
            key_node = key_rberase(tree,key_node);
            node= key_node ? &key_node->node : NULL;
            freedate(pdata);
        }
    } else {
        struct rb_node *node;
        struct key_node *key_node;
        for(node=rb_first(root);node; ) {
            key_node = rb_entry(node,struct key_node,node);
            key_node = key_rberase(tree,key_node);
            node= key_node ? &key_node->node : NULL;
        }
    }
}

void key_rbclear_Ex2(struct key_tree *tree,void *param,void (*freenode)(void*,void*), void (*freedate)(void *))
{
    struct rb_root *root = &tree->root;
    if(freedate) {
        struct rb_node *node;
        void *pdata;
        struct key_node *key_node;
        for(node=rb_first(root);node; ) {
            key_node = rb_entry(node,struct key_node,node);
            pdata = key_node->data;
            key_node = key_rberase_EX(tree,key_node,param,freenode);
            node= key_node ? &key_node->node : NULL;
            freedate(pdata);
        }
    } else {
        struct rb_node *node;
        struct key_node *key_node;
        for(node=rb_first(root);node; ) {
            key_node = rb_entry(node,struct key_node,node);
            key_node = key_rberase_EX(tree,key_node,param,freenode);
            node= key_node ? &key_node->node : NULL;
        }
    }
}

void key_tree_destroy(struct key_tree *tree)
{
    key_rbclear_Ex(tree,NULL);
    key_tree_destroy_lock(tree);
}

void key_tree_destroy2(struct key_tree *tree,void (*freedata)(void *))
{
    key_rbclear_Ex(tree,freedata);
    key_tree_destroy_lock(tree);
}

struct key_node *Rbtree_FindLastNode_FollowKey(struct key_tree *tree, struct key_node **right, struct key_node ** left, const unsigned int key)
{
    struct rb_root *root = &tree->root;
    struct rb_node *last_right=NULL,*last_left=NULL,*last=NULL;
    struct rb_node *node=root->rb_node;
    while(node) {
        last=node;
        if(key < rb_entry(node, struct key_node,node)->key)  {
            last_left = node;
            node = node->rb_left;
        } else if(key > rb_entry(node, struct key_node,node)->key) {
            last_right = node;
            node = node->rb_right;
        } else
            return  rb_entry(last, struct key_node,node);
    }
    if(last_right==last)
        last_right = NULL;
    if(last_left==last)
        last_left = NULL;
    if(last_right/* &&  key > rb_entry(last_right, struct key_node,node)->key*/)
        *right = rb_entry(last_right, struct key_node,node);
    if(last_left /*&&  key < rb_entry(last_left, struct key_node,node)->key*/)
        *left = rb_entry(last_left, struct key_node,node);

    if(last){
        //        last_right = rb_next(last);
        //        last_left = rb_prev(last);
        //        *right = last_right ?  rb_entry(last_right, struct key_node,node) : NULL;
        //        *left = last_right ?  rb_entry(last_left, struct key_node,node) : NULL;
        return rb_entry(last, struct key_node,node);
    }
    return NULL;
}

struct key_node *KeyTree_FindLastNode_LQ_Key(struct key_tree *tree,const unsigned int  key)//less or equal
{
    struct rb_root *root = &tree->root;
    struct rb_node *last_right=NULL,*last_left=NULL,*last=NULL;
    struct rb_node *node=root->rb_node;
    while(node) {
        last=node;
        if(key < rb_entry(node, struct key_node,node)->key)  {
            last_left=node;
            node = node->rb_left;
        } else if(key > rb_entry(node, struct key_node,node)->key) {
            last_right=node;
            node = node->rb_right;
        } else
            return  rb_entry(last, struct key_node,node);
    }
    if(last_left && rb_entry(last_left, struct key_node,node)->key <= key){
        return rb_entry(last_left, struct key_node,node);
    } else if(last_right &&  rb_entry(last_right, struct key_node,node)->key <= key){
        return rb_entry(last_right, struct key_node,node);
    }

    return last ? rb_entry(last, struct key_node,node) : NULL;
}
