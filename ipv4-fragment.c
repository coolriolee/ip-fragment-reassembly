#include "handler.h"

int ipv4_is_dont_fragment(const struct iphdr *ip)
{
    return (ntohs(ip->frag_off) & 0x4000);
}

int ipv4_is_more_fragment(const struct iphdr *ip)
{
    return (ntohs(ip->frag_off) & 0x2000);
}

int ipv4_is_fragment(const struct iphdr *ip)
{
    return (ntohs(ip->frag_off) & 0x3FFF);
}

int ipv4_is_first_fragment(const struct iphdr *ip)
{
    return ipv4_fragment_offset(ip) ? 0:1;
}

int ipv4_is_last_fragment(const struct iphdr *ip)
{
    return ipv4_is_more_fragment(ip) ? 0:1;
}

int ipv4_fragment_offset(const struct iphdr *ip)
{
    return (ntohs(ip->frag_off) & 0x1FFF);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
extern void pcap_handle_write(const unsigned char *buffer,const int pktlen,int offset);
int ipv4_handle_fragment(struct rte_mbuf_t *mbuf,const unsigned short mtu)
{
//    unsigned short fragment_step = FRAGMENT_STEP(mtu);//分片偏移步长
    unsigned int meat=0,index=1;
    unsigned short fragment_size = FRAGMENT_SIZE(mtu);//
    unsigned short fragment_count = (mbuf->payloadlen / fragment_size) + 1;

    const struct iphdr *ip = (const struct iphdr *)mbuf->pl3;
    const unsigned char *payload = (const unsigned char *)(mbuf->pl3 + ip->ihl*4);

    while (index <= fragment_count)
    {
        unsigned short offset = 0;
        unsigned short payloadlen=0,fragoffset=0;
        struct rte_mbuf_t *mbuf0 = rte_mbuf_malloc(RTEMBUF_BUFFERSIZE);
        mbuf0->flags.direction = mbuf->flags.direction;
        mbuf0->pl3 = &mbuf0->buffer[mbuf0->data_off];
        struct iphdr *ip0 = (struct iphdr *)mbuf0->pl3;

        memcpy(ip0,ip,sizeof(struct iphdr));
        ip0->check=0;

        offset = (index - 1)*fragment_size;//负载偏移
        fragoffset = offset/8;//分片偏移

        if (index == fragment_count) {
            payloadlen = mbuf->payloadlen - offset;
            ip0->frag_off = htons(fragoffset);
        } else {
            mbuf0->flags.is_fragment=1;
            payloadlen = fragment_size;
            ip0->frag_off = htons(IP_MF | fragoffset);
        }
        ip0->tot_len = htons(payloadlen+ip0->ihl*4);
        memcpy(mbuf0->pl3+ip0->ihl*4, &payload[offset], payloadlen);//copy payload

        mbuf0->fragoffset = fragoffset;
        mbuf0->payloadlen = payloadlen;
        mbuf0->l3len = payloadlen+ip0->ihl*4;
        mbuf0->data_len += mbuf0->l3len;

        mbuf0->data_off -= sizeof (struct ether_header);
        mbuf0->data_len += sizeof (struct ether_header);
        mbuf0->pl2 = &mbuf0->buffer[mbuf0->data_off];
        memcpy(mbuf0->pl2,mbuf->pl2,sizeof(struct ether_header));

        meat+=mbuf0->payloadlen;
        pcap_handle_write(mbuf0->pl2,mbuf0->data_len,0);
        rte_mbuf_free(mbuf0);

        ++index;//next
    }

    if (meat != mbuf->payloadlen)
        fprintf(stdout,"fragment maybe have error.\r\n");
    return 0;
}
