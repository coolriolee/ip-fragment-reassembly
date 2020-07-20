#ifndef _FRAGMENT_REASSEMBLY_HANDLER_H
#define _FRAGMENT_REASSEMBLY_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rbtree2/key_elem.h"
#include "rbtree2/set_elem.h"
#include "ipv4-fragment.h"
#include "ipv4-reassembly.h"

struct ipv4_frag_compare_key {
    unsigned int saddr;//netbit
    unsigned int daddr;//netbit
    unsigned short id;//netbit
    unsigned char protocol;
};

#define IPFRAG_INSPIRE_INTERVAL 30/*second*/
//分片分组队列
struct frag_queue_t {
    union {
        struct ipv4_frag_compare_key v4;
    } key;
    struct key_tree fragments;//ipv4/ipv6
#define IPFRAG_FIRST_IN  0x01
#define IPFRAG_LAST_IN   0x02
#define IPFRAG_DO_DEL   0x04
    unsigned char flags;
    unsigned int len;
    unsigned int meet;
    time_t firststamp;//
};
extern void *frag_queue_alloc(struct ipv4_frag_compare_key key_v4);
extern void frag_queue_destory(void *p);
extern void *frag_queue_find(struct set_tree *settree,struct ipv4_frag_compare_key key_v4);


#define RETMBUF_BUFFER_LEN   2048
#define RETMBUF_DATA_OFFSET 64
struct rte_mbuf_t {
    unsigned short data_off,data_len;
    unsigned char *pl2,*pl3;
    unsigned char buffer[RETMBUF_BUFFER_LEN];//layer3
}__attribute__((packed));
extern void rte_mbuf_init(void *p);
extern void rte_mbuf_free(void *p);

extern struct set_tree inet_frag_settree;
extern int ipv4_handler(struct set_tree *settree,struct rte_mbuf_t *mbuf);

#endif // _FRAGMENT_REASSEMBLY_HANDLER_H
