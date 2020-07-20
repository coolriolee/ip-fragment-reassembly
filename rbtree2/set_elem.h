#ifndef __SET_ELEM_H
#define __SET_ELEM_H
#include "rbtree.h"
#include <sched.h>

struct set_node {
    struct rb_node node;
    union {
        void *data;
        long key;
    };
};

struct set_tree {
    unsigned long long ncount;
    struct rb_root root;
    union {
        time_t erase_tick;
        time_t last_tick;
    };
#ifdef USE_SPIN_LOCK
    pthread_spinlock_t lock;
#else
    int mutex;// = 0;
#endif
};

#ifdef USE_SPIN_LOCK
#define set_tree_init_lock(p) {pthread_spin_init(&(p)->lock,0);  }
#define set_tree_destroy_lock(p) { pthread_spin_destroy(&(p)->lock); }
#define set_tree_unlock(p)  { pthread_spin_unlock(&(p)->lock); }
#define set_tree_lock(p)     { pthread_spin_lock(&(p)->lock); }
#else
#define set_tree_init_lock(p) { (p)->mutex = 0;}
#define set_tree_destroy_lock(p) {}
#define set_tree_unlock(p)  {__sync_bool_compare_and_swap(&(p)->mutex,1,0);}
#define set_tree_lock(p)     { while (!(__sync_bool_compare_and_swap (&(p)->mutex,0, 1) )) {sched_yield();} smp_rmb();}

#endif

#define SET_TREE_NODES(tree) ((tree)->ncount)

extern struct set_node *set_first(struct set_tree *tree);
extern  struct set_node *set_next(struct set_node *node_t);

void set_tree_init(struct set_tree *tree);
void set_tree_destroy(struct set_tree *tree, void(*freec)(void *));
struct set_node *set_rbinsert(struct set_tree *tree,void *data, int (*cmp)(const void *, const void *));
struct set_node *set_rbinsert2(struct set_tree *tree, void *data, int (*cmp)(const void *, const void *),void *param,void* (*mallocfunc)(void *));
struct set_node *set_rbsearch(struct set_tree *tree,void *data, int (*cmp)(const void *, const void *));

struct set_node *set_rberase(struct set_tree *tree, struct set_node *elem);
struct set_node *set_rberase_EX(struct set_tree *tree,struct set_node *node_it,void *param,void (*freec)(void*,void*));
struct set_node *set_rberase_EX_mc(struct set_tree *tree,struct set_node *node_it,void *param,void (*freec)(void*,void*));

void *set_rberase_by_data(struct set_tree *tree,void  *data, int (*func)(const void *, const void *));
void *set_rberase_by_data_Ex(struct set_tree *tree,void  *data, int (*cmp)(const void *, const void *),void *param,void(*func)(void *,void *));

struct set_node *SetTree_FindLastNode_FollowKey(struct set_tree *tree,void *key,int (*cmp)(const void *, const void *));
struct set_node *SetTree_FindLastNode_LQ_Key(struct set_tree *tree,void *key,int (*cmp)(const void *, const void *));

void set_rbclear(struct set_tree *tree);
void set_rbclear_Ex(struct set_tree *tree, void (*freedate)(void *));

#endif // __SET_ELEM_H
