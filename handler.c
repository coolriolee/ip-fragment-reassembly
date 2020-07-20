#include <time.h>
#include "handler.h"

void rte_mbuf_init(void *p)
{
    struct rte_mbuf_t *mbuf = (struct rte_mbuf_t *)p;
    memset(mbuf,0,sizeof(struct rte_mbuf_t));
    mbuf->data_off = RETMBUF_DATA_OFFSET;
}

void rte_mbuf_free(void *p)
{
    struct rte_mbuf_t *mbuf = (struct rte_mbuf_t *)p;
    free(mbuf);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
struct set_tree inet_frag_settree;

int frag_queue_cmp(const void *a,const void *b)
{
    struct ipv4_frag_compare_key *pA = &((struct frag_queue_t *)a)->key.v4;
    struct ipv4_frag_compare_key *pB = &((struct frag_queue_t *)b)->key.v4;

    if (pA->daddr < pB->daddr) return -1;
    else if (pA->daddr > pB->daddr) return 1;

    if (pA->saddr < pB->saddr) return -1;
    else if (pA->saddr > pB->saddr) return 1;

    if (pA->id < pB->id) return -1;
    else if (pA->id > pB->id) return 1;

    if (pA->protocol < pB->protocol) return -1;
    else if (pA->protocol > pB->protocol) return 1;

    return 0;
}

void *frag_queue_alloc(struct ipv4_frag_compare_key key_v4)
{
    struct frag_queue_t *fqueue = (struct frag_queue_t *)malloc(sizeof (struct frag_queue_t));
    memset(fqueue,0,sizeof (struct frag_queue_t));
    memcpy(&fqueue->key.v4,&key_v4,sizeof(struct ipv4_frag_compare_key));
    key_tree_init(&fqueue->fragments);
    fqueue->firststamp = time(0);
    return fqueue;
}

void frag_queue_destory(void *p)
{
    struct frag_queue_t *fqueue = (struct frag_queue_t *)p;
    if (fqueue) {
        key_tree_destroy2(&fqueue->fragments,rte_mbuf_free);
        free(fqueue);
    }
}

void *frag_queue_find(struct set_tree *settree,struct ipv4_frag_compare_key key_v4)
{
    struct frag_queue_t tmp;
    memcpy(&tmp.key.v4,&key_v4,sizeof(struct ipv4_frag_compare_key));
    struct set_node *snode = set_rbsearch(settree,&tmp,frag_queue_cmp);
    if (snode && snode->data)
        return snode->data;

    struct frag_queue_t *fqueue = (struct frag_queue_t *)frag_queue_alloc(key_v4);
    set_tree_lock(settree);
    snode = set_rbinsert(settree,fqueue,frag_queue_cmp);
    set_tree_unlock(settree);
    if (snode) {
        void *pdata = snode->data;
        if (pdata) {
            frag_queue_destory(fqueue);
            fqueue = pdata;
        } else {
            snode->data = fqueue;
        }
        fprintf(stdout,"fqueue already exsit.\r\n");
    }
    return fqueue;
}

void frag_queue_insert(struct frag_queue_t *fqueue,struct rte_mbuf_t *mbuf)
{

}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int ipv4_handler(struct set_tree *settree,struct rte_mbuf_t *mbuf)
{
    struct iphdr *ip = (struct iphdr *)mbuf->pl3;
    struct ipv4_frag_compare_key key;
//    memset(&key,0,sizeof(struct ipv4_frag_compare_key));
    key.daddr = ip->daddr;
    key.saddr = ip->saddr;
    key.id = ip->id;
    key.protocol = ip->protocol;

    if (ipv4_is_fragment(ip)) {
        struct frag_queue_t *fqueue = (struct frag_queue_t *)frag_queue_find(settree,key);

        if (ipv4_is_first_fragment(ip)) {
            //first fragment block
            fqueue->flags |= IPFRAG_FIRST_IN;
        } else if (ipv4_is_last_fragment(ip)) {
            //last fragment block
            fqueue->flags |= IPFRAG_LAST_IN;
        } else {

        }

        frag_queue_insert(fqueue,mbuf);
        int fragment_offset = ipv4_fragment_offset(ip);
        printf("[MF]%d offset[%d]\r\n",mbuf->data_len,fragment_offset);

        return -1;
    } else {
        printf("[DF]%d\r\n",mbuf->data_len);
        return 0;
    }
}
