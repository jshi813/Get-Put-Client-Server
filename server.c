/*
** listener.c -- a datagram sockets "server" demo
*/

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

#define UPORT "4950" // UDP port
#define TPORT "5950" // TCP port
#define chunk 256 // server send 256b at a time
#define BACKLOG 5
#define MAXBUFLEN 256

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void chunked_send_file(int sockfd, char *buf) {
  int total = 0;
  int file_size = strlen(buf);
  int rv;

  while (total < file_size) {
    rv = send(sockfd, buf+total, 256, 0);
    printf("chunk sent: %i\n", rv);
    if (rv == -1) {
      perror("chunked send:");
      break;
    }
    total += rv;
  }
}

int main(void)
{
  int sockfd, sockfd_tcp, new_fd;
  struct addrinfo hints_udp, hints_tcp, *servinfo, *p;
  int rv;
  int numbytes;
  struct sockaddr_storage their_addr;
  char buf[MAXBUFLEN];
  socklen_t addr_len, sin_size;
  char s[INET6_ADDRSTRLEN];
  char *msg_ok = "ok";
  char *msg_error = "error";

  // variables for sending file
  int expectTCP = 0;
  char *file_buffer = 0;;
  int file_size;

  memset(&hints_udp, 0, sizeof hints_udp);
  memset(&hints_tcp, 0, sizeof hints_tcp);
  hints_udp.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
  hints_udp.ai_socktype = SOCK_DGRAM;
  hints_udp.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, UPORT, &hints_udp, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
        p->ai_protocol)) == -1) {
      perror("listener: socket");
      continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("listener: bind");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "listener: failed to bind udp socket\n");
    return 2;
  }

  freeaddrinfo(servinfo);

  printf("listener: waiting to recvfrom...\n");

  //TODO: we should loop and listen on TPORT too
  hints_tcp.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
  hints_tcp.ai_socktype = SOCK_STREAM;
  hints_tcp.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, TPORT, &hints_tcp, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd_tcp = socket(p->ai_family, p->ai_socktype,
        p->ai_protocol)) == -1) {
      perror("listener: socket");
      continue;
    }

    if (bind(sockfd_tcp, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd_tcp);
      perror("listener: bind");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "listener: failed to bind tcp socket\n");
    return 2;
  }

  if (listen(sockfd_tcp, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  freeaddrinfo(servinfo);

  addr_len = sizeof their_addr;
  while(1) {

    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
      (struct sockaddr *)&their_addr, &addr_len)) == -1) {
      perror("recvfrom");
      exit(1);
    } else {
      //TODO: check if file exists and send message accordingly
      if (access(buf, F_OK) != -1) {
        sendto(sockfd, msg_ok, strlen(msg_ok), 0, (struct sockaddr *)&their_addr, addr_len);
        expectTCP = 1;
      } else {
        sendto(sockfd, msg_error, strlen(msg_error), 0, (struct sockaddr *)&their_addr, addr_len);
        expectTCP = 0;
      }
    }
    if (expectTCP) {
      printf("server: waiting for tcp connections...\n");

      sin_size = sizeof their_addr;
      new_fd = accept(sockfd_tcp, (struct sockaddr *)&their_addr, &sin_size);

      inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr),
        s, sizeof s);
      printf("server: got connection from %s\n", s);

      if (!fork()) { // this is the child process
        close(sockfd_tcp); // child doesn't need the listener

        /***************************/
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
        }

        printf("file in server buffer is %s\n", file_buffer);

        if (file_buffer) {
          chunked_send_file(new_fd, file_buffer);
        }


        /***************************/
        close(new_fd);
        exit(0);
      }
      close(new_fd);  // parent doesn't need this

      printf("listener: got packet from %s\n",
        inet_ntop(their_addr.ss_family,
          get_in_addr((struct sockaddr *)&their_addr),
          s, sizeof s));
      printf("listener: packet is %d bytes long\n", numbytes);
      buf[numbytes] = '\0';
      printf("listener: packet contains \"%s\"\n", buf);
    }
  }

  close(sockfd);

  return 0;
}
