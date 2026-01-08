#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <signal.h>
#include <unistd.h>


int port_number = 44444;

int log_enable = 0;

char buf_rcv[2000];
int buf_len;
int sd;
socklen_t sin_size;
struct sockaddr_in from_addr;
int on_off = 0;

int dont_stop_rx = 0;

FILE *fp;
char filename[128];

void make_filename() {
  char date_time[40];
  struct tm *timenow;
  time_t now = time(NULL);
  timenow = localtime(&now);

//    timenow = gmtime(&now);
    strftime(date_time, sizeof(date_time), "%y%m%d_%H%M%S", timenow);
    sprintf(filename, "./log/RX_%05d_%s.log", port_number, date_time);
}

struct timeval diff;
int timeToInt(struct timeval time) {
//  printf("%ld %ld \n", time.tv_usec , time.tv_sec);
  return (time.tv_usec + time.tv_sec * 1000000);
}

int packet_rx_init() {
  struct sockaddr_in addr;

  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
  addr.sin_family = AF_INET;
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
  int dif_ref = 0;
  unsigned char c;
  uint32_t *p1, *p2;
  uint32_t *p3, *p4;
  uint32_t *p5, *p6;
  uint32_t *p7;
  uint16_t *q;
  int payload_size;
  int interval_us;
  uint8_t tos;
  struct timeval udp_time, now;
  double rate;

  memset(buf_rcv, 0, sizeof(buf_rcv));

  on_off = 1;
  q = (uint16_t *)&buf_rcv[2];    // size
  p1 = (uint32_t *)&buf_rcv[4];   // tx sec
  p2 = (uint32_t *)&buf_rcv[8];   // tx usec
  p3 = (uint32_t *)&buf_rcv[12];  // counter
  p4 = (uint32_t *)&buf_rcv[16];  // void
  p5 = (uint32_t *)&buf_rcv[20];  // rx sec
  p6 = (uint32_t *)&buf_rcv[24];  // rx usec
  p7 = (uint32_t *)&buf_rcv[28];  // counter
  while (on_off) {
    if (recvfrom(sd, buf_rcv, sizeof(buf_rcv), 0, (struct sockaddr *)&from_addr,
                 &sin_size) < 0) {
      perror("recvfrom");
      return -1;
    }

#if 1
    //    printf("R\n");
    gettimeofday(&now, NULL);
    udp_time.tv_sec = *p1;
    udp_time.tv_usec = *p2;

    counter++;
    *p5 = now.tv_sec;
    *p6 = now.tv_usec;
    *p7 = counter;
    timersub(&now, &udp_time, &diff);
    dif = timeToInt(diff);
//    dif -= dif_ref

//    printf("%d \n", dif);
//    interval_us = *p4;
    payload_size = *q;
    c = buf_rcv[0];
    if (fp != NULL) {
      fwrite(buf_rcv, 1, 32, fp);
    }
    tos = (buf_rcv[1] >> 5) & 0x7;
    switch (c) {
      case 'C':
        break;
      case 'L':
        rate = 0.0;
        if (*p3 > 0) rate = 1.0 - (double)counter/(double)*p3;
//        printf("L %1u %10u /%10u %9.4e   %10d.%06d %10d\n", tos, counter, *p3, rate, *p1, *p2, dif - dif_ref);
        printf("L %1u %10u /%10u %9.4e   %10d.%06d %10d\n", tos, counter, *p3, rate, *p1, *p2, dif);
        fflush(stdout);
        break;
      case 'S':
        dif_ref = dif;
//        counter = 1;
//        printf("S %10u %6d\n", counter, payload_size);
        printf("S %4d %7u                          %10d.%06d %10d\n", payload_size, counter, *p1, *p2, dif);
//        printf("L %1u %10u /%10u %9.4e   %10d.%06d %10d\n", tos, counter, *p3, rate, *p1, *p2, dif);
        fflush(stdout);
        break;
      case 'E':
        rate = 0.0;
        if (*p3 > 0) rate = 1.0 - (double)counter/(double)*p3;
        //      on_off = 0;
        printf("E %1u %10u /%10u %9.4e", tos, counter, *p3, rate);
        printf("   %10d.%06d %10d\n", *p1, *p2, dif);
//        printf("   %10d.%06d %10d\n", *p1, *p2, dif - dif_ref);
        fflush(stdout);
        break;
      case 'F':
//        counter--;
        //        on_off = 0;
        if (*p3 > 0) rate = 1.0 - (double)(counter-1)/(double)(*p3-1);
        printf("\nF %1u %10u /%10u %9.4e", tos, counter, *p3, rate);
        printf("   %10d.%06d %10d\n\n", *p1, *p2, dif);
//        printf("   %10d.%06d %10d\n\n", *p1, *p2, dif - dif_ref);
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
#else
    c = buf_rcv[0];
    counter++;
    switch (c) {
      case 'C':
        break;
      case 'L':
        fprintf(stderr, "\rL %10u", counter);
//        printf(" %10d %10d %10d\n", *p1, *p2, dif);
        break;
      case 'S':
        counter = 1;
        printf("S %10u\n", counter);
        break;
      case 'E':
        //      on_off = 0;
        printf("\nE %10u \n", counter);
        fflush(stdout);
        break;
      case 'F':
        counter--;
        //        on_off = 0;
        printf("F %10u \n", counter);
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
#endif
  }

  //  printf("%s\n", buf_rcv);
  return 1;
}

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("Received SIGINT\n");
        on_off = 0;
        if (fp != NULL) {
          fclose(fp);
        }
        exit(3);
    }
}

int main(int argc, char **argv) {
  int result;
  int temp;

  if (signal(SIGINT, sig_handler) == SIG_ERR) {
      printf("\nCan't catch SIGINT\n");
      exit(5);
  }

  while ((result = getopt(argc, argv, "d:s:c:hp:lu:nt:")) != -1) {
    switch (result) {
      case 'p':
        temp = atoi(optarg);
        if (temp > 0) {
          port_number = temp;
        }
        fprintf(stderr, "Port_number = %d.\n", port_number);
        break;
      case 'n':
        dont_stop_rx = 1;
        break;
      case 'l':
        log_enable = 1;
        break;
      case 'h':  // No break;
      case '?':
        fprintf(stderr, "Availabe options: p n l\n");
        fprintf(stderr, " p PORT_NUMBER : Port number (default 44444)\n");
        fprintf(stderr, " n Dont stop RX\n");
        fprintf(stderr, " l Enable log\n");
        exit(1);
        break;
      default:
        break;
    }
  }

  fp = NULL;
  (void)make_filename();

  if (log_enable) {
    if ((fp = fopen(filename, "wb")) == NULL) {
      perror(filename);
      exit(1);
    }
  }

  packet_rx_init();
  packet_rx();
  packet_rx_close();
  if (fp != NULL) {
    fclose(fp);
  }
  return 0;
}
