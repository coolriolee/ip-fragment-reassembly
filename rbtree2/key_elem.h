#ifndef __KEY_ELEM_H__
#define __KEY_ELEM_H__

#include "rbtree.h"
#include <sched.h>

struct key_node
{
    struct rb_node node;
    union{
        const key_type key;
        key_type private_key;
    };
    union {
        data_type data;
        unsigned long long second;
    };
};


struct key_tree
{
    struct rb_root root;
    union{
        time_t erase_tick;
        time_t last_tick;
    };
#ifdef USE_SPIN_LOCK
    pthread_spinlock_t lock;
#else
    unsigned int mutex;// = 0;
#endif
    unsigned int ncount;
};

#ifdef USE_SPIN_LOCK
#define key_tree_init_lock(p) { pthread_spin_init(&(p)->lock,0); }
#define key_tree_destroy_lock(p) { pthread_spin_destroy(&(p)->lock); }
#define key_tree_unlock(p)  { pthread_spin_unlock(&(p)->lock); }
#define key_tree_lock(p)     { pthread_spin_lock(&(p)->lock); }
#else
#define key_tree_init_lock(p) { (p)->mutex=0;}
#define key_tree_destroy_lock(p) {}
#define key_tree_unlock(p)  { __sync_bool_compare_and_swap(&(p)->mutex,1,0);}
#define key_tree_lock(p)     { while (!(__sync_bool_compare_and_swap (&(p)->mutex,0, 1) )) {sched_yield();} smp_rmb();}
#endif

#define KEY_TREE_NODES(tree) ((tree)->ncount)
extern struct key_node *key_first(struct key_tree *tree);
extern struct key_node *key_last(struct key_tree *tree);
extern struct key_node *key_next(struct key_node *node_t);
extern struct key_node *key_prev(struct key_node *node_t);
void key_tree_init(struct key_tree *tree);
void key_tree_destroy(struct key_tree *tree);
void key_tree_destroy2(struct key_tree *tree,void (*freedata)(void *));
struct key_node* key_rbinsert(struct key_tree *tree, const key_type first, void *second);
struct key_node* key_rbinsert_u(struct key_tree *tree, const key_type first, unsigned long long second);
struct key_node *key_rbinsert_mp(struct key_tree *tree,const key_type first,void *second);

struct key_node* key_rbsearch(struct key_tree *tree,const key_type key);

struct key_node *key_rberase(struct key_tree *tree, struct key_node *elem);
struct key_node *key_rberase_EX(struct key_tree *tree,struct key_node *node_it,void *param,void (*freec)(void*,void*));

void *key_rberase_by_key(struct key_tree *tree,const key_type key);
void *key_rberase_by_key_EX(struct key_tree *tree,const key_type key,void *param,void (*func)(void*,void*));

void key_rbclear_Ex(struct key_tree *tree, void (*freedate)(void *));
void key_rbclear_Ex2(struct key_tree *tree,void *param,void (*freenode)(void*,void*), void (*freedate)(void *));

struct key_node *Rbtree_FindLastNode_FollowKey(struct key_tree *tree,struct key_node **right,struct key_node **left, const    unsigned int key);
struct key_node *KeyTree_FindLastNode_LQ_Key(struct key_tree *tree,const unsigned int key);

#endif // SET_ELEM_H
