#ifndef _IPV4FRAGMENT_H
#define _IPV4FRAGMENT_H

#include <netinet/ether.h>
#include <netinet/ip.h>

extern int ipv4_is_dont_fragment(const struct iphdr *ip);
extern int ipv4_is_more_fragment(const struct iphdr *ip);
extern int ipv4_is_fragment(const struct iphdr *ip);
extern int ipv4_fragment_offset(const struct iphdr *ip);
extern int ipv4_is_first_fragment(const struct iphdr *ip);
extern int ipv4_is_last_fragment(const struct iphdr *ip);

#endif // _IPV4FRAGMENT_H
