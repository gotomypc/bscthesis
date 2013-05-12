/*

    natpeer.h

    Contains public methods of the library.

*/

#ifndef _NATPEER_H_
#define _NATPEER_H_

#include <stdint.h>

#define NP_TRUE       1
#define NP_FALSE      0
#define NP_SERV_PORT  8001
#define NP_LOCAL_PORT 8002
#define NP_BUF_SIZE   1024
#define NP_MAX_TTL    64
#define NP_TCP_FIN    0x01
#define NP_TCP_SYN    0x02
#define NP_TCP_RST    0x04
#define NP_TCP_PSH    0x08
#define NP_TCP_ACK    0x10
#define NP_TCP_URG    0x20
#define NP_TCP_ECE    0x40
#define NP_TCP_CWR    0x80
#define NP_ID         "id"
#define NP_IP         "ip"
#define NP_PORT       "port"
#define NP_PEER_IP    "peer_ip"
#define NP_PEER_PORT  "peer_port"

/* toggle debugging on/off */
#define DEBUG_LEVEL 1
#define DEBUG(...) { \
  if (DEBUG_LEVEL > 0) {  \
    fprintf(stdout, "[DEBUG] %s:%u %s: ", __FILE__, __LINE__, __func__); \
    fprintf(stdout, __VA_ARGS__); \
  } \
}

/*

    Enum that specifies which method to call from the library.

*/
typedef enum
{
  NP_NONE,
  NP_TEST,
  NP_CON_ESTAB,
  NP_CON_RESP,
} np_action_t;

/*

    Struct if different parameters which are for connection establishment.

*/
typedef struct
{
  np_action_t action;
  uint8_t     ttl;       /* TTL of sockets */
  char       *src_ip;    /* source IP address */
  char       *dst_ip;    /* destination IP address */
  uint16_t    src_prt;   /* source port */
  uint16_t    dst_prt;   /* destination port */
  uint32_t    seq;       /* TCP packet sequence number */
  char       *net_if;    /* network interface, e.g. 'eth0', 'wlan0' */
  char       *server;    /* rendezvous server IP address */
  uint32_t    ts_val;    /* TCP packet timestamp value */
  uint32_t    ts_echo;   /* TCP packet timestamp reply value */
  uint8_t     tcp_flags; /* TCP packet flags */
  uint32_t    ack_seq;   /* TCP packet acknowledgement number */
  uint32_t    payload;   /* TCP packet payload size */
  uint8_t     nat;       /* whether given peer is behind NAT or not */
  char       *service;   /* service to be accessed */
} np_opts_t;

int  np_init                (void);
int  np_connection_establish(np_opts_t *);
int  np_connection_response (np_opts_t *);
void np_test                (np_opts_t *);

#endif /* _NATPEER_H_ */
