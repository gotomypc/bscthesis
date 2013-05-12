/*

    natpeer.c

    Simple executable which uses the library. It only parses the command line
    arguments and invokes methods accordingly.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "natpeer.h"

static np_opts_t opts = {
  .action    = NP_NONE,
  .ttl       = NP_MAX_TTL,
  .src_ip    = NULL,
  .dst_ip    = NULL,
  .src_prt   = 0,
  .dst_prt   = 0,
  .seq       = 0,
  .net_if    = NULL,
  .server    = NULL,
  .ts_val    = 0,
  .ts_echo   = 0,
  .tcp_flags = 0,
  .ack_seq   = 0,
  .payload   = 0,
  .nat       = NP_FALSE,
  .service   = NULL
};

/*

    Parses TCP flags argument, currently only SYN and ACK are required.

*/
void
np_parse_tcp_flags(const char *str, const int len)
{
  int i;
  char c;
  for (i = 0; i < len; i++) {
    c = str[i];
    if (c == 'S') {
      opts.tcp_flags |= NP_TCP_SYN;
    } else if (c == 'A') {
      opts.tcp_flags |= NP_TCP_ACK;
    }
  }
}

/*

    Parses arguments and constructs np_opts_t object accordingly.

*/
void
np_parse_args(int argc, char **argv)
{
  int i;
  for (i = 0; i < argc; i++) {

    if (strcmp("-S", argv[i]) == 0) {
      opts.src_ip = argv[++i];
      DEBUG("-S %s\n", opts.src_ip);

    } else if (strcmp("-Sp", argv[i]) == 0) {
      opts.src_prt = atoi(argv[++i]);
      DEBUG("-Sp %i\n", opts.src_prt);

    } else if (strcmp("-D", argv[i]) == 0) {
      opts.dst_ip = argv[++i];
      DEBUG("-D %s\n", opts.dst_ip);

    } else if (strcmp("-Dp", argv[i]) == 0) {
      opts.dst_prt = atoi(argv[++i]);
      DEBUG("-Dp %i\n", opts.dst_prt);

    } else if (strcmp(argv[i], "-server") == 0) {
      opts.server = argv[++i];
      DEBUG("-server %s\n", opts.server);

    } else if (strcmp("-ttl", argv[i]) == 0) {
      opts.ttl = atoi(argv[++i]);
      DEBUG("-ttl %i\n", opts.ttl);

    } else if (strcmp(argv[i], "-if") == 0) {
      opts.net_if = argv[++i];
      DEBUG("-if %s\n", opts.net_if);

    } else if (strcmp(argv[i], "-seq") == 0) {
      opts.seq = atoi(argv[++i]);
      DEBUG("-seq %i\n", opts.seq);

    } else if (strcmp(argv[i], "-tv") == 0) {
      opts.ts_val = atoi(argv[++i]);
      DEBUG("-tv %d\n", opts.ts_val);

    } else if (strcmp(argv[i], "-te") == 0) {
      opts.ts_echo = atoi(argv[++i]);
      DEBUG("-te %d\n", opts.ts_echo);

    } else if (strcmp(argv[i], "-ack") == 0) {
      opts.ack_seq = atoi(argv[++i]);
      DEBUG("-ack %d\n", opts.ack_seq);

    } else if (strcmp(argv[i], "-payload") == 0) {
      opts.payload = atoi(argv[++i]);
      DEBUG("-payload %d\n", opts.payload);

    } else if (strcmp(argv[i], "-service") == 0) {
      opts.service = argv[++i];
      DEBUG("-service %s\n", opts.service);

    } else if (strcmp(argv[i], "--nat") == 0) {
      opts.nat = NP_TRUE;
      DEBUG("--nat %d\n", opts.nat);

    } else if (argv[i][0] == '-' && argv[i][1] == 'f') {
      DEBUG("-f\n");
      np_parse_tcp_flags(argv[i], strlen(argv[i]));

    } else if (strcmp(argv[i], "--establish") == 0) {
      DEBUG("--establish\n");
      opts.action = NP_CON_ESTAB;

    } else if (strcmp(argv[i], "--response") == 0) {
      opts.action = NP_CON_RESP;
      DEBUG("--response\n");

    } else if (strcmp(argv[i], "--test") == 0) {
      DEBUG("--test\n");
      opts.action = NP_TEST;

    }

  }
}

int
main(int argc, char **argv)
{
  np_init();
  np_parse_args(argc, argv);

  if (opts.action == NP_CON_ESTAB)
    np_connection_establish(&opts);

  else if (opts.action == NP_CON_RESP)
    np_connection_response(&opts);

  else if (opts.action == NP_TEST)
    np_test(&opts);

  else
    printf("[WARN] no args specified\n");

  return 0;
}
