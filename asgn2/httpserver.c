
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#define OPTIONS              "t:l:"
#define BUF_SIZE             4096
#define DEFAULT_THREAD_COUNT 4

static FILE *logfile;
#define LOG(...) fprintf(logfile, __VA_ARGS__);

//-------------------------------------------------------------------------



struct Response {
    char method[8];

    char uri[19];
    char status_phrase[100];
    
    char version[25];
    int status_code;

    char header[100];

    long length;

    char message[64];
};

void refresh_res(struct Response *res){

    memset(res, 0, sizeof(struct Response));
    return;
}

struct Request {

    char method[8];
    
    char uri[19];
    char version[25];

    int request_id;

    int content_length;

    int status_code;

    int err_flag;

    char *message;

};

void refresh_req(struct Request *req){

    memset(req, 0, sizeof(struct Request));

    return;
}

void enter_log(struct Request req) {

    char buffer[1024];

    memset(buffer, 0, 1024);

    sprintf(buffer, "%s,/%s,%d,%d", req.method, req.uri, req.status_code, req.request_id);

    fwrite(buffer, 1, strlen(buffer), logfile);

    fflush(logfile);

    return;
}

struct Request process_request(char read_buffer[]){

    char store_token[1024];

    memset(store_token, 0, sizeof(store_token));

    struct  Request req;

    refresh_req(&req);

    req.request_id = 0;

    const char delim[2] = "\n";

    char *token;

    char *context;

    char header[64];

    memset(header, 0, sizeof(header));

    char value[64];

    memset(value, 0, sizeof(value));


    token = strtok_r(read_buffer, delim, &context);
    
    strcpy(store_token, token);

    if(token == NULL){

        req.err_flag = 2; // nc -zv do not wirte to log just return

        return req;

    }

    sscanf(store_token,"%s /%s %s", req.method, req.uri, req.version);

    token = strtok_r(NULL, delim, &context);

    while(token != NULL){

        memset(store_token, 0, sizeof(store_token));

        strcpy(store_token, token);

        sscanf(store_token, "%s %s", header, value);

        if(strcmp(header, "") == 0 && strcmp(value, "") == 0){
            token = strtok_r(NULL, delim, &context);
            break;
        }

        if(strcmp("Request-Id:", header) == 0){

            req.request_id = atoi(value);
        }
        if(strcmp("Content_length:", header) == 0){

            req.content_length = atoi(value);

        }

        token = strtok_r(NULL, delim, &context);
        memset(header, 0, sizeof(header));
        memset(value, 0, sizeof(value));

    }

    req.message = token;

    return req;
}

struct Response Get(struct Request req, int connfd){

  char resp_buf[1024];

  memset(resp_buf, 0, sizeof(resp_buf));

  struct Response res;

  refresh_res(&res);

  int fd;

  int bytes_read;

  struct stat ln = {0};

  stat(req.uri, &ln);

  if (access(req.uri, F_OK) != 0) {

    res.status_code = 404;
    strcpy(res.status_phrase, "File Not Found");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "File Not Found\n");
    res.length = strlen(res.message);
    sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buf, strlen(resp_buf));

    memset(resp_buf, 0, 1024);


    return res;
  }

  if (S_ISREG(ln.st_mode) == 0) {

    res.status_code = 403;
    strcpy(res.status_phrase, "Forbidden");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "Forbidden\n");
    res.length = strlen(res.message);
    sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buf, strlen(resp_buf));

    memset(resp_buf, 0, 1024);


    return res;
  }

  if (access(req.uri, R_OK) != 0) {

    res.status_code = 403;
    strcpy(res.status_phrase, "Forbidden");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "Forbidden\n");
    res.length = strlen(res.message);
    sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buf, strlen(resp_buf));

    memset(resp_buf, 0, 1024);


    return res;
  }

  fd = open(req.uri, O_RDONLY);

  if (fd == -1) {

    printf("File opening problem %d\n", errno);
    exit(1);
  }

  res.length = ln.st_size;

  sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n", res.version, res.status_code,
          res.status_phrase, res.header, res.length);

  write(connfd, resp_buf, strlen(resp_buf));

  while ((bytes_read = read(fd, resp_buf, 1024)) > 0) {

    if ((write(connfd, resp_buf, bytes_read)) == -1) {
      res.status_code = 500;
      strcpy(res.status_phrase, "Internal Server Error");
      strcpy(res.header, "Content-Length");
      strcpy(res.message, "Internal Server Error\n");
      res.length = strlen(res.message);
      sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
              res.status_code, res.status_phrase, res.header, res.length,
              res.message);
      write(connfd, resp_buf, strlen(resp_buf));
      close(fd);

      memset(resp_buf, 0, 1024);


      return res;
    }
  }

  if (bytes_read == -1) {
    res.status_code = 500;
    strcpy(res.status_phrase, "Internal Server Error");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "Internal Server Error\n");
    res.length = strlen(res.message);
    sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buf, strlen(resp_buf));
    close(fd);
    memset(resp_buf, 0, 1024);


    return res;
  }

  memset(resp_buf, 0, 1024);

  close(fd);
  return res;

}

struct Response Put(struct Request req, int connfd){


  int bytes;

  struct Response res;

  refresh_res(&res);

  int fd;

  long limit;

  res.length = req.content_length;

  limit = res.length;

  char resp_buffer[1024];

  memset(resp_buffer, 0, sizeof(resp_buffer));

  struct stat ln = {0};

  char message[4096];

  memset(message, 0, sizeof(message));

  strcpy(message, req.message);

  stat(req.uri, &ln);

  if (S_ISDIR(ln.st_mode) != 0) {

    res.status_code = 403;
    strcpy(res.status_phrase, "Forbidden");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "Forbidden\n");
    res.length = strlen(res.message);
    sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buffer, strlen(resp_buffer));

    memset(resp_buffer, 0, 1024);


    return res;
  }

  if (access(req.uri, F_OK) == 0) {

    if (S_ISREG(ln.st_mode) == 0 | access(req.uri, W_OK) != 0) {

      res.status_code = 403;
      strcpy(res.status_phrase, "Forbidden");
      strcpy(res.header, "Content-Length");
      strcpy(res.message, "Forbidden\n");
      res.length = strlen(res.message);
      sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
              res.status_code, res.status_phrase, res.header, res.length,
              res.message);
      write(connfd, resp_buffer, strlen(resp_buffer));

      memset(resp_buffer, 0, 1024);


      return res;
    }

    fd = open(req.uri, O_WRONLY | O_TRUNC);

    res.status_code = 200;
    strcpy(res.status_phrase, "OK");
    res.length = 3;
    strcpy(res.header, "Content-Length");
    strcpy(res.version, "HTTP/1.1");
    strcpy(res.message, "OK\n");
  }

  if (access(req.uri, F_OK) != 0) {

    fd = open(req.uri, O_WRONLY | O_CREAT, 0644);

    res.status_code = 201;
    strcpy(res.status_phrase, "Created");
    res.length = 8;
    strcpy(res.header, "Content-Length");
    strcpy(res.version, "HTTP/1.1");
    strcpy(res.message, "Created\n");
  }

  if (limit < 4096) {
    write(fd, message, limit);
    close(fd);
  }

  else {

    int total = 0;

    write(fd, message, strlen(message));

    total = strlen(message);

    while ((bytes = read(connfd, message, 4096)) > 0) {

      total += bytes;

      if (limit > total) {

        if (write(fd, message, bytes) == -1) {

          res.status_code = 500;
          strcpy(res.status_phrase, "Internal Server Error");
          strcpy(res.header, "Content-Length");
          strcpy(res.message, "Internal Server Error\n");
          res.length = strlen(res.message);
          sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
                  res.status_code, res.status_phrase, res.header, res.length,
                  res.message);
          write(connfd, resp_buffer, strlen(resp_buffer));
          close(fd);

          memset(resp_buffer, 0, 1024);



          return res;
        }
      }

      if (total >= limit) {

        if (write(fd, message, bytes - (total - limit)) == -1) {

          res.status_code = 500;
          strcpy(res.status_phrase, "Internal Server Error");
          strcpy(res.header, "Content-Length");
          strcpy(res.message, "Internal Server Error\n");
          res.length = strlen(res.message);
          sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
                  res.status_code, res.status_phrase, res.header, res.length,
                  res.message);
          write(connfd, resp_buffer, strlen(resp_buffer));
          close(fd);

          memset(resp_buffer, 0, 1024);

   

          return res;
        }

        break;
      }
      memset(message, 0, strlen(message));
    }

    if (bytes == -1) {
      res.status_code = 500;
      strcpy(res.status_phrase, "Internal Server Error");
      strcpy(res.header, "Content-Length");
      strcpy(res.message, "Internal Server Error\n");
      res.length = strlen(res.message);
      sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
              res.status_code, res.status_phrase, res.header, res.length,
              res.message);
      write(connfd, resp_buffer, strlen(resp_buffer));
      close(fd);

      memset(resp_buffer, 0, 1024);


      return res;
    }

    close(fd);
  }

  // Write the formal response

  sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
          res.status_code, res.status_phrase, res.header, res.length,
          res.message);
  write(connfd, resp_buffer, strlen(resp_buffer));

  memset(resp_buffer, 0, 1024);
  memset(message, 0, 4096);

  return res;
}

struct Response Append(struct Request req, int connfd) {

  int bytes;

  struct Response res;

  refresh_res(&res);

  int fd;

  long limit;

  res.length = req.content_length;

  limit = res.length;

  struct stat ln = {0};

  char resp_buffer[1024];

  memset(resp_buffer, 0, sizeof(resp_buffer));

  char message[4096];

  memset(message, 0, 4096);

  strcpy(message, req.message);


  if (access(req.uri, F_OK) != 0) {

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

    memset(resp_buffer, 0, 1024);


    return res;
  }

  stat(req.uri, &ln);

  if (S_ISREG(ln.st_mode) == 0) {
    res.status_code = 403;
    strcpy(res.status_phrase, "Forbidden");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "Forbidden\n");
    res.length = strlen(res.message);
    sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buffer, strlen(resp_buffer));

    memset(resp_buffer, 0, 1024);

    return res;
  }
  if (access(req.uri, W_OK) != 0) {
    res.status_code = 403;
    strcpy(res.status_phrase, "Forbidden");
    strcpy(res.header, "Content-Length");
    strcpy(res.message, "Forbidden\n");
    res.length = strlen(res.message);
    sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
            res.status_code, res.status_phrase, res.header, res.length,
            res.message);
    write(connfd, resp_buffer, strlen(resp_buffer));

    memset(resp_buffer, 0, 1024);

    return res;
  }

  fd = open(req.uri, O_WRONLY | O_APPEND);

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

  long len = 0;

  len = strlen(message);

  if (limit < len) {
    write(fd, message, limit);
    close(fd);

  } else {
    int total = 0;

    write(fd, message, strlen(message));

    total = strlen(message);

    while ((bytes = read(connfd, message, 4096)) > 0) {

      total += bytes;

      if (limit > total) {
        if (write(fd, message, bytes) == -1) {

          res.status_code = 500;
          strcpy(res.status_phrase, "Internal Server Error");
          strcpy(res.header, "Content-Length");
          strcpy(res.message, "Internal Server Error\n");
          res.length = strlen(res.message);
          sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
                  res.status_code, res.status_phrase, res.header, res.length,
                  res.message);
          write(connfd, resp_buffer, strlen(resp_buffer));
          close(fd);
          memset(resp_buffer, 0, 1024);

          return res;
        }

      } else {

        if (write(fd, message, bytes - (total - limit)) == -1) {

          res.status_code = 500;
          strcpy(res.status_phrase, "Internal Server Error");
          strcpy(res.header, "Content-Length");
          strcpy(res.message, "Internal Server Error\n");
          res.length = strlen(res.message);
          sprintf(resp_buffer, "%s %d %s\r\n%s:% ld\r\n\r\n%s", res.version,
                  res.status_code, res.status_phrase, res.header, res.length,
                  res.message);
          write(connfd, resp_buffer, strlen(resp_buffer));
          close(fd);
          memset(resp_buffer, 0, 1024);

          return res;
        }
        break;
      }
      memset(message, 0, 4096);
    }
    if (bytes == -1) {
      res.status_code = 500;
      strcpy(res.status_phrase, "Internal Server Error");
      strcpy(res.header, "Content-Length");
      strcpy(res.message, "Internal Server Error\n");
      res.length = strlen(res.message);
      sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
              res.status_code, res.status_phrase, res.header, res.length,
              res.message);
      write(connfd, resp_buffer, strlen(resp_buffer));
      close(fd);
      memset(resp_buffer, 0, 1024);


      return res;
    }

    memset(message, 0, 4096);

    close(fd);
  }

  sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
          res.status_code, res.status_phrase, res.header, res.length,
          res.message);

  write(connfd, resp_buffer, strlen(resp_buffer));

  memset(resp_buffer, 0, 1024);
  memset(message, 0, 4096);

  return res;
}



//==========================================================================
// Converts a string to an 16 bits unsigned integer.
// Returns 0 if the string is malformed or out of the range.
static size_t strtouint16(char number[]) {
    char *last;
    long num = strtol(number, &last, 10);
    if (num <= 0 || num > UINT16_MAX || *last != '\0') {
        return 0;
    }
    return num;
}

// Creates a socket for listening for connections.
// Closes the program and prints an error message on error.
static int create_listen_socket(uint16_t port) {
    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err(EXIT_FAILURE, "socket error");
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        err(EXIT_FAILURE, "bind error");
    }
    if (listen(listenfd, 128) < 0) {
        err(EXIT_FAILURE, "listen error");
    }
    return listenfd;
}

static void handle_connection(int connfd) {
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);

    struct Request req;

    refresh_req(&req);

    struct Response res_return;

    refresh_res(&res_return);

    ssize_t bytes_read =0;

    //do {
        // Read from connfd until EOF or error.
        bytes_read = read(connfd, buf, sizeof(buf));
        if (bytes_read < 0) {
            return;
        }



        req = process_request(buf);

        if(req.err_flag == 2){

          memset(buf, 0, sizeof(buf));
          
          return;
        }

        if (req.err_flag == 1) {

          refresh_req(&req);

          memset(buf, 0, sizeof(buf));

          return;
        }

        if(strcmp(req.method, "GET") == 0 | strcmp(req.method, "get") == 0){

            res_return = Get(req, connfd);
            req.status_code = res_return.status_code;
            enter_log(req);
            refresh_res(&res_return);
            return;

        }

        if(strcmp(req.method, "PUT") == 0 | strcmp(req.method, "put") == 0){
            
            res_return = Put(req, connfd);
          
            req.status_code = res_return.status_code;
           
            enter_log(req);
            refresh_res(&res_return);

            return;

        }

        if(strcmp(req.method, "APPEND") == 0 | strcmp(req.method, "append") == 0){

           
            res_return = Append(req, connfd);
          
            req.status_code = res_return.status_code;

            enter_log(req);

            refresh_res(&res_return);

            return;

        }



        memset(buf, 0, BUF_SIZE);


    //} while (bytes_read > 0);
}

static void sigterm_handler(int sig) {
    if (sig == SIGTERM) {
        warnx("received SIGTERM");
        fclose(logfile);
        exit(EXIT_SUCCESS);
    }
    if (sig == SIGINT){
        fclose(logfile);
        exit(EXIT_SUCCESS);
    }
}

static void usage(char *exec) {
    fprintf(stderr, "usage: %s [-t threads] [-l logfile] <port>\n", exec);
}

int main(int argc, char *argv[]) {
    int opt = 0;
    int threads = DEFAULT_THREAD_COUNT;
    logfile = stderr;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, NULL, 10);
            if (threads <= 0) {
                errx(EXIT_FAILURE, "bad number of threads");
            }
            break;
        case 'l':
            logfile = fopen(optarg, "w");
            if (!logfile) {
                errx(EXIT_FAILURE, "bad logfile");
            }
            break;
        default: usage(argv[0]); return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        warnx("wrong number of arguments");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t port = strtouint16(argv[optind]);
    if (port == 0) {
        errx(EXIT_FAILURE, "bad port number: %s", argv[1]);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler );

    int listenfd = create_listen_socket(port);
    LOG("port=%" PRIu16 ", threads=%d\n", port, threads);

    for (;;) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            warn("accept error");
            continue;
        }
        handle_connection(connfd);
        close(connfd);
    }

    return EXIT_SUCCESS;
}

// ===================================== Written By me ==================================================
