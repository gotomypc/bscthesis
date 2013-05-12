/*

    libnatpeer.c

    The actual library itself.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <linux/tcp.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <jansson.h>
#include <pthread.h>

#include "natpeer.h"
#include "libnatpeer.h"

static void
np_error(const char *msg)
{
  perror(msg);
  exit(1);
}

static void
np_error_exit(const char *msg)
{
  fprintf(stderr, "%s", msg);
  exit(1);
}

static void
np_setup_ip_hdr(struct iphdr *iph)
{
  DEBUG("setting up IP header\n");
  iph->ihl      = 5;                            /* 8/2 */
  iph->version  = 4;                            /* 8/2 */
  iph->tos      = 0;                            /* 8 */
  iph->tot_len  = htons(sizeof(struct iphdr)  +
                        sizeof(struct tcphdr)); /* 16 */
  iph->id       = htons(54321);                 /* 16 */
  iph->frag_off = htons(0);                     /* 16 */
  iph->ttl      = NP_MAX_TTL;                   /* 8  */
  iph->protocol = IPPROTO_TCP;                  /* 8  */
  iph->check    = htons(0);                     /* 16 */
  iph->saddr    = htonl(0);                     /* 32 */
  iph->daddr    = htonl(0);                     /* 32 */
}

static uint16_t
np_ip_chksum(uint16_t *buf, int n)
{
  DEBUG("calculating IP header checksum\n");
  uint32_t chksum = 0;
  while (n > 1) {
    chksum += *buf++;
    n -= sizeof(uint16_t);
  }
  if (n > 0) {
    chksum += *(uint8_t *) buf;
  }
  chksum  = (chksum >> 16) + (chksum & 0xffff);
  chksum += (chksum >> 16);
  return (uint16_t) (~chksum);
}

static void
np_setup_tcp_hdr(struct tcphdr *tcph)
{
  DEBUG("setting up TCP header\n");
  tcph->source  = htons(0);    /* 16 */
  tcph->dest    = htons(0);    /* 16 */
  tcph->seq     = htonl(0);    /* 32 */
  tcph->ack_seq = htonl(0);    /* 32 */
  /* 16 */
  tcph->res1    = 0;           /* 4  */
  tcph->doff    = 5;           /* 4  */
  /* flags */
  tcph->fin     = 0;           /* 1  */
  tcph->syn     = 0;           /* 1  */
  tcph->rst     = 0;           /* 1  */
  tcph->psh     = 0;           /* 1  */
  tcph->ack     = 0;           /* 1  */
  tcph->urg     = 0;           /* 1  */
  tcph->ece     = 0;           /* 1  */
  tcph->cwr     = 0;           /* 1  */
  /* end of flags */
  tcph->window  = htons(4096); /* 16 */
  tcph->check   = htons(0);    /* 16 */
  tcph->urg_ptr = htons(0);    /* 16 */
}

static uint16_t
np_tcp_chksum(struct iphdr *iph, struct tcphdr *tcph)
{
  tcp_pseudo_t ptcph;
  uint16_t tot_len = ntohs(iph->tot_len);
  DEBUG("tot_len %i\n", tot_len);
  int tcpopt_len = tcph->doff * 4 - 20;
  int tcpdatalen = tot_len - (tcph->doff*4) - (iph->ihl*4);

  ptcph.src_addr = iph->saddr;
  ptcph.dst_addr = iph->daddr;
  ptcph.zero = 0;
  ptcph.protocol = IPPROTO_TCP;
  ptcph.len = htons(sizeof(struct tcphdr) + tcpopt_len + tcpdatalen);
  int totaltcplen = sizeof(tcp_pseudo_t) + sizeof(struct tcphdr) +
    tcpopt_len + tcpdatalen;

  unsigned short *tcp = malloc(sizeof(unsigned short) * totaltcplen);
  memcpy((uint8_t *) tcp, &ptcph, sizeof(tcp_pseudo_t));

  memcpy((uint8_t *) tcp + sizeof(tcp_pseudo_t),
         (uint8_t *) tcph,
         sizeof(struct tcphdr));

  memcpy((uint8_t *) tcp + sizeof(tcp_pseudo_t) + sizeof(struct tcphdr),
         (uint8_t *) iph+(iph->ihl*4)+sizeof(struct tcphdr),
         tcpopt_len);

  memcpy((uint8_t *) tcp + sizeof(tcp_pseudo_t) + sizeof(struct tcphdr) +
         tcpopt_len, (uint8_t *) tcph+(tcph->doff*4), tcpdatalen);

  DEBUG("TCP data length: %i\n", tcpdatalen);
  uint16_t sum = np_ip_chksum((uint16_t *) tcp, totaltcplen);
  free(tcp);
  return sum;
}

static void
np_set_sock_ttl(int sock, const uint8_t ttl)
{
  int r;
  r = setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
  if (r < 0) {
    np_error("unable to set IP_TTL\n");
  }
  else {
    DEBUG("sock TTL: %u\n", ttl);
  }
}

static int
np_raw_sock_listener(np_opts_t *np_opts)
{
  ssize_t size;
  socklen_t addr_size;
  struct sockaddr addr;
  uint32_t daddr;
  int raw_sock;

  if (np_opts->dst_ip == NP_NONE)
    np_error_exit("please specify destination addr\n");

  daddr = inet_addr(np_opts->dst_ip);
  uint8_t *buffer = (uint8_t *) malloc(65536);
  raw_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (raw_sock < 0)
    np_error("unable to open raw socket");

  addr_size = sizeof(addr);

  DEBUG("capturing outgoing TCP SYN\n");
  while (1) {
    size = recvfrom(raw_sock, buffer, 65536, 0, &addr, &addr_size);
    if (size < 0) {
      DEBUG("unable to receive data\n");
      continue;
    }
    struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));
    if (iph->protocol != 6 || iph->daddr != daddr) {
      continue;
    }
    uint16_t iphlen;
    iphlen = iph->ihl * 4;
    struct tcphdr *tcph = (struct tcphdr *) (buffer + sizeof(struct ethhdr)
                                                    + iphlen);

    tcphdr_t *tcp = (tcphdr_t *) tcph;
    if ((uint8_t) tcph->syn == 1) {
      DEBUG("TCP SYN captured\n");
      DEBUG("TCP seq: %u\n", (uint32_t) ntohl(tcph->seq));
      DEBUG("TCP ts_val: %u\n", (uint32_t) ntohl(tcp->opts.ts_val));
      DEBUG("forwarding TCP handshake data\n");
      np_connection_send_info(NULL, ntohl(tcph->seq), ntohl(tcp->opts.ts_val));
      break;
    }
  }

  DEBUG("capturing incoming SYN-ACK...\n");
  while (1) {
    size = recvfrom(raw_sock, buffer, 65536, 0, &addr, &addr_size);
    if (size < 0) {
      DEBUG("unable to receive data\n");
      continue;
    }
    struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));
    if (iph->protocol != 6 || iph->saddr != daddr) {
      continue;
    }
    uint16_t iphlen;
    iphlen = iph->ihl * 4;
    struct tcphdr *tcph = (struct tcphdr *) (buffer + sizeof(struct ethhdr)
                                             + iphlen);
    if (tcph->ack == 1 && tcph->syn == 1) {
      DEBUG("...TCP SYN-ACK captured\n");
      break;
    }
  }

  DEBUG("capturing outgoing TCP ACK...\n");
  while (1) {
    size = recvfrom(raw_sock, buffer, 65536, 0, &addr, &addr_size);
    if (size < 0) {
      DEBUG("unable to receive data\n");
      continue;
    }
    struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));
    if (iph->protocol != 6 || iph->daddr != daddr) {
      continue;
    }
    struct tcphdr *tcph = (struct tcphdr *) (buffer + sizeof(struct ethhdr)
                                             + sizeof(struct iphdr));
    if ((uint8_t) tcph->ack == 1 && (uint8_t) tcph->syn == 0) {
      DEBUG("...TCP ACK captured\n");
      break;
    }
  }
  close(raw_sock);
  DEBUG("resending TCP ACK\n");
  np_increase_ttl_resend(np_opts, buffer, size);
  free(buffer);
  return 0;
}

static void *
np_raw_sock_listener_thread(void *args)
{
  np_opts_t *opts = (np_opts_t *) args;
  np_raw_sock_listener(opts);
  DEBUG("RAW SOCK listener thread done\n");
  return NULL;
}

static int
np_connection_local_forward(int remote_sock)
{
  DEBUG("forwarding local socket\n");
  int serv_sock = 0, cli_sock = 0, maxfd;
  socklen_t socklen;
  struct sockaddr_in serv_addr, client_addr;

  fd_set sockets;
  char buf[NP_BUF_SIZE];
  ssize_t n;

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(NP_LOCAL_PORT);
  serv_sock = socket(AF_INET, SOCK_STREAM, 0);

  int opt = 1;
  if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    np_error("unable to set SO_REUSEADDR");

  if (bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    np_error_exit("unable to bind\n");

  printf("[INFO] connect to 127.0.0.1:%d to access requested service\n",
    NP_LOCAL_PORT);
  listen(serv_sock, 5);

  socklen = sizeof(client_addr);
  cli_sock = accept(serv_sock, (struct sockaddr *) &client_addr, &socklen);
  if (cli_sock < 0) {
    np_error_exit("unable to accept connection\n");
    goto end;
  }
  DEBUG("local client connected\n");

  maxfd = (remote_sock > cli_sock ? remote_sock : cli_sock) + 1;
  while (1) {
    FD_ZERO(&sockets);
    FD_SET(remote_sock, &sockets);
    FD_SET(cli_sock, &sockets);

    if (select(maxfd, &sockets, NULL, NULL, NULL) < 0)
      goto end;

    if (FD_ISSET(cli_sock, &sockets)) {
      n = recv(cli_sock, buf, NP_BUF_SIZE, 0);
      if (n <= 0)
        goto end;
      else
        send(remote_sock, buf, n, 0);

    } else if (FD_ISSET(remote_sock, &sockets)) {
      n = recv(remote_sock, buf, NP_BUF_SIZE, 0);
      if (n <= 0)
        goto end;
      else
        send(cli_sock, buf, n, 0);
    }
  }
end:
  close(serv_sock);
  close(cli_sock);
  return 0;
}

static void
np_sock_connect(np_opts_t *np_opts)
{
  int r, sock;
  struct sockaddr_in sa_loc, sa_dst;
  struct hostent *host;

  if (np_opts->src_ip  == NP_NONE || np_opts->dst_ip  == NP_NONE ||
      np_opts->src_prt == NP_NONE || np_opts->dst_prt == NP_NONE  ) {
    np_error_exit("please specify src ip, src port, dst ip, dst port\n");
  }

  if (np_opts->ttl == NP_MAX_TTL)
    printf("[WARN] connecting with MAX_TTL\n");

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == 0)
    np_error("np_error opening socket");

  int opt = 1;
  r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (r < 0)
    np_error("unable to set SO_REUSEADDR");

  np_set_sock_ttl(sock, 5);
  struct timeval tv;
  tv.tv_sec = 1000;
  tv.tv_usec = 0;
  r = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &tv, sizeof(tv));
  if (r < 0)
    np_error("unable to set SO_SNDTIMEO");

  DEBUG("remote host: %s\n", np_opts->dst_ip);
  if (np_opts->src_prt != NP_NONE) {
    DEBUG("local port %i\n", np_opts->src_prt);
    memset(&sa_loc, 0, sizeof(sa_loc));
    sa_loc.sin_family = AF_INET;
    sa_loc.sin_port = htons(np_opts->src_prt);
    sa_loc.sin_addr.s_addr = INADDR_ANY;
    r = bind(sock, (struct sockaddr *) &sa_loc, sizeof(sa_loc));
    if (r < 0) {
      DEBUG("unable to bind to local port (%i)\n", np_opts->src_prt);
      np_error("unable to bind to local port");
    }
  }

  host = gethostbyname(np_opts->dst_ip);
  if (host == NULL)
    np_error_exit("no such host\n");

  memset(&sa_dst, 0, sizeof(sa_dst));
  sa_dst.sin_family = AF_INET;

  sa_dst.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
  DEBUG("remote port: %i\n",
      np_opts->dst_ip != NP_NONE ? np_opts->dst_prt : NP_SERV_PORT);
  sa_dst.sin_port = htons(
      np_opts->dst_ip != NP_NONE ? np_opts->dst_prt : NP_SERV_PORT);

  r = connect(sock, (struct sockaddr *) &sa_dst, sizeof(sa_dst));
  if (r < 0)
    np_error_exit("unable to connect");
  else
    printf("[INFO] connection established!\n");

  np_set_sock_ttl(sock, NP_MAX_TTL);
  np_connection_local_forward(sock);
  close(sock);
}


static void *
np_sock_connect_thread(void *args)
{
  np_opts_t *opts = (np_opts_t *) args;
  np_sock_connect(opts);
  return NULL;
}

static int
np_packet_inject(np_opts_t *np_opts, uint8_t *packet, uint32_t len)
{
  if (np_opts->net_if == NULL)
    np_error_exit("interface not specified\n");

  DEBUG("injecting packet to TCP/IP stack\n");
  int sock, r;
  sock = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
  if (sock < 0)
    np_error("unable to open socket");

  struct ifreq if_idx;
  memset(&if_idx, 0, sizeof(struct ifreq));
  strncpy(if_idx.ifr_name, np_opts->net_if, IFNAMSIZ-1);
  if (ioctl(sock, SIOCGIFINDEX, &if_idx) < 0)
    np_error("unable to get SIOCGIFINDEX");

  struct ifreq if_mac;
  memset(&if_mac, 0, sizeof(struct ifreq));
  strncpy(if_mac.ifr_name, np_opts->net_if, IFNAMSIZ-1);
  if (ioctl(sock, SIOCGIFHWADDR, &if_mac) < 0)
    np_error("unable to get SIOCGIFHWADDR");

  struct ifreq if_ip;
  memset(&if_ip, 0, sizeof(struct ifreq));
  strncpy(if_ip.ifr_name, np_opts->net_if, IFNAMSIZ-1);
  if (ioctl(sock, SIOCGIFADDR, &if_ip) < 0)
    np_error("unable to get SIOCGIFADDR");

  int pack_size = 0;
  uint8_t ether_packet[NP_BUF_SIZE];
  memset(ether_packet, 0, NP_BUF_SIZE);
  struct ether_header *eth = (struct ether_header *) ether_packet;

  eth->ether_shost[0] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[0];
  eth->ether_shost[1] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[1];
  eth->ether_shost[2] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[2];
  eth->ether_shost[3] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[3];
  eth->ether_shost[4] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[4];
  eth->ether_shost[5] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[5];

  eth->ether_dhost[0] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[0];
  eth->ether_dhost[1] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[1];
  eth->ether_dhost[2] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[2];
  eth->ether_dhost[3] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[3];
  eth->ether_dhost[4] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[4];
  eth->ether_dhost[5] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[5];

  eth->ether_type = htons(ETH_P_IP);
  pack_size += sizeof(struct ether_header);
  DEBUG("ether packet len: %d\n", pack_size);

  memcpy(ether_packet + pack_size, packet, len);
  pack_size += len;
  DEBUG("total packet len: %d\n", pack_size);

  struct sockaddr_ll sa;
  sa.sll_ifindex = if_idx.ifr_ifindex;
  sa.sll_halen = ETH_ALEN;
  sa.sll_addr[0] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[0];
  sa.sll_addr[1] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[1];
  sa.sll_addr[2] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[2];
  sa.sll_addr[3] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[3];
  sa.sll_addr[4] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[4];
  sa.sll_addr[5] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[5];

  r = sendto(sock, ether_packet, pack_size, 0, (struct sockaddr *) &sa,
             sizeof(struct sockaddr_ll));
  if (r < 0) {
    np_error("unable to send");
  }
  else {
    DEBUG("packet injected\n");
  }
  return 0;
}

static void *
np_packet_create(np_opts_t *np_opts)
{
  uint8_t *packet = malloc(NP_BUF_SIZE);
  if (packet == NULL)
    np_error_exit("unable to malloc memory");

  struct iphdr  *iph  = (struct iphdr *) packet;
  struct tcphdr *tcph = (struct tcphdr *) ((uint8_t *) iph +
                                                       (5 * sizeof(uint32_t)));

  memset(packet, 0, NP_BUF_SIZE);
  np_setup_ip_hdr(iph);
  np_setup_tcp_hdr(tcph);

  tcphdr_t *tcp = (tcphdr_t *) tcph;
  iph->tot_len = htons(sizeof(struct iphdr) + sizeof(tcphdr_t) +
                       np_opts->payload);

  if (np_opts->src_ip != NP_NONE)
    iph->saddr = inet_addr(np_opts->src_ip);

  if (np_opts->dst_ip != NP_NONE)
    iph->daddr = inet_addr(np_opts->dst_ip);

  if (np_opts->ttl != NP_NONE)
    iph->ttl = np_opts->ttl;

  iph->check = np_ip_chksum((uint16_t *) packet, ntohs(iph->tot_len) >> 1);

  if (np_opts->src_prt != NP_NONE)
    tcph->source = htons(np_opts->src_prt);

  if (np_opts->dst_prt != NP_NONE)
    tcph->dest = htons(np_opts->dst_prt);

  if (np_opts->seq != NP_NONE)
    tcph->seq = htonl(np_opts->seq);

  if (np_opts->ack_seq != NP_NONE)
    tcph->ack_seq = htonl(np_opts->ack_seq);

  tcph->doff = sizeof(tcphdr_t) / 4;
  tcph->window = htons(14600);

  tcph->syn = NP_TCP_SYN & np_opts->tcp_flags ? 1 : 0;
  tcph->ack = NP_TCP_ACK & np_opts->tcp_flags ? 1 : 0;
  tcph->psh = NP_TCP_PSH & np_opts->tcp_flags ? 1 : 0;

  tcp->opts.mms_kind  = 2;
  tcp->opts.mms_len   = 4;
  tcp->opts.mms_mss   = htons(1460);
  tcp->opts.sack_kind = 4;
  tcp->opts.sack_len  = 2;
  tcp->opts.ts_kind   = 8;
  tcp->opts.ts_len    = 10;

  if (np_opts->ts_val != NP_NONE)
    tcp->opts.ts_val = htonl(np_opts->ts_val);
  else
    tcp->opts.ts_val = htonl(2810600);

  if (np_opts->ts_echo != NP_NONE)
    tcp->opts.ts_echo = htonl(np_opts->ts_echo);
  else
    tcp->opts.ts_echo = htonl(0);

  tcp->opts.nop_nop         = 1;
  tcp->opts.win_scale_kind  = 3;
  tcp->opts.win_scale_len   = 3;
  tcp->opts.win_scale_count = 8;

  tcph->check = np_tcp_chksum(iph, tcph);

  DEBUG("packet len: %d\n", ntohs(iph->tot_len));
  return (void *) packet;
}

static int
np_packet_send(np_opts_t *np_opts, uint8_t *packet, uint32_t len)
{
  int sock, r;
  struct sockaddr_in sa;
  sock = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
  if (sock < 0)
    np_error("unable to open RAW_SOCK");
  sa.sin_family = AF_INET;
  sa.sin_port = htons(np_opts->dst_prt);
  sa.sin_addr.s_addr = inet_addr(np_opts->dst_ip);

  int opt = 1;
  r = setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt));
  if (r < 0)
    np_error("unable to set IP_HDRINCL");

  r = sendto(sock, packet, len, 0, (struct sockaddr *) &sa, sizeof(sa));
  if (r < 0) {
    np_error("unable to send packet");
  } else {
    DEBUG("packet sent\n");
  }
  close(sock);
  return 0;
}

static int
np_increase_ttl_resend(np_opts_t *np_opts, uint8_t *packet, uint32_t size)
{
  struct iphdr *iph;
  iph        = (struct iphdr *) (packet + sizeof(struct ethhdr));
  iph->ttl   = NP_MAX_TTL;
  iph->check = htons(0);
  iph->check = np_ip_chksum((uint16_t *) iph, ntohs(iph->tot_len) >> 1);
  DEBUG("captured packet length: %u\n", ntohs(iph->tot_len));
  np_packet_send(np_opts, (uint8_t *) iph, ntohs(iph->tot_len));
  return 0;
}

static int
np_connection_send_syn(np_opts_t *np_opts)
{
  DEBUG("sending out TCP SYN with low TTL\n");
  np_opts_t opts;
  memset(&opts, 0, sizeof(opts));
  opts.src_ip     = np_opts->dst_ip;
  opts.src_prt    = np_opts->dst_prt;
  opts.dst_ip     = np_opts->src_ip;
  opts.dst_prt    = np_opts->src_prt;
  opts.ttl        = 5;
  opts.tcp_flags |= NP_TCP_SYN;
  uint8_t *packet = (uint8_t *) np_packet_create(&opts);
  struct iphdr *iph = (struct iphdr *) packet;
  np_packet_send(&opts, packet, ntohs(iph->tot_len));
  free(packet);
  return 0;
}

static int
np_connection_send_info(np_opts_t *opts, uint32_t isn, uint32_t ts_val)
{
  int n;
  char *json_str;
  json_t *json, *json_isn, *json_ts_val, *json_event, *json_id;
  json        = json_object();
  json_isn    = json_integer(isn);
  json_ts_val = json_integer(ts_val);
  json_event  = json_string("connection_info");
  json_id     = json_string(np_connection_id);
  DEBUG("ID %s\n", np_connection_id);

  json_object_set(json, "event",  json_event);
  json_object_set(json, "isn",    json_isn);
  json_object_set(json, "ts_val", json_ts_val);
  json_object_set(json, "id",     json_id);
  json_str = json_dumps(json, JSON_INDENT(2) | JSON_ENCODE_ANY);
  n = send(np_serv_sock, json_str, strlen(json_str), 0);
  if (n < 0)
    np_error("unable to write\n");

  free(json_str);
  close(np_serv_sock);
  return 0;
}

static int
np_connection_send_fake(np_opts_t *np_opts)
{
  DEBUG("sending out fake packet with payload\n");
  char *payload = "HELLO";
  np_opts->tcp_flags |= NP_TCP_PSH;
  np_opts->tcp_flags |= NP_TCP_ACK;
  np_opts->seq        = 123123;
  np_opts->ack_seq    = 321321;
  np_opts->ttl        = 5;
  uint8_t *packet     = np_packet_create(np_opts);
  struct iphdr *iph   = (struct iphdr *) packet;
  DEBUG("total length of IP packet: %u\n", ntohs(iph->tot_len));
  memcpy(packet + ntohs(iph->tot_len), payload, strlen(payload));
  np_packet_send(np_opts, packet, ntohs(iph->tot_len));
  free(packet);
  return 0;
}

int
np_init(void)
{
  np_s_opts = malloc(sizeof(np_opts_t));
  if (np_s_opts == NULL) {
    return 1;
  } else {
    memset(np_s_opts, 0, sizeof(np_opts_t));
    return 0;
  }
}

static int
np_connection_request(np_opts_t *np_opts)
{
  if (np_opts->server == NP_NONE || np_opts->service == NULL)
    np_error_exit("please specify remote server and service\n");

  int n, r;
  struct sockaddr_in sa_loc, sa_dst;
  struct hostent *host;
  char buf[NP_BUF_SIZE];

  np_serv_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (np_serv_sock == 0)
    np_error("error opening socket");

  int opt = 1;
  r = setsockopt(np_serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (r < 0)
    np_error("unable to set SO_REUSEADDR");

  DEBUG("remote server: %s\n", np_opts->server);

  if (np_opts->src_prt != NP_NONE) {
    DEBUG("local port %i\n", np_opts->src_prt);
    memset(&sa_loc, 0, sizeof(sa_loc));
    sa_loc.sin_family = AF_INET;
    sa_loc.sin_port = htons(np_opts->src_prt);
    sa_loc.sin_addr.s_addr = INADDR_ANY;
    r = bind(np_serv_sock, (struct sockaddr *) &sa_loc, sizeof(sa_loc));
    if (r < 0) {
      DEBUG("unable to bind to local port (%i)\n", np_opts->src_prt);
      np_error("unable to bind to local port");
    }
  }

  host = gethostbyname(np_opts->server);
  if (host == NULL)
    np_error("no such host");

  memset(&sa_dst, 0, sizeof(sa_dst));
  sa_dst.sin_family = AF_INET;

  sa_dst.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
  DEBUG("remote port: %i\n",
      np_opts->dst_prt != NP_NONE ? np_opts->dst_prt : NP_SERV_PORT);
  sa_dst.sin_port = htons(
      np_opts->dst_prt != NP_NONE ? np_opts->dst_prt : NP_SERV_PORT);

  r = connect(np_serv_sock, (struct sockaddr *) &sa_dst, sizeof(sa_dst));
  if (r < 0)
    np_error("unable to connect");

  struct sockaddr_in local_addr;
  socklen_t local_addr_len = sizeof(local_addr);
  memset(&local_addr, 0, local_addr_len);
  getsockname(np_serv_sock, (struct sockaddr *) &local_addr, &local_addr_len);
  int local_port = (int) ntohs(local_addr.sin_port);

  char local_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(local_addr.sin_addr), local_ip, INET_ADDRSTRLEN);

  char *json_str;
  json_t *json, *json_event, *json_service, *json_nat;
  json         = json_object();
  json_event   = json_string("request");
  json_service = json_string(np_opts->service);

  json_object_set(json, "event", json_event);
  json_object_set(json, "service", json_service);

  if (np_opts->nat == NP_TRUE) {
    DEBUG("peer is behind NAT\n");
    json_nat = json_true();
    json_object_set(json, "nat", json_nat);
  }

  json_str = json_dumps(json, JSON_INDENT(2) | JSON_ENCODE_ANY);
  DEBUG("%s\n", json_str);

  n = write(np_serv_sock, json_str, strlen(json_str));
  if (n < 0)
    np_error("unable to write");
  free(json_str);

  memset(buf, 0, NP_BUF_SIZE);
  n = recv(np_serv_sock, buf, NP_BUF_SIZE, 0);
  if (n < 0)
    np_error("unable to read");
  DEBUG("response: %s\n", buf);

  json_t *root;
  json_error_t json_err;
  root = json_loads(buf, 0, &json_err);
  if (!root)
    np_error("unable to parse response");

  DEBUG("root size: %i\n", (int) json_array_size(root));
  json = json_array_get(root, 0);

  int port, peer_port;
  json_t *json_ip, *json_port, *json_peer_ip, *json_peer_port, *json_id;
  json_ip        = json_object_get(json, NP_IP);
  json_port      = json_object_get(json, NP_PORT);
  json_peer_ip   = json_object_get(json, NP_PEER_IP);
  json_peer_port = json_object_get(json, NP_PEER_PORT);
  json_id        = json_object_get(json, NP_ID);

  const char *ip = json_string_value(json_ip);
  const char *peer_ip = json_string_value(json_peer_ip);
  port = json_integer_value(json_port);
  peer_port = json_integer_value(json_peer_port);

  np_connection_id = json_string_value(json_id);

  char *pip = malloc(strlen(peer_ip)+1);
  if (pip == NULL)
    np_error_exit("unable to allocate memory\n");
  strncpy(pip, peer_ip, strlen(peer_ip)+1);

  np_s_opts->src_ip  = local_ip;
  np_s_opts->src_prt = local_port;
  np_s_opts->dst_ip  = pip;
  np_s_opts->dst_prt = peer_port;
  DEBUG("pub  addr: %s:%i\n", ip, port);
  DEBUG("loc  addr: %s:%i\n", np_s_opts->src_ip, np_s_opts->src_prt);
  DEBUG("peer addr: %s:%i\n", np_s_opts->dst_ip, np_s_opts->dst_prt);
  return 0;
}

int
np_connection_response(np_opts_t *np_opts)
{
  np_connection_send_syn(np_opts);
  uint8_t *packet = (uint8_t *) np_packet_create(np_opts);
  struct iphdr *iph = (struct iphdr *) packet;
  np_packet_inject(np_opts, packet, ntohs(iph->tot_len));
  free(packet);
  np_opts_t opts;
  memset(&opts, 0, sizeof(np_opts_t));
  opts.src_ip  = np_opts->dst_ip;
  opts.src_prt = np_opts->dst_prt;
  opts.dst_ip  = np_opts->src_ip;
  opts.dst_prt = np_opts->src_prt;
  opts.payload = 5;
  np_connection_send_fake(&opts);
  return 0;
}

int
np_connection_establish(np_opts_t *np_opts)
{
  pthread_t raw_sock_thread, sock_thread;
  np_connection_request(np_opts);
  pthread_create(&raw_sock_thread, NULL,
                 &np_raw_sock_listener_thread, (void *) np_s_opts);
  pthread_create(&sock_thread, NULL,
                 &np_sock_connect_thread, (void *) np_s_opts);
  pthread_join(raw_sock_thread, NULL);
  pthread_join(sock_thread, NULL);
  return 0;
}

void
np_test(np_opts_t *np_opts)
{
}
