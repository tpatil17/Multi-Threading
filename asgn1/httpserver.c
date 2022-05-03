#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

/**
   Converts a string to an 16 bits unsigned integer.
   Returns 0 if the string is malformed or out of the range.
 */
uint16_t strtouint16(char number[]) {
  char *last;
  long num = strtol(number, &last, 10);
  if (num <= 0 || num > UINT16_MAX || *last != '\0') {
    return 0;
  }
  return num;
}
/**
   Creates a socket for listening for connections.
   Closes the program and prints an error message on error.
 */
int create_listen_socket(uint16_t port) {
  struct sockaddr_in addr;
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    err(EXIT_FAILURE, "socket error");
  }
  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htons(INADDR_ANY);
  addr.sin_port = htons(port);
  if (bind(listenfd, (struct sockaddr *)&addr, sizeof addr) < 0) {
    err(EXIT_FAILURE, "bind error");
  }
  if (listen(listenfd, 500) < 0) {
    err(EXIT_FAILURE, "listen error");
  }
  return listenfd;
}

struct Response {
  char version[24];
  int status_code;
  char status_phrase[100];
  char header[19];
  long length;
  char message[64];
};

// Function implementations -- GET --

void Get(char file[], int connfd, struct Response res) {

  char resp_buf[1024];

  int fd;

  int bytes_read;

  struct stat ln;

  stat(file, &ln);

  if (access(file, F_OK) != 0) {

    res.status_code = 404;
    strcpy(res.status_phrase, "File Not Found");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "File Not Found\n");
    res.length = strlen(res.message);
    sprintf(resp_buf, "%s %d %s\r\n%s:%ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buf, strlen(resp_buf));
    return;
  }

  stat(file, &ln);

  if (S_ISREG(ln.st_mode) == 0) {

    res.status_code = 403;
    strcpy(res.status_phrase, "Forbidden");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "Forbidden\n");
    res.length = strlen(res.message);
    sprintf(resp_buf, "%s %d %s\r\n%s:%ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buf, strlen(resp_buf));
    return;
  }

  fd = open(file, O_RDONLY);

  if (fd == -1) {

    printf("File opening problem %d\n", errno);
    exit(1);
  }

  res.length = ln.st_size;

  sprintf(resp_buf, "%s %d %s\r\n%s:%ld\r\n\r\n", res.version, res.status_code,
          res.status_phrase, res.header, res.length);

  write(connfd, resp_buf, strlen(resp_buf));
  while ((bytes_read = read(fd, resp_buf, 1024)) > 0) {

    if ((write(connfd, resp_buf, bytes_read)) == -1) {
      res.status_code = 500;
      strcpy(res.status_phrase, "Internal Server Error");
      strcpy(res.header, "Content-Length");
      strcpy(res.message, "Internal Server Error\n");
      res.length = strlen(res.message);
      sprintf(resp_buf, "%s %d %s\r\n%s:%ld\r\n\r\n%s", res.version,
              res.status_code, res.status_phrase, res.header, res.length,
              res.message);
      write(connfd, resp_buf, strlen(resp_buf));
      return;
    }
  }

  if (bytes_read == -1) {
    res.status_code = 500;
    strcpy(res.status_phrase, "Internal Server Error");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "Internal Server Error\n");
    res.length = strlen(res.message);
    sprintf(resp_buf, "%s %d %s\r\n%s:%ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buf, strlen(resp_buf));
    return;
  }

  close(fd);
  return;
}

// Function implementation --- PUT ---

void Put(char file[], int connfd, struct Response res, char parser[],
         int offset) {

  int bytes;

  int fd;

  long limit;

  limit = res.length;

  char resp_buffer[1024];

  struct stat ln;

  stat(file, &ln);

  if (S_ISDIR(ln.st_mode) != 0) {

    res.status_code = 403;
    strcpy(res.status_phrase, "Forbidden");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "Forbidden\n");
    res.length = strlen(res.message);
    sprintf(resp_buffer, "%s %d %s\r\n%s:%ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buffer, strlen(resp_buffer));
    return;
  }

  if (access(file, F_OK) == 0) {

    if (S_ISREG(ln.st_mode) == 0) {

      res.status_code = 403;
      strcpy(res.status_phrase, "Forbidden");
      strcpy(res.header, "Content-Length");
      strcpy(res.message, "Forbidden\n");
      res.length = strlen(res.message);
      sprintf(resp_buffer, "%s %d %s\r\n%s:%ld\r\n\r\n%s", res.version,
              res.status_code, res.status_phrase, res.header, res.length,
              res.message);
      write(connfd, resp_buffer, strlen(resp_buffer));
      return;
    }

    fd = open(file, O_WRONLY | O_TRUNC);

    res.status_code = 200;
    strcpy(res.status_phrase, "OK");
    res.length = 3;
    strcpy(res.header, "Content-Length");
    strcpy(res.version, "HTTP/1.1");
    strcpy(res.message, "OK\n");
  }

  if (access(file, F_OK) != 0) {

    fd = open(file, O_WRONLY | O_CREAT, 0644);

    res.status_code = 201;
    strcpy(res.status_phrase, "Created");
    res.length = 8;
    strcpy(res.header, "Content-Length");
    strcpy(res.version, "HTTP/1.1");
    strcpy(res.message, "Created\n");
  }

  sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
          res.status_code, res.status_phrase, res.header, res.length,
          res.message);
  write(connfd, resp_buffer, strlen(resp_buffer));

  if (limit < (4096 - offset)) {
    write(fd, parser + offset, limit);
    close(fd);
  }

  else {
    int total;

    write(fd, parser + offset, strlen(parser) - offset);

    total = strlen(parser) - offset;

    while ((bytes = read(connfd, parser, 4096)) > 0) {

      total += bytes;

      if (limit >= total) {

        write(fd, parser, bytes);

      } else {

        write(fd, parser, bytes - (total - limit));
        break;
      }
    }

    if (bytes == -1) {
      res.status_code = 500;
      strcpy(res.status_phrase, "Internal Server Error");
      strcpy(res.header, "Content-Length");
      strcpy(res.message, "Internal Server Error\n");
      res.length = strlen(res.message);
      sprintf(resp_buffer, "%s %d %s\r\n%s:%ld\r\n\r\n%s", res.version,
              res.status_code, res.status_phrase, res.header, res.length,
              res.message);
      write(connfd, resp_buffer, strlen(resp_buffer));
      return;
    }

    close(fd);
  }

  return;
}

void Append(char file[], int connfd, struct Response res, char parser[],
            int offset) {

  int bytes;

  int fd;

  long limit;

  limit = res.length;

  struct stat ln;

  char resp_buffer[1024];

  if (access(file, F_OK) != 0) {

    res.status_code = 404;
    strcpy(res.status_phrase, "File Not Found");
    res.length = 15;
    strcpy(res.header, "Content-Length");
    strcpy(res.version, "HTTP/1.1");
    strcpy(res.message, "File Not Found\n");
    sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);

    write(connfd, resp_buffer, strlen(resp_buffer));
    return;
  }

  stat(file, &ln);

  if (S_ISREG(ln.st_mode) == 0) {
    res.status_code = 403;
    strcpy(res.status_phrase, "Forbidden");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "Forbidden\n");
    res.length = strlen(res.message);
    sprintf(resp_buffer, "%s %d %s\r\n%s:%ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buffer, strlen(resp_buffer));
    return;
  }
  fd = open(file, O_WRONLY | O_APPEND);

  if (fd == -1) {

    printf("open failure:\n%d", errno);
    exit(1);
  }

  res.status_code = 200;
  strcpy(res.status_phrase, "OK");
  res.length = 3;
  strcpy(res.header, "Content-Length");
  strcpy(res.version, "HTTP/1.1");
  strcpy(res.message, "OK\n");

  sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
          res.status_code, res.status_phrase, res.header, res.length,
          res.message);

  write(connfd, resp_buffer, strlen(resp_buffer));

  if (limit < (4096 - offset)) {
    write(fd, parser + offset, limit);
    close(fd);

  } else {
    int total;

    write(fd, parser + offset, strlen(parser) - offset);

    total = strlen(parser) - offset;

    while ((bytes = read(connfd, parser, 4096)) > 0) {

      total += bytes;

      if (limit >= total) {
        write(fd, parser, bytes);

      } else {

        write(fd, parser, bytes - (total - limit));
        break;
      }
    }
    if (bytes == -1) {
      res.status_code = 500;
      strcpy(res.status_phrase, "Internal Server Error");
      strcpy(res.header, "Content-Length");
      strcpy(res.message, "Internal Server Error\n");
      res.length = strlen(res.message);
      sprintf(resp_buffer, "%s %d %s\r\n%s:%ld\r\n\r\n%s", res.version,
              res.status_code, res.status_phrase, res.header, res.length,
              res.message);
      write(connfd, resp_buffer, strlen(resp_buffer));
      return;
    }

    close(fd);
  }

  return;
}

void handle_connection(int connfd) {
  // make the compiler not complain
  char parser[4096];
  int count = 0;

  struct Response res;
  // get the method
  read(connfd, parser, 4096);

  char method[8];
  char url[19], version[10];

  char buff_res[1024];

  if ((count = sscanf(parser, "%s /%s %s\r\n", method, url, version)) != 3) {

    strcpy(res.version, "HTTP/1.1");
    res.status_code = 400;
    strcpy(res.status_phrase, "Bad Request");
    strcpy(res.header, "Content-Length");
    res.length = 12;
    sprintf(buff_res, "%s %d %s\r\n%s: %ld\r\n\r\n", res.version,
            res.status_code, res.status_phrase, res.header, res.length);
    write(connfd, buff_res, strlen(buff_res));
    write(connfd, "Bad Request\n", 12);

  }

  else {

    strcpy(res.version, "HTTP/1.1");

    if (strcmp(method, "GET") == 0 | strcmp(method, "get") == 0) {

      res.status_code = 200;
      strcpy(res.status_phrase, "OK");
      strcpy(res.header, "Content-Length");

      Get(url, connfd, res);
      return;
    }

    if (strcmp(method, "PUT") == 0 | strcmp(method, "put") == 0) {

      int offset;

      if (sscanf(parser, "%s /%s %s %s %ld %n", method, url, version,
                 res.header, &res.length, &offset) != 5) {

        res.status_code = 400;
        strcpy(res.status_phrase, "Bad Request");
        strcpy(res.header, "Content-Length");
        res.length = 12;
        sprintf(buff_res, "HTTP/1.1 %d %s\r\n%s: %ld\r\n\r\n", res.status_code,
                res.status_phrase, res.header, res.length);
        write(connfd, buff_res, strlen(buff_res));
        write(connfd, "Bad Request\n", 12);
        return;
      }

      Put(url, connfd, res, parser, offset);
      return;
    }

    if (strcmp(method, "APPEND") == 0 | strcmp(method, "append") == 0) {

      int offset;

      if (sscanf(parser, "%s /%s %s %s %ld %n", method, url, version,
                 res.header, &res.length, &offset) != 5) {

        res.status_code = 400;
        strcpy(res.status_phrase, "Bad Request");
        strcpy(res.header, "Content-Length");
        res.length = 12;
        sprintf(buff_res, "HTTP/1.1 %d %s\r\n%s: %ld\r\n\r\n", res.status_code,
                res.status_phrase, res.header, res.length);
        write(connfd, buff_res, strlen(buff_res));
        write(connfd, "Bad Request\n", 12);
        return;
      }

      Append(url, connfd, res, parser, offset);
      return;
    }

    else {
      res.status_code = 500;
      strcpy(res.status_phrase, "Not Implemented");
      strcpy(res.header, "Content-Length");
      res.length = 16;
      sprintf(buff_res, "HTTP/1.1 %d %s\r\n%s: %ld\r\n\r\n", res.status_code,
              res.status_phrase, res.header, res.length);
      write(connfd, buff_res, strlen(buff_res));
      write(connfd, "Not Implemented\n", 16);
      return;
    }
  }
}
int main(int argc, char *argv[]) {
  int listenfd;
  uint16_t port;
  if (argc != 2) {
    errx(EXIT_FAILURE, "wrong arguments: %s port_num", argv[0]);
  }
  port = strtouint16(argv[1]);
  if (port == 0) {
    errx(EXIT_FAILURE, "invalid port number: %s", argv[1]);
  }
  listenfd = create_listen_socket(port);
  signal(SIGPIPE, SIG_IGN);
  while (1) {
    int connfd = accept(listenfd, NULL, NULL);
    if (connfd < 0) {
      warn("accept error");
      continue;
    }
    handle_connection(connfd);
    // good code opens and closes objects in the same context. *sigh*
    close(connfd);
  }
  return EXIT_SUCCESS;
}
