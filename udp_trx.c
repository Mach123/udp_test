#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int sd_t;
struct sockaddr_in to_addr;
char dest_addr[20];

char buf_rcv[2000];
int buf_len;
int sd;
socklen_t sin_size;
struct sockaddr_in from_addr;
int port_number = 44444;
int on_off = 0;
FILE *fp;
int dont_stop_rx = 0;

struct timeval diff;
int timeToInt(struct timeval time) {
  return (time.tv_usec + time.tv_sec * 1000000);
}

int packet_tx_init() {
  if ((sd_t = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  // 送信先アドレスとポート番号を設定する
  // 受信プログラムと異なるあて先を設定しても UDP の場合はエラーにはならない
  to_addr.sin_family = AF_INET;
  to_addr.sin_port = htons(port_number);
  to_addr.sin_addr.s_addr = inet_addr(dest_addr);
#if 0
    // パケットをUDPで送信
    if (sendto(sd, buf_send, buf_len, 0,
              (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("sendto");
        return -1;
    }
#endif

  return 0;
}

int packet_rx_init() {
  struct sockaddr_in addr;

  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
  addr.sin_family = AF_INET;
//  addr.sin_port = htons(7);
  addr.sin_port = htons(port_number);

  addr.sin_addr.s_addr = INADDR_ANY;  // from any IP
  if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
  }
  return 0;
}

int packet_rx_close() {
  close(sd);
  return 0;
}

int packet_rx() {
  unsigned int counter = 0;
  int i;
  int dif;
  unsigned char c;
  uint32_t *p1, *p2;
  uint32_t *p3, *p4;
  uint32_t *p5, *p6;
  uint32_t *p7;
  uint16_t *q;
  int payload_size;
  int interval_us;

  struct timeval udp_time, now;

  memset(buf_rcv, 0, sizeof(buf_rcv));

  on_off = 1;
  q = (uint16_t *)&buf_rcv[2];    // size
  p1 = (uint32_t *)&buf_rcv[4];   // tx sec
  p2 = (uint32_t *)&buf_rcv[8];   // tx usec
  p3 = (uint32_t *)&buf_rcv[12];  // counter
  p4 = (uint32_t *)&buf_rcv[16];  // void
  p5 = (uint32_t *)&buf_rcv[20];  // rx sec
  p6 = (uint32_t *)&buf_rcv[24];  // rx usec
  p7 = (uint32_t *)&buf_rcv[28];  // rx usec
  while (on_off) {
    payload_size = recvfrom(sd, buf_rcv, sizeof(buf_rcv), 0,
                            (struct sockaddr *)&from_addr, &sin_size);
    if (payload_size < 0) {
      perror("recvfrom");
      return -1;
    }
    gettimeofday(&now, NULL);
    udp_time.tv_sec = *p1;
    udp_time.tv_usec = *p2;

    counter++;
    *p5 = now.tv_sec;
    *p6 = now.tv_usec;
//    *p7 = counter;
    *p4 = counter;
    sendto(sd_t, buf_rcv, payload_size, 0, (struct sockaddr *)&to_addr,
           sizeof(to_addr));

    timersub(&now, &udp_time, &diff);
    dif = timeToInt(diff);
    interval_us = *p4;
    payload_size = *q;
    c = buf_rcv[0];
    if (fp != NULL) {
      fwrite(buf_rcv, 1, 32, fp);
    }
    switch (c) {
      case 'C':
        break;
      case 'L':
        printf("L %10u ", counter);
        printf(" %10d %10d %10d\n", *p1, *p2, dif);
        break;
      case 'S':
        counter = 1;
        printf("S %10u %10d %10d\n", counter, interval_us, payload_size);
        break;
      case 'E':
        //      on_off = 0;
        printf("E %10u ", counter);
        printf(" %10d %10d %10d\n", *p1, *p2, dif);
        break;
      case 'F':
//        counter--;
//        on_off = 0;
        printf("F %10u ", counter);
        printf(" %10d %10d %10d  %10d\n", *p1, *p2, dif, *p3 - 1);
        fflush(stdout);
        if (dont_stop_rx) {
          counter = 0;
        } else {
          on_off = 0;
        }
        break;
      default:
        break;
    }
  }

  //  printf("%s\n", buf_rcv);
  return 1;
}
int main(int argc, char **argv) {
  int result;
  int temp;


  fp = NULL;
  sprintf(dest_addr, "%s", "192.168.0.1");
  if (argc > 1) {
    while ((result = getopt(argc, argv, "nd:p:")) != -1) {
      switch (result) {
        case 'd':
          sprintf(dest_addr, "%s", optarg);
          break;
        case 'p':
          temp = atoi(optarg);
          if (temp > 0)
          {
            port_number = temp;
          }
          fprintf(stderr, "Port_number = %d.\n", port_number);
          break;
        case 'n':
          dont_stop_rx = 1;
          break;

        case 'h': // No break;
        case '?':
          fprintf(stderr, "Availabe options: a c p s u\n");
//          fprintf(stderr, " s IP4_ADDRESS : Source address (0.0.0.0)\n");
          fprintf(stderr, " d IP4_ADDRESS : Destination address (192.168.1.1)\n");
          fprintf(stderr, " p PORT_NUMBER : Port number (44444)\n");
          fprintf(stderr, " n Dont stop RX\n");
//          fprintf(stderr, " l Enable log\n");
          exit(1);
        default:
          break;
      }
    }
    if (optind < argc) {
      if ((fp = fopen(argv[optind], "wb")) == NULL) {
        perror(argv[1]);
        exit(1);
      }
    }
  }
  fprintf(stdout, "Destination address = %s\n", dest_addr);

  packet_tx_init();
  packet_rx_init();
  packet_rx();
  packet_rx_close();
  if (fp != NULL) {
    fclose(fp);
  }
  return 0;
}
