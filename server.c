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
#define MAXDATASIZE 256

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void chunked_send_file(int sockfd, char *filename) {
  int total = 0;
  int file_size = strlen(filename);
  int rv;
  char *file_buffer = 0;


  FILE *fp = fopen(filename, "r");
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

void chunked_write_file(int sockfd, char *filename) {
  char buf[MAXBUFLEN];
  clear_buf(buf);
  printf("%s\n", "writing to file");
  int bytesRead = 1;

  FILE *out;
  out = fopen(filename, "w");

  while(bytesRead > 0) {
    bytesRead = recv(sockfd, buf, MAXDATASIZE, 0);
    buf[bytesRead] = '\0'; // get rid of the random characters
    if (bytesRead > 0) {
      printf("bytes read: %i\n", bytesRead);
      printf("client: received '%s'\n",buf);
      fwrite(buf, 1, strlen(buf), out);
    }
  }
  fclose(out);
}

void clear_buf(char *buf) {
  int i;
  for (i = 0; i < MAXBUFLEN-1; i++) {
    buf[i] = '\0';
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
  char filename[MAXBUFLEN];
  char op[2];
  socklen_t addr_len, sin_size;
  char s[INET6_ADDRSTRLEN];
  char *msg_ok = "ok";
  char *msg_error = "error";

  // variables for sending file
  int expectTCP = 1;
  char *file_buffer = 0;
  int file_size;

  op[1] = '\0';
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
      strncpy(filename, buf, strlen(buf)-1);
      op[0] = buf[strlen(buf)-1];
      printf("filename is %s\n", filename);
      printf("op is %s\n", op);
      //TODO: check if file exists and send message accordingly
      if (op[0] == '1') {
        if (access(filename, F_OK) != -1) {
          printf("%s\n", msg_ok);
          sendto(sockfd, msg_ok, strlen(msg_ok), 0, (struct sockaddr *)&their_addr, addr_len);
          expectTCP = 1;
        } else {
          sendto(sockfd, msg_error, strlen(msg_error), 0, (struct sockaddr *)&their_addr, addr_len);
          expectTCP = 0;
        }
      } else {
        printf("%s\n", msg_ok);
        sendto(sockfd, msg_ok, strlen(msg_ok), 0, (struct sockaddr *)&their_addr, addr_len);
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

        /* handles get/put requests */

        if (op[0] == '1') { // get request
          chunked_send_file(new_fd, filename);
        } else { // put request
          chunked_write_file(new_fd, filename);
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
    }
    clear_buf(buf);
  }

  close(sockfd);

  return 0;
}
