#include "handler.h"

struct rte_mbuf_t *ipv4_handle_reassembly(struct frag_queue_t *fqueue)
{
    if (fqueue->meat != fqueue->len)
        return NULL;
    if (fqueue->len > 65535)
        return NULL;

    struct rte_mbuf_t *mbuf_resm = rte_mbuf_malloc(fqueue->len);
    mbuf_resm->pl3 = &mbuf_resm->buffer[mbuf_resm->data_off];
    mbuf_resm->payloadlen = fqueue->len;
    mbuf_resm->flags.direction = fqueue->first_fragment->flags.direction;

    mbuf_resm->data_off -= sizeof (struct ether_header);
    mbuf_resm->data_len += sizeof (struct ether_header);
    mbuf_resm->pl2 = &mbuf_resm->buffer[mbuf_resm->data_off];
    memcpy(mbuf_resm->pl2,fqueue->first_fragment->pl2,sizeof(struct ether_header));

    struct iphdr *ip0 = (struct iphdr *)fqueue->first_fragment->pl3;//first fragment
    struct iphdr *ip = (struct iphdr *)mbuf_resm->pl3;
    ip->version = ip0->version;
    ip->ihl = ip0->ihl;
    ip->tos = ip0->tos;
    ip->tot_len = htons(mbuf_resm->payloadlen + ip->ihl*4);
    ip->id = ip0->id;
    ip->frag_off = 0;
    ip->ttl = ip0->ttl;
    ip->protocol = ip0->protocol;
    ip->check=0;
    ip->saddr = ip0->saddr;
    ip->daddr = ip0->daddr;
    mbuf_resm->l3len = ntohs(ip->tot_len);

    unsigned char *payload = mbuf_resm->pl3 + ip->ihl*4;
    unsigned int meat = 0;
    struct key_node *knode = key_first(&fqueue->fragments);
    while (knode && knode->data)
    {
        struct rte_mbuf_t *mbuf1 = (struct rte_mbuf_t *)knode->data;
        struct iphdr *ip1 = (struct iphdr *)mbuf1->pl3;
        memcpy(&payload[mbuf1->fragoffset*8], mbuf1->pl3+ip1->ihl*4, mbuf1->payloadlen);
        meat += mbuf1->payloadlen;

        knode = key_next(knode);
    }

    if (meat != mbuf_resm->payloadlen)
        fprintf(stdout,"reassembly maybe have error.\r\n");

    mbuf_resm->data_len += mbuf_resm->l3len;
    return mbuf_resm;
}
