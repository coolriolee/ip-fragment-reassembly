#include <time.h>
#include <unistd.h>
#include "handler.h"

void *rte_mbuf_malloc(const unsigned int size)
{
    unsigned int truesize = RTEMBUF_TRUESIZE(size);
    struct rte_mbuf_t *mbuf = (struct rte_mbuf_t *)malloc(sizeof(struct rte_mbuf_t) + truesize);
    memset(mbuf,0,sizeof(struct rte_mbuf_t));
    mbuf->data_off = RTEMBUF_DATA_OFFSET;
    mbuf->true_size = truesize;
    return mbuf;
}

void rte_mbuf_free(void *p)
{
    struct rte_mbuf_t *mbuf = (struct rte_mbuf_t *)p;
    free(mbuf);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
struct set_tree inet_frag_settree;
struct key_tree inet_frag_trash;
extern unsigned int mtu;

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

void *frag_queue_find(struct set_tree *settree,struct ipv4_frag_compare_key key_v4,const unsigned char direction)
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
    fqueue->direction = direction;
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

static int frag_queue_insert(struct frag_queue_t *fqueue,struct rte_mbuf_t *mbuf)
{
    key_tree_lock(&fqueue->fragments);
    struct key_node *knode = key_rbinsert(&fqueue->fragments,mbuf->fragoffset,mbuf);
    key_tree_unlock(&fqueue->fragments);
    if (knode) {
//        void *pdata = knode->data;
//        if (pdata) rte_mbuf_free(mbuf);
//        else knode->data = mbuf;
        fprintf(stdout,"fragment mbuf already exsit.\r\n");
        return -1;
    } else {
        return 0;
    }
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

    mbuf->l3len = ntohs(ip->tot_len);
    mbuf->payloadlen = mbuf->l3len - ip->ihl * 4;

    if (ipv4_is_fragment(ip)) {
        struct frag_queue_t *fqueue = (struct frag_queue_t *)frag_queue_find(settree,key,mbuf->flags.direction);
        mbuf->flags.is_fragment = 1;
        mbuf->fragoffset = ipv4_fragment_offset(ip);

        if (!IPFRAG_DO_REASSEMBLY(fqueue->flags) && !frag_queue_insert(fqueue,mbuf)) {
            //分片入列成功
            fqueue->meat += mbuf->payloadlen;
            if (ipv4_is_first_fragment(ip)) {
                //first fragment block
                fqueue->first_fragment = mbuf;
                fqueue->flags |= IPFRAG_FIRST_IN;
            } else if (ipv4_is_last_fragment(ip)) {
                //last fragment block
                fqueue->flags |= IPFRAG_LAST_IN;
                fqueue->len = mbuf->fragoffset * 8 + mbuf->payloadlen;
            } else {
            }

            if (fqueue->len && fqueue->meat == fqueue->len)
                fqueue->flags |= IPFRAG_FULL;
            else if (fqueue->len && fqueue->meat > fqueue->len)
                fqueue->flags |= IPFRAG_ERROR;
            if (fqueue->direction != mbuf->flags.direction)
                fqueue->flags |= IPFRAG_ERROR;

            return 0;
        } else {
            //分片入列失败(free)
            return 1;
        }
    } else {
        //非IP分片(free)
        return -1;
    }
}

int ipv4_fragment_recycle(struct key_tree *keytree,struct frag_queue_t *fqueue)
{
    static unsigned long trashid=0;
    key_tree_lock(keytree);
    struct key_node *knode = key_rbinsert(keytree,++trashid,fqueue);
    key_tree_unlock(keytree);
    if (knode) {
        usleep(100);
        frag_queue_destory(fqueue);
    }
    return 0;
}

int ipv4_fragment_trash_maintain(struct key_tree *keytree)
{
    struct key_node *knode = key_first(keytree);
    while (knode && knode->data)
    {
        struct frag_queue_t *fqueue = (struct frag_queue_t *)knode->data;
        if (++fqueue->erase >= 20) {
            frag_queue_destory(fqueue);
            key_tree_lock(keytree);
            knode = key_rberase(keytree,knode);
            key_tree_unlock(keytree);
        } else {
            knode = key_next(knode);
        }
    }

    return 0;
}

int ipv4_fragment_maintain(struct set_tree *settree)
{
    //垃圾回收
    ipv4_fragment_trash_maintain(&inet_frag_trash);

    struct set_node *snode = set_first(settree);
    while (snode && snode->data)
    {
        struct frag_queue_t *fqueue = (struct frag_queue_t *)snode->data;

        if (IPFRAG_DO_REASSEMBLY(fqueue->flags)) {
            //reassembly
            struct rte_mbuf_t *mbuf_resm = ipv4_handle_reassembly(fqueue);
            if (mbuf_resm) {
                ipv4_handle_fragment(mbuf_resm,mtu);
                rte_mbuf_free(mbuf_resm);
            }

            fqueue->flags |= IPFRAG_COMPLETE;
        }

        if (time(0) - fqueue->firststamp > IPFRAG_INSPIRE_INTERVAL)
            fqueue->flags |= IPFRAG_EXPIRE;

        if (IPFRAG_DO_ERASE(fqueue->flags)) {
            ipv4_fragment_recycle(&inet_frag_trash,fqueue);
            set_tree_lock(settree);
            snode = set_rberase(settree,snode);
            set_tree_unlock(settree);
        } else {
            snode = set_next(snode);
        }
    }

    return 0;
}
