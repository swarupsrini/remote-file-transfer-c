#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

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

int main(int argc, char* argv[]) {
  int cfd = socket(AF_INET, SOCK_STREAM, 0);
  
  // socket connection data
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &addr.sin_addr); 
  
  // connect socket to data
  if (-1 == connect(cfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) {
    perror("connect");
    return 1;
  }
  
  unsigned char buf[100];
  
  // write file name
  mywrite(cfd, argv[3], strlen(argv[3]));
  
  // read file status
  char sig[1];
  myread(cfd, sig, 1);
  if (sig[0] == 'n') {
    fprintf(stderr, "file not found on server\n");
    close(cfd);
    return 1;
  }
  
  // read file size
  off_t fsize;
  myread(cfd, &fsize, sizeof(off_t));
  fsize = ntohl(fsize);
  
  // read data
  FILE *f = fopen(argv[4], "w+");
  char buf1[BUFSIZE];
  off_t reads = 0;
  while (reads < fsize) {
    off_t readl = 0;
    while (readl < min(BUFSIZE, fsize - reads)) {
      readl += myread(cfd, buf1, min(BUFSIZE, fsize - reads));
    }
    fwrite(buf1, min(BUFSIZE, fsize - reads), 1, f);
    reads += readl;
  }

  close(cfd);
  return 0;
}