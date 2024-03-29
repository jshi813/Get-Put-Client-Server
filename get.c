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

void clear_buf(char *buf) {
  int i;
  for (i = 0; i < MAXBUFLEN-1; i++) {
    buf[i] = '\0';
  }
}

void chunked_write_file(int sockfd, char *filename) {
  int bytesRead = 1;
  char buf[MAXBUFLEN];
  clear_buf(buf);
  printf("%s\n", "writing to file");

  FILE *out;
  out = fopen(filename, "w");

  while(bytesRead > 0) {
    bytesRead = recv(sockfd, buf, MAXBUFLEN, 0);
    buf[bytesRead] = '\0'; // get rid of the random characters
    if (bytesRead > 0) {
      printf("bytes read: %i\n", bytesRead);
      printf("client: received '%s'\n",buf);
      fwrite(buf, 1, strlen(buf), out);
    }
  }
  fclose(out);
}

int main(int argc, char *argv[])
{
  int sockfd;
  struct addrinfo hints_udp, hints_tcp, *servinfo, *p;
  int rv;
  int numbytes;
  char buf[MAXBUFLEN];
  struct sockaddr_storage their_addr;
  char msg[strlen(argv[1])+2]; // use last byte to indicate get/put: 1 - get; 0 - put;

  strncpy(msg, argv[1], strlen(argv[1]));
  msg[strlen(argv[1])] = '1';
  msg[strlen(argv[1])+1] = '\0';
  printf("udp msg is %s\n", msg);

  clear_buf(buf);

  socklen_t addr_len;

  if (argc != 3) {
    fprintf(stderr,"usage: ./get src dst\n");
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
      freeaddrinfo(servinfo);
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

      chunked_write_file(sockfd, argv[2]);

      close(sockfd);

    } else {
      printf("File doesn't exist\n");
      exit(1);
    }
  }

  return 0;
}
