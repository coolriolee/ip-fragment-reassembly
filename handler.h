#ifndef _FRAGMENT_REASSEMBLY_HANDLER_H
#define _FRAGMENT_REASSEMBLY_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ether.h>
#include <netinet/ip.h>

#include "rbtree2/key_elem.h"
#include "rbtree2/set_elem.h"

struct ipv4_frag_compare_key {
    unsigned int saddr;//netbit
    unsigned int daddr;//netbit
    unsigned short id;//netbit
    unsigned char protocol;
};

#define IPFRAG_INSPIRE_INTERVAL 30/*second*/
#define FRAGMENT_STEP(mtu) (((mtu) - sizeof(struct iphdr))/8)
#define FRAGMENT_SIZE(mtu) (FRAGMENT_STEP((mtu))*8)
//分片分组队列
struct frag_queue_t {
    union {
        struct ipv4_frag_compare_key v4;
    } key;
    struct rte_mbuf_t *first_fragment;
    struct key_tree fragments;//ipv4/ipv6
#define IPFRAG_FIRST_IN     0x01
#define IPFRAG_LAST_IN      0x02
#define IPFRAG_FULL           0x04
#define IPFRAG_EXPIRE        0x08
#define IPFRAG_ERROR        0x10
#define IPFRAG_COMPLETE  0x80
    unsigned char direction;//0:rx 1:tx
    unsigned char flags;
    unsigned int len,meat;
    time_t firststamp;//
    unsigned char erase;
};
#define IPFRAG_DO_REASSEMBLY(flag) (((flag) & IPFRAG_FIRST_IN) && ((flag) & IPFRAG_LAST_IN) && ((flag) & IPFRAG_FULL))
#define IPFRAG_DO_ERASE(flag) (((flag) & IPFRAG_EXPIRE) || ((flag) & IPFRAG_ERROR) || ((flag) & IPFRAG_COMPLETE))

extern void *frag_queue_alloc(struct ipv4_frag_compare_key key_v4);
extern void frag_queue_destory(void *p);
extern void *frag_queue_find(struct set_tree *settree,struct ipv4_frag_compare_key key_v4,const unsigned char direction);

#define RTEMBUF_CACHELINE_64(mark) unsigned char mark[0] __attribute__((aligned(64)))
#define RTEMBUF_TRUESIZE(size) ((size)/64+2)*64
#define RTEMBUF_BUFFERSIZE   1500
#define RTEMBUF_DATA_OFFSET 64
struct rte_mbuf_t {
    unsigned short data_off;//数据偏移
    unsigned int data_len;//数据总长
    unsigned int true_size;
    unsigned char *pl2,*pl3;
    unsigned short l3len,payloadlen;
    struct {
        unsigned char direction:1;//0:rx 1:tx
        unsigned char is_fragment:1;
    } flags;
    unsigned short fragoffset;//分片偏移(8bytes)
    RTEMBUF_CACHELINE_64(cacheline0);
    unsigned char buffer[0];//
}__attribute__((packed));
extern void *rte_mbuf_malloc(const unsigned int size);
extern void rte_mbuf_free(void *p);

extern struct set_tree inet_frag_settree;
extern struct key_tree inet_frag_trash;
extern int ipv4_handler(struct set_tree *settree,struct rte_mbuf_t *mbuf);
extern int ipv4_fragment_recycle(struct key_tree *keytree,struct frag_queue_t *fqueue);
extern int ipv4_fragment_trash_maintain(struct key_tree *keytree);
extern int ipv4_fragment_maintain(struct set_tree *settree);

//ipv4-fragment.c
extern int ipv4_is_dont_fragment(const struct iphdr *ip);
extern int ipv4_is_more_fragment(const struct iphdr *ip);
extern int ipv4_is_fragment(const struct iphdr *ip);
extern int ipv4_is_first_fragment(const struct iphdr *ip);
extern int ipv4_is_last_fragment(const struct iphdr *ip);
extern int ipv4_fragment_offset(const struct iphdr *ip);
extern int ipv4_handle_fragment(struct rte_mbuf_t *mbuf,const unsigned short mtu);

//ipv4-reassembly
extern struct rte_mbuf_t *ipv4_handle_reassembly(struct frag_queue_t *fqueue);

#endif // _FRAGMENT_REASSEMBLY_HANDLER_H
