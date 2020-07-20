#ifndef TAG_ELEM_H
#define TAG_ELEM_H
#include "rbtree.h"
#include <sched.h>

struct tag_node {
    union {
        const key_type key;
        key_type private_key;
    };
    union {
        data_type data;
        unsigned long long second;
    };
    union {
        time_t tag;
        void *pTag;
    };
    struct rb_node node;
};

struct tag_tree {
    unsigned long long ncount;
    union {
        time_t erase_tick;
        time_t last_tick;
    };
    struct rb_root root;
#ifdef USE_SPIN_LOCK
    pthread_spinlock_t lock;
#else
    int mutex;// = 0;
#endif
};

#ifdef USE_SPIN_LOCK
#define tag_tree_init_lock(p) { pthread_spin_init(&(p)->lock,0);  }
#define tag_tree_destroy_lock(p) { pthread_spin_destroy(&(p)->lock); }
#define tag_tree_unlock(p)  { pthread_spin_unlock(&(p)->lock); }
#define tag_tree_lock(p)     { pthread_spin_lock(&(p)->lock); }
#else
#define tag_tree_init_lock(p) {(p)->mutex = 0;}
#define tag_tree_destroy_lock(p) {}
#define tag_tree_unlock(p) {__sync_bool_compare_and_swap(&(p)->mutex,1,0);}
#define tag_tree_lock(p)     { while (!(__sync_bool_compare_and_swap (&(p)->mutex,0, 1) )) {sched_yield();} smp_rmb();}
#endif

#define TAG_TREE_NODES(tree) ((tree)->ncount)

extern struct tag_node *tag_first(struct tag_tree *tree);
extern struct tag_node *tag_next(struct tag_node *node_t);

void tag_tree_init(struct tag_tree *tree);
void tag_tree_destroy(struct tag_tree *tree);
void tag_tree_destroy2(struct tag_tree *tree, void (*freedate)(void *));
struct tag_node *tag_rbinsert(struct tag_tree *tree, const key_type first, void *data, unsigned long tag);
struct tag_node *tag_rbinsert_mp(struct tag_tree *tree, const key_type first, void *data, unsigned long tag);
struct tag_node *tag_rbinsert2(struct tag_tree *tree, const key_type first, unsigned long long second, unsigned long tag);
struct tag_node *tag_rbinsert2_mp(struct tag_tree *tree, const key_type first, unsigned long long second, unsigned long tag);
struct tag_node *tag_rbsearch(struct tag_tree *tree,const key_type key);
struct tag_node *tag_rberase(struct tag_tree *tree,struct tag_node *node);
struct tag_node *tag_rberase_EX(struct tag_tree *tree,struct tag_node *node_it,void *param,void (*freec)(void*,void*));
void *tag_rberase_by_key(struct tag_tree *tree,const key_type key);
void *tag_rberase_by_key_EX(struct tag_tree *tree,const key_type key,void *param,void (*func)(void*,void*));
void tag_rbclear_Ex(struct tag_tree *tree, void (*freedate)(void *));
struct tag_node *Tag_FindLastNode_FollowKey(struct tag_tree *tree,const unsigned int key);

#endif // TAG_ELEM_H
