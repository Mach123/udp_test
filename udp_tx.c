#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#if 1

#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int param_lap = 100;

int sd;
struct sockaddr_in addr;
char dest_addr[20];
char source_addr[20] = {"0.0.0.0"};
char buf_send[1500];
int payload_size = 1000;
int port_number = 44444;
uint8_t tos = 0;
uint8_t precedence = 0;

unsigned long saddr, daddr;


int raw_udpip_setup(void)
{
  int sockfd;
  int on = 1;

  if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
  {
    perror("socket");
    return -1;
  }

  if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
  {
    perror("setsockopt");
    return -1;
  }

  return sockfd;
}

int raw_udpip_close(int sockfd)
{
  close(sockfd);
  return 0;
}

int raw_udpip_tx(int rawfd, uint8_t *packet, int totlen)
{
  ssize_t size;
  struct sockaddr_in dst_sin;
  struct iphdr *iph = (struct iphdr *)packet;

  bzero(&dst_sin, sizeof(dst_sin));
  dst_sin.sin_family = AF_INET;
  //  dst_sin.sin_port = ;
  dst_sin.sin_addr.s_addr = iph->daddr;
  size = sendto(rawfd, (void *)packet, totlen, 0,
                (const struct sockaddr *)&dst_sin, sizeof(dst_sin));
  return size;
}

/*
+------------------------------------------------------------------+
|IP Header fields modified on sending when IP_HDRINCL is specified |
+------------------------------------------------------------------+
|  Sending fragments with IP_HDRINCL is not supported currently.   |
+--------------------------+---------------------------------------+
|IP Checksum               |Always filled in.                      |
+--------------------------+---------------------------------------+
|Source Address            |Filled in when zero.                   |
+--------------------------+---------------------------------------+
|Packet Id                 |Filled in when passed as 0.            |
+--------------------------+---------------------------------------+
|Total Length              |Always filled in.                      |
+--------------------------+---------------------------------------+
 */

void udpip_hdr_fill(uint8_t *packet, uint8_t tos, uint32_t saddr,
                    uint32_t daddr, int dport, int len)
{
  struct iphdr *iph;
  struct udphdr *udph;

  iph = (struct iphdr *)packet;
  iph->version = 4;
  iph->ihl = 5;
  iph->tos = tos;
  iph->tot_len = 0;
  iph->id = 0;
  iph->frag_off = 0;
  iph->ttl = 16;
  iph->protocol = IPPROTO_UDP;
  iph->check = 0;
  iph->saddr = saddr;
  iph->daddr = daddr;

  udph = (struct udphdr *)(packet + sizeof(struct iphdr));
  udph->source = htons(dport); /* dummy */
  udph->dest = htons(dport);
  udph->len = htons(len + sizeof(struct udphdr));
  udph->check = 0;

  return;
}

// int gettimeofday(struct timeval *tv, struct timezone *tz);
// int settimeofday(const struct timeval *tv, const struct timezone *tz);


int udp_sendto(int sd, uint8_t *buf_send, int payload_size, int dummy,
               struct sockaddr *addr, int sizeof_addr)
{
  uint8_t tx_buffer[1500];
  ssize_t size;
  //  struct iphdr *iph = (struct iphdr *)packet;
  int totlen;
  uint8_t *body;

//  printf("%lx ", saddr);

  udpip_hdr_fill(tx_buffer, tos, saddr, daddr, port_number,
                 payload_size);
  body = (tx_buffer + sizeof(struct iphdr) + sizeof(struct udphdr));
  memcpy(body, buf_send, payload_size);
  totlen = payload_size + sizeof(struct udphdr) + sizeof(struct iphdr);

  size = sendto(sd, (void *)tx_buffer, totlen, 0, (const struct sockaddr *)addr,
                sizeof_addr);
}

unsigned int loop_count = 1000;
int interval_us = 1000;
int timer_on_off = 0;
int pps = 1000;
int pps_d = 1;
int pps_r = 0;
int seconds = 10;


struct timeval before, now, sub;
uint32_t counter = 0, dif = 0, min = 0x7ffffff, max = 0;
int timeToInt(struct timeval time)
{
  return (time.tv_usec + time.tv_sec * 1000000);
}

void timer_handler_main()
{
  uint32_t *p;
  uint16_t *q;
  counter++;
  buf_send[0] = 'C';
  buf_send[1] = tos;

  if (counter == 1)
  {
    buf_send[0] = 'S';
  }
  else
  {
    if (param_lap > 0)
    {
      if ((counter % param_lap) == 0)
      {
        fprintf(stderr, "\rL %10u", counter);

        buf_send[0] = 'L';
      }
    }
    if (counter >= loop_count)
    {
      timer_on_off = 0;
      buf_send[0] = 'E';
    }
  }
  gettimeofday(&now, NULL);
  q = (uint16_t *)&buf_send[2];
  *q = payload_size;
  p = (uint32_t *)&buf_send[4];
  *p = now.tv_sec;
  p = (uint32_t *)&buf_send[8];
  *p = now.tv_usec;
  p = (uint32_t *)&buf_send[12];
  *p = counter;

#if 1
  udp_sendto(sd, buf_send, payload_size, 0, (struct sockaddr *)&addr,
              sizeof(addr));
#else
  if (buf_send[0] != 'E')
  {
    udp_sendto(sd, buf_send, payload_size, 0, (struct sockaddr *)&addr,
               sizeof(addr));
  }
#endif
  return;
}


int sum_rem = 0;

int packet_send(uint8_t c)
{
  uint16_t *q;
  uint32_t *p;

  buf_send[0] = c;
  counter++;
  buf_send[1] = tos;
  q = (uint16_t *)&buf_send[2];
  *q = payload_size;
  gettimeofday(&now, NULL);
  p = (uint32_t *)&buf_send[4];
  *p = now.tv_sec;
  p = (uint32_t *)&buf_send[8];
  *p = now.tv_usec;
  p = (uint32_t *)&buf_send[12];
  *p = counter;
  udp_sendto(sd, buf_send, payload_size, 0, (struct sockaddr *)&addr,
             sizeof(addr));
}


/*
 * タイマハンドラ
 */
void timer_handler(int signum)
{
  gettimeofday(&now, NULL);
  if (counter > 1) {
    timersub(&now, &before, &sub);
    dif = timeToInt(sub);
    if (dif < min)
    {
      min = dif;
    }
    if (dif > max)
    {
      max = dif;
    }
  }
  before = now;

  for (int i = 0; i < pps_d; i++)
    timer_handler_main();
  sum_rem += pps_r;
  if (sum_rem >= 1000) {
    sum_rem -= 1000;
    timer_handler_main();
  }
  return;
}

int timer_set()
{
  int i;
  unsigned int loop;
  struct sigaction act, oldact;
  timer_t tid;
  struct itimerspec itval;
  int total;
  double duration, ticks;

  memset(&act, 0, sizeof(struct sigaction));
  memset(&oldact, 0, sizeof(struct sigaction));

  // シグナルハンドラの登録
  act.sa_handler = timer_handler;
  act.sa_flags = SA_RESTART;
  if (sigaction(SIGALRM, &act, &oldact) < 0)
  {
    perror("sigaction()");
    return -1;
  }

  timer_on_off = 1;
  // タイマ割り込みを発生させる
  itval.it_value.tv_sec = interval_us / 1000000; // 最初の1回目
  itval.it_value.tv_nsec = (interval_us % 1000000) * 1000;
  itval.it_interval.tv_sec = interval_us / 1000000; // 2回目以降
  itval.it_interval.tv_nsec = (interval_us % 1000000) * 1000;

  total = loop_count;
  ticks = 1.0 / 1000000.0 * (double)interval_us;
  duration = (double)total * ticks;
//  printf("Duration = %.3f sec, Ticks = %.2f msec.\n", duration, ticks * 1000);
  printf("Duration = %d sec, Ticks = %.2f msec.\n", seconds, ticks * 1000);

//  buf_send[0] = 'S';
//  udp_sendto(sd, buf_send, payload_size, 0, (struct sockaddr *)&addr,
//             sizeof(addr));

//  usleep(500000);

//    packet_send('F');
//    usleep(500000);

  // タイマの作成
  if (timer_create(CLOCK_REALTIME, NULL, &tid) < 0)
  {
    perror("timer_create");
    return -1;
  }

  //  タイマのセット
  if (timer_settime(tid, 0, &itval, NULL) < 0)
  {
    perror("timer_settime");
    return -1;
  }

  loop = 0;
  while (timer_on_off)
  {
    loop++;
    // シグナル(タイマ割り込み)が発生するまでサスペンドする
    pause();
    if ((loop % 1000) == 0)
    {
      //	printf("%8d\n", loop);
    }
  }
  // タイマの解除
  timer_delete(tid);
  // シグナルハンドラの解除
  sigaction(SIGALRM, &oldact, NULL);

#if 0
  if (buf_send[0] == 'E')
  {
//    usleep(500000);
    usleep(100000);
    udp_sendto(sd, buf_send, payload_size, 0, (struct sockaddr *)&addr,
               sizeof(addr));
  }
#endif
  printf("\nExpected %d, Actual %d , Min %d, Max %d\n", loop_count, counter, min,
         max);
  return 0;
}

int packet_set()
{
  sd = raw_udpip_setup();
  if (sd < 0)
  {
    perror("socket");
    return -1;
  }

  // 送信先アドレスとポート番号を設定する
  // 受信プログラムと異なるあて先を設定しても UDP の場合はエラーにはならない
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_number);
  addr.sin_addr.s_addr = inet_addr(dest_addr);
  bzero(buf_send, sizeof(buf_send));
  sprintf(buf_send, "%s", " ");

  return 0;
}


#define MILION 1000000
#define LIMIT 1000
int main(int argc, char **argv)
{
  int result;
  int temp;
  int dont_stop_rx = 0;

  sprintf(dest_addr, "%s", "192.168.0.158");

  while ((result = getopt(argc, argv, "d:s:c:hp:l:u:nt:x:o:")) != -1)
  {
    switch (result)
    {
    case 's':
      sprintf(source_addr, "%s", optarg);
      break;
    case 'd':
      sprintf(dest_addr, "%s", optarg);
      break;
    case 'c':
      temp = atoi(optarg);
      if (temp > 0)
      {
        seconds = temp;
      }
      break;
    case 'p':
      temp = atoi(optarg);
      if (temp > 0)
      {
        port_number = temp;
      }
      fprintf(stderr, "Port_number = %d.\n", port_number);
      break;
    case 'x':
      temp = atoi(optarg);
      if (temp > 0)
      {
        param_lap = temp;
      }
      fprintf(stderr, "lap_interval = %d.\n", param_lap);
      break;
    case 'o':
      temp = atoi(optarg);
      pps = temp;
      if (pps <= LIMIT)
      {
        interval_us = (int)(MILION / pps);
        pps_d = 1;
        pps_r = 0;
      } else {
        interval_us = 1000;
        pps_d = (int)(pps / 1000);
        pps_r = pps % 1000;
      }

      break;

    case 'l':
      temp = atoi(optarg);
      if ((temp > 0) & (temp < 4000))
      {
        payload_size = temp;
      }
      break;
    case 'u':
      temp = atoi(optarg);
      if ((temp > 9) & (temp < 10000000))
      {
        interval_us = temp;
      }
      break;
    case 't':
      temp = atoi(optarg);
      precedence = temp & 0x7;
      tos = precedence << 5;
      break;
    case 'n':
      dont_stop_rx = 1;
      break;
    case 'h': // No break;
    case '?':
      fprintf(stderr, "Availabe options: a c p s u\n");
      fprintf(stderr, " s IP4_ADDRESS : Source address (0.0.0.0)\n");
      fprintf(stderr, " d IP4_ADDRESS : Destination address (192.168.1.1)\n");
      fprintf(stderr, " c ITERATION : Loop counts (10) \n");
      fprintf(stderr, " t TOS : TOS priority:0-7 (0)\n");
      fprintf(stderr, " p PORT_NUMBER : Port number (44444)\n");
      fprintf(stderr, " o PPS : Packets Per Second (1000)\n");
      fprintf(stderr, " l UDP_PAYLOAD_SIZE : UDP payload size  (1000)\n");
//      fprintf(stderr, " u INTERVAL_MICROSEC : Interval  (1000)\n");
      fprintf(stderr, " x LAP_INTERVAL : Interval  (100)\n");
      fprintf(stderr, " n No indication TX for stop RX\n");
      exit(1);
      break;
    default:
      break;
    }
  }

  loop_count = pps * seconds;

  fprintf(stdout, "Source      address = %s\n", source_addr);
  fprintf(stdout, "Destination address = %s\n", dest_addr);
  fprintf(stdout, "Precedence  = %d.\n", precedence);
  fprintf(stdout, "Payload size = %d.\n", payload_size);
  fprintf(stdout, "Tick = %d(usec).\n", interval_us);
  fprintf(stdout, "Loop count = %d.\n", loop_count);
  fprintf(stdout, "Estimated rate  = %g(MBPS).\n",
          8.0 * pps * (payload_size + 66) / 1000000.);
//          8.0 / interval_us * (payload_size + 66));
  fprintf(stdout, "PPS = %d %d %d\n", pps, pps_d, pps_r);

  inet_pton(AF_INET, source_addr, &saddr);
  inet_pton(AF_INET, dest_addr, &daddr);

  packet_set();
  timer_set();
  if (!dont_stop_rx)
  {
    usleep(1000000);
    packet_send('F');
  }
  raw_udpip_close(sd);
}

#endif