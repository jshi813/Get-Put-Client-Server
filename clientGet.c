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
#define MAXBUFLEN 100

int main(int argc, char *argv[])
{
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int numbytes;
  char buf[MAXBUFLEN];
  struct sockaddr_storage their_addr;
  socklen_t addr_len;

  if (argc != 3) {
    fprintf(stderr,"usage: ./clientGet src dst\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  if ((rv = getaddrinfo(SERVER_ADDR, UPORT, &hints, &servinfo)) != 0) {
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

  if ((numbytes = sendto(sockfd, argv[1], strlen(argv[1]), 0,
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
      printf("LEEORY\n");
    } else { // file doesn't exist
      printf("File doesn't exist\n");
      exit(1);
    }
  }

  freeaddrinfo(servinfo);

  printf("talker: sent %d bytes to %s\n", numbytes, SERVER_ADDR);
  close(sockfd);

  return 0;
}
