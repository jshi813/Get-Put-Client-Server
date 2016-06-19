#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>

#define PORT 1200
#define BACKLOG 5 // pending connections queue will hold
#define MAX_SADDR 100

int main () {
  fd_set master;
  fd_set read_fds;
  int fdmax;

  int listener, newfd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage remoteaddr;
  struct sockaddr_in sa;
  int yes = 1;
  int i, j;
  int nbytes;

  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  char server_address[MAX_SADDR];
  memset(&sa, 0, sizeof sa);
  gethostname(server_address, MAX_SADDR);

  sa.sin_family = AF_INET;
  sa.sin_port = htons(PORT);
  sa.sin_addr.s_addr = INADDR_ANY;

  listener = socket(AF_INET, SOCK_STREAM, 0);
  bind(listener, (struct sockaddr*)&sa, sizeof sa);

  listen(listener, BACKLOG);

  int length = sizeof sa;
  getsockname(listener, (struct sockaddr*)&sa, &length);

  printf("SERVER_ADDRESS %s\n", server_address);
  printf("SERVER_PORT %i\n", ntohs(sa.sin_port));

  FD_SET(listener, &master);
  fdmax = listener;

  // main loop
  for (;;) {
    read_fds = master;
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }

    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        // handling new connection from client
        if (i == listener) {
          newfd = accept(listener, NULL, NULL);
          if (newfd == -1) {
            perror("accept");
          } else {
            FD_SET(newfd, &master);
            if (newfd > fdmax) {
              fdmax = newfd;
            }
          }
        } else { // handle data from client
          char *buf = (char*)malloc(256);
          nbytes = recv(i, buf, 256, 0);
          if (nbytes <= 0) {
            close(i);
            FD_CLR(i, &master);
          }
          printf("%s\n", buf);
          toTitleCaps(buf);
          send(i, buf, nbytes, 0);

          free(buf);
        }
      }
    }
  }

  return 0;
}
