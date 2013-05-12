/*

    libnatpeer.h

    Declarations of private functions, structs and variables of the library.

*/

/* Private variables of the library */
static int         np_serv_sock;
static np_opts_t  *np_s_opts;
static const char *np_connection_id;

/*

    Struct of pseudo TCP header for calculating the correct checksum of the TCP
    header.

*/
typedef struct {
  uint32_t src_addr;
  uint32_t dst_addr;
  uint8_t  zero;
  uint8_t  protocol;
  uint16_t len;
} tcp_pseudo_t;

/*

    Struct for different options in TCP header.

*/
typedef struct {
  /* mms */
  uint8_t  mms_kind;        /* 1 */
  uint8_t  mms_len;         /* 1 */
  uint16_t mms_mss;         /* 2 */
  /* sack */
  uint8_t  sack_kind;       /* 1 */
  uint8_t  sack_len;        /* 1 */
  /* timestamp */
  uint8_t  ts_kind;         /* 1 */
  uint8_t  ts_len;          /* 1 */
  uint32_t ts_val;          /* 4 */
  uint32_t ts_echo;         /* 4 */
  /* nop */
  uint8_t  nop_nop;         /* 1 */
  /* window scale */
  uint8_t  win_scale_kind;  /* 1 */
  uint8_t  win_scale_len;   /* 1 */
  uint8_t  win_scale_count; /* 1 */
} tcpopts_t;

/*

    TCP header with options.

*/
typedef struct {
  struct tcphdr tcph;
  tcpopts_t     opts;
} tcphdr_t;

/*

    Prints out system error and exits.

*/
static void np_error(const char *);

/*

    Exits program immediately, prints out 'msg' to stderr first.

*/
static void np_error_exit(const char *);

/*

    Prepares IP header.

*/
static void np_setup_ip_hdr(struct iphdr *);

/*

    Calculates IP header checksum.

*/
static uint16_t np_ip_chksum(uint16_t *, int);

/*

    Prepares TCP header.

*/
static void np_setup_tcp_hdr(struct tcphdr *);

/*

    Calculates TCP header checksum.

*/
static uint16_t np_tcp_chksum(struct iphdr *, struct tcphdr *);

/*

    Sets the TTL value.

*/
static void np_set_sock_ttl(int, const uint8_t);

/*

    Opens a raw socket, captures outgoing TCP SYN packet, incoming SYN-ACK
    packet and outgoing ACK packet.

*/
static int np_raw_sock_listener(np_opts_t *);

/*

    Wrapper for np_raw_sock_listener function.

*/
static void * np_raw_sock_listener_thread(void *);

/*

    Creates a new local server socket and passes any data between this socket
    and the established remote socket.

*/
static int np_connection_local_forward(int);

/*

    Creates plain TCP socket and connects to given end-point in the options.

*/
static void np_sock_connect(np_opts_t *);

/*

    Wrapper for np_sock_connect function.

*/
static void * np_sock_connect_thread(void *);

/*

    Injects an IP packet to local TCP/IP stack.

*/
static int np_packet_inject(np_opts_t *, uint8_t *, uint32_t);

/*

    Prepares a TCP packet with attributes specified in the parameter.

*/
static void * np_packet_create(np_opts_t *);

/*

    Sends an IP packet out of its NIC.

*/
static int np_packet_send(np_opts_t *, uint8_t *, uint32_t);

/*

    Increases the TTL of a captured packet to maximum, recalculates the IP
    header checksum and resends the packet.

*/
static int np_increase_ttl_resend(np_opts_t *, uint8_t *, uint32_t);

/*

    Creates a TCP packet with low TTL and SYN flag set. Sends it out in order to
    create NAT mapping.

*/
static int np_connection_send_syn(np_opts_t *);

/*

    Sends connection info to the rendezvous server which will be then forwarded
    to the remote peer.

*/
static int np_connection_send_info(np_opts_t *, uint32_t, uint32_t);

/*

    Sends out a fake packet with payload after the connection has been
    established. It was observed that the firewalls may not allow incoming
    packets with payload unless some data has been sent out first.

*/
static int np_connection_send_fake(np_opts_t *);

/*

    Allocates memory for private options struct.

*/
int np_init(void);

/*

    Creates TCP connection to np_opts->server and asks for IP endpoints.

*/
static int np_connection_request(np_opts_t *);

/*

    Responds to connection request by sending out TCP SYN packet, injecting
    packet to its networking stack and sending out fake packet with payload
    afterwards.

*/
int np_connection_response(np_opts_t *);

/*

    Tries to establish a connection with a remote peer by initiating the TCP
    hole-punching procedure.

*/
int np_connection_establish(np_opts_t *);
