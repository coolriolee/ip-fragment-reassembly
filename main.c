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
unsigned int mtu=1500;
pcap_t *handle;

void print_usage()
{
    printf("ipv4 fragment reaassembly:\r\n");
    printf("\toptions:\r\n");
    printf("\t\t-f: filename[pcap or pcapng].\r\n");
    printf("\t\t-u: mtu[default 1500].\r\n");
    printf("\t\t-h: helper.\r\n");
    exit(0);
}

int parse_options(int argc,char **argv)
{
    int c;

    while (-1 != (c = getopt(argc,argv,"f:u:h")))
    {
        switch (c) {
        case 'f':
            filename = strdup(optarg);
            break;
        case 'u':
            mtu = atoi(optarg);
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

static void pcap_handle_close();
static void pcap_handle_open(const char *filename);

void *ipv4_read_fragment_thread(__attribute__((unused)) void *p);
void *ipv4_write_fragment_thread(__attribute__((unused)) void *p);

int main(int argc, char *argv[])
{
    char errbuf[255],tmpfile[255]={0};
    parse_options(argc,argv);

    handle = pcap_open_offline(filename,errbuf);
    if (!handle) {
        printf("%s open failed [%s].\r\n",filename,errbuf);
        return -1;
    }

    snprintf(tmpfile,sizeof(tmpfile)-1,"fragment-mtu-%d-%s",mtu,filename);
    pcap_handle_open(tmpfile);

    set_tree_init(&inet_frag_settree);
    key_tree_init(&inet_frag_trash);

    pthread_t thread;
    pthread_create(&thread,NULL,ipv4_read_fragment_thread,NULL);

    pthread_create(&thread,NULL,ipv4_write_fragment_thread,NULL);

    pthread_join(thread,NULL);

    set_tree_destroy(&inet_frag_settree,frag_queue_destory);
    key_tree_destroy2(&inet_frag_trash,frag_queue_destory);

    pcap_close(handle);
    pcap_handle_close();
    free(filename);
    return 0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#include <fcntl.h>
#include <errno.h>
/* "libpcap" file header. */
struct pcap_hdr {
    unsigned int magic;          /* magic number */
    unsigned short version_major;  /* major version number */
    unsigned short version_minor;  /* minor version number */
    unsigned int thiszone;       /* GMT to local correction */
    unsigned int sigfigs;        /* accuracy of timestamps */
    unsigned int snaplen;        /* max length of captured packets, in octets */
    unsigned int network;        /* data link type */
};

/* "libpcap" record header. */
struct pcaprec_hdr {
    unsigned int ts_sec;         /* timestamp seconds */
    unsigned int ts_usec;        /* timestamp microseconds (nsecs for PCAP_NSEC_MAGIC) */
    unsigned int incl_len;       /* number of octets of packet saved in file */
    unsigned int orig_len;       /* actual length of packet */
};

static struct pcap_hdr  hdr;
static struct pcaprec_hdr rec_hdr;
static int hdr_len = 0;
static int rec_hdr_len = 0;
static int writefd=-1;

#define PCAP_MAGIC                      0xa1b2c3d4
#define PCAP_SWAPPED_MAGIC              0xd4c3b2a1
#define PCAP_NSEC_MAGIC                 0xa1b23c4d
#define PCAP_SWAPPED_NSEC_MAGIC         0x4d3cb2a1

static void pcap_handle_open(const char *filename)
{
    writefd = open(filename,O_CREAT|O_RDWR);
    if (-1 == writefd) {
        fprintf(stdout,"%s %s\n",filename,strerror(errno));
        return ;
    }

    hdr.magic = PCAP_MAGIC;
    hdr.version_major=2;
    hdr.version_minor=4;
    hdr.thiszone = 0;
    hdr.sigfigs = 0;
    hdr.snaplen = 0xff;
    hdr.network = 1;

    hdr_len = sizeof(hdr);
    rec_hdr_len = sizeof(rec_hdr);
    write(writefd,&hdr,hdr_len);
}

void pcap_handle_write(const unsigned char *buffer,const int pktlen,int offset)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    rec_hdr.ts_sec =  tv.tv_sec+28800;
    rec_hdr.ts_usec = tv.tv_usec;
    rec_hdr.incl_len = pktlen-offset;
    rec_hdr.orig_len = pktlen- offset;

    int ret = write(writefd,&rec_hdr,rec_hdr_len);
    ret = write(writefd,buffer+offset,pktlen-offset);
    if (ret < 0) {
        perror("write packet failed.");
        close(writefd);
        exit(3);
    }
}

static void pcap_handle_close()
{
    close(writefd);
    writefd=-1;
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
        int mbuf_free_flag=1;
        struct rte_mbuf_t *mbuf0 = rte_mbuf_malloc(RTEMBUF_BUFFERSIZE);
        memcpy(&mbuf0->buffer[mbuf0->data_off],packet,header.len);
        mbuf0->data_len = header.len;

        mbuf0->pl2 = &mbuf0->buffer[mbuf0->data_off];
        struct ether_header *pETHHDR = (struct ether_header *)mbuf0->pl2;

        switch (ntohs(pETHHDR->ether_type)) {
        case ETHERTYPE_IP: {
            mbuf0->pl3 = &mbuf0->buffer[mbuf0->data_off + sizeof(struct ether_header)];
            if (!ipv4_handler(&inet_frag_settree,mbuf0))
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
        ipv4_fragment_maintain(&inet_frag_settree);
        usleep(100);
    }

    return NULL;
}
