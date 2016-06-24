#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define UPORT "4950"
#define TPORT "5950"
#define SERVER_ADDR "jason-TP300LA" //TODO: Need to update accordingly
#define MAXBUFLEN 256

void chunked_send_file(int sockfd, char *buf) {
  int total = 0;
  int file_size = strlen(buf);
  int rv;
  char *file_buffer = 0;

  FILE *fp = fopen(buf, "r");
  if (fp) {
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    file_buffer = malloc(file_size);
    if (file_buffer) {
      rv = fread(file_buffer, 1, file_size, fp);
    }
    file_buffer[rv] = '\0';
    fclose(fp);
  } else {
    printf("File doesn't exist\n");
    exit(1);
  }

  if (file_buffer) {
    while (total < file_size) {
      rv = send(sockfd, file_buffer+total, 256, 0);
      printf("chunk sent: %i\n", rv);
      if (rv == -1) {
        perror("chunked send:");
        break;
      }
      total += rv;
    }
  }
}

void clear_buf(char *buf) {
  int i;
  for (i = 0; i < MAXBUFLEN-1; i++) {
    buf[i] = '\0';
  }
}

int main(int argc, char *argv[])
{
  int sockfd;
  struct addrinfo hints_udp, hints_tcp, *servinfo, *p;
  int rv;
  int numbytes;
  char buf[MAXBUFLEN];
  struct sockaddr_storage their_addr;

  char msg[strlen(argv[2])+2]; // use last byte to indicate get/put: 1 - get; 0 - put;
  strncpy(msg, argv[2], strlen(argv[2]));
  msg[strlen(argv[2])] = '0';
  msg[strlen(argv[2])+1] = '\0';
  printf("udp msg is %s\n", msg);

  clear_buf(buf);

  socklen_t addr_len;

  if (argc != 3) {
    fprintf(stderr,"usage: ./put src dst\n");
    exit(1);
  }

  memset(&hints_udp, 0, sizeof hints_udp);
  hints_udp.ai_family = AF_UNSPEC;
  hints_udp.ai_socktype = SOCK_DGRAM;

  memset(&hints_tcp, 0, sizeof hints_tcp);
  hints_tcp.ai_family = AF_UNSPEC;
  hints_tcp.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(SERVER_ADDR, UPORT, &hints_udp, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and make a socket
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
        p->ai_protocol)) == -1) {
      perror("talker: socket");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "talker: failed to create socket\n");
    return 2;
  }

  freeaddrinfo(servinfo);

  // exit if file doesn't exist
  if (access(argv[1], F_OK) == -1) {
    printf("File doesn't exist\n");
    exit(1);
  }

  if ((numbytes = sendto(sockfd, msg, strlen(msg), 0,
       p->ai_addr, p->ai_addrlen)) == -1) {
    perror("talker: sendto");
    exit(1);
  } else {
    perror("talker: sendto works");
  }

  addr_len = sizeof their_addr;

  if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0,
    (struct sockaddr *)&their_addr, &addr_len)) == -1) {
    perror("talker: recvfrom");
    exit(1);
  } else {
    printf("server sent %s\n", buf);
    if (strcmp("ok", buf) == 0) {
      printf("File exists\n");
      // connect through tcp and read
      if ((rv = getaddrinfo(SERVER_ADDR, TPORT, &hints_tcp, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
      }

      for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
          perror("client: socket");
          continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
          perror("client: connect");
          close(sockfd);
          continue;
        }
      }

      freeaddrinfo(servinfo);

      chunked_send_file(sockfd, argv[1]);

      close(sockfd);

    } else {
      printf("File doesn't exist\n");
      exit(1);
    }
  }

  return 0;
}
