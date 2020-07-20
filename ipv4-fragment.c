#include "ipv4-fragment.h"

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

int ipv4_fragment_offset(const struct iphdr *ip)
{
    return (ntohs(ip->frag_off) & 0x1FFF);
}

int ipv4_is_first_fragment(const struct iphdr *ip)
{
    return ipv4_fragment_offset(ip) ? 0:1;
}

int ipv4_is_last_fragment(const struct iphdr *ip)
{
    return ipv4_is_more_fragment(ip) ? 0:1;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int ipv4_maintain_fragment()
{

    return 0;
}
