#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <pcap.h>

#include "handler.h"

static char *filename=NULL;
pcap_t *handle;

void print_usage()
{
    printf("ipv4 fragment reaassembly:\r\n");
    printf("\toptions:\r\n");
    printf("\t\t-f: filename[pcap or pcapng].\r\n");
    printf("\t\t-h: helper.\r\n");
    exit(0);
}

int parse_options(int argc,char **argv)
{
    int c;

    while (-1 != (c = getopt(argc,argv,"f:h")))
    {
        switch (c) {
        case 'f':
            filename = strdup(optarg);
            break;
        case 'h':
        default:
            print_usage();
            break;
        }
    }

    assert(filename);
    return 0;
}

void *ipv4_read_fragment_thread(__attribute__((unused)) void *p);
void *ipv4_write_fragment_thread(__attribute__((unused)) void *p);

int main(int argc, char *argv[])
{
    char errbuf[255];
    parse_options(argc,argv);

    handle = pcap_open_offline(filename,errbuf);
    if (!handle) {
        printf("%s open failed [%s].\r\n",filename,errbuf);
        return -1;
    }

    set_tree_init(&inet_frag_settree);

    pthread_t thread;
    pthread_create(&thread,NULL,ipv4_read_fragment_thread,NULL);

    pthread_create(&thread,NULL,ipv4_write_fragment_thread,NULL);

    pthread_join(thread,NULL);

    set_tree_destroy(&inet_frag_settree,frag_queue_destory);

    pcap_close(handle);
    free(filename);
    return 0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void *ipv4_read_fragment_thread(__attribute__((unused)) void *p)
{
    while (1)
    {
        struct pcap_pkthdr header;
        const unsigned char *packet = pcap_next(handle,&header);
        if (!packet) {
            usleep(100);
            continue;
        }
        struct rte_mbuf_t *mbuf0 = (struct rte_mbuf_t *)malloc(sizeof(struct rte_mbuf_t));
        int mbuf_free_flag=1;
        rte_mbuf_init(mbuf0);
        mbuf0->data_len = header.len;
        memcpy(&mbuf0->buffer[mbuf0->data_off],packet,header.len);

        mbuf0->pl2 = &mbuf0->buffer[mbuf0->data_off];
        struct ether_header *pETHHDR = (struct ether_header *)mbuf0->pl2;

        switch (ntohs(pETHHDR->ether_type)) {
        case ETHERTYPE_IP: {
            mbuf0->pl3 = &mbuf0->buffer[mbuf0->data_off + sizeof(struct ether_header)];
            if (ipv4_handler(&inet_frag_settree,mbuf0) < 0)
                mbuf_free_flag=0;//ip fragment

        } break;
        default:
            break;
        }

        if (mbuf_free_flag) rte_mbuf_free(mbuf0);
    }

    return NULL;
}

void *ipv4_write_fragment_thread(__attribute__((unused)) void *p)
{
    while (1)
    {

        usleep(100);
    }

    return NULL;
}
