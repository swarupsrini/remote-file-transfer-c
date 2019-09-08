#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define BUFSIZE 1024

off_t min(off_t a, off_t b) {
  return a<b?a:b;
}

off_t myread(int fd, void *buf, size_t count) {
  int s = read(fd, buf, count);
  if (s < 0){
    perror("read");
    exit(1);
  }
}

off_t mywrite(int fd, void *buf, size_t count) {
  int s = write(fd, buf, count);
  if (s < 0){
    perror("write");
    exit(1);
  }
}

int main(int argc, char *argv[]) {
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  
  // socket connection data
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in)); // clear padding
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(argv[1]));
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // open to any incoming IP
  
  // binding socket to the connection data
  if (-1 == bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) { 
    perror("bind");
    return 1;
  }
  
  // setting sfd to be open to new connections
  if (-1 == listen(sfd, 2)) {
    perror("listen");
    return 1;
  }
  
  /*
   creating epoll to wait until either
     (1) a client connects, i.e. we can call accept without blocking
         in which case we can add the client's fd to the epoll
     (2) a client has written stuff in which case we:
         read file name
         report validity of file
         if valid, send file content
  */
  // we reuse this epoll for future (2)'s
  int ep = epoll_create1(0);
  struct epoll_event e;
  memset(&e, 0, sizeof(struct epoll_event));
  e.events = EPOLLIN; // wanna initially know when client writes
  e.data.fd = sfd; // if it's the socket we can read from, i.e. client ready for connection
  epoll_ctl(ep, EPOLL_CTL_ADD, sfd, &e);
  
  // infinitely wait for either (1) or (2)
  for (;;) { 
    epoll_wait(ep, &e, 1, -1);
    
    // if (1), i.e. new client wanna connect
    if (e.data.fd == sfd) {
      // accept a new client
      struct sockaddr_in client_addr;
      socklen_t client_len = sizeof(struct sockaddr_in);
      int cfd = accept(sfd, (struct sockaddr *) &client_addr, &client_len);
      // if we got a new client, we add it to the epoll list
      // to wait for it to write stuff, i.e. (2) on the client
      if (cfd != -1) {
        e.events = EPOLLIN;
        e.data.fd = cfd;
        epoll_ctl(ep, EPOLL_CTL_ADD, cfd, &e);
      }
    }
    // otherwise epoll waited for (2), i.e. client to write
    else {
      int cfd = e.data.fd;
      unsigned char buf[100];
      memset(&buf, 0, sizeof(buf));
      int n = read(cfd, buf, 100);
      // we now have the file name in buf (probably)
      
      if (n <= 0) {
        //printf("Client fd=%d leaves.\n", cfd);
        epoll_ctl(ep, EPOLL_CTL_DEL, cfd, NULL);
        close(cfd);
      }
      else {
        // if invalid file
        if (access(buf, R_OK) == -1) {
          perror("client requested invalid file");
          //write(cfd, (char[]){'n'}, 1);
          mywrite(cfd, "n", 1);
          close(cfd);
        }
        // file can be read
        else {
          mywrite(cfd, "y", 1);
          int fd = open(buf, 'r');
          
          // write file size
          off_t fsize = lseek(fd, 0, SEEK_END);
          off_t fsize1 = htonl(fsize);
          lseek(fd, 0, SEEK_SET);
          mywrite(cfd, &fsize1, sizeof(off_t));
          
          // read from file, write to cfd
          char buf1[BUFSIZE];
          off_t written = 0;
          while (written < fsize) {
            memset(&buf1, 0, BUFSIZE);
            myread(fd, buf1, BUFSIZE);
            off_t lwritten = 0;
            while (lwritten < min(BUFSIZE, fsize - written)) {
              lwritten += mywrite(cfd, buf1, min(BUFSIZE, fsize - written));
            }
            written += lwritten;
          }
          close(fd);
          close(cfd);
        }
      }
    }
  }
}