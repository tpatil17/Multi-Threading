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



struct logbook {
    char oper[8];
    char uri[21];
    int status_code;
    int request_id;
};

struct Response {
  char version[24];
  int status_code;
  char status_phrase[100];
  char header[19];
  long length;
  char message[64];
};

struct Request {
  char method[10];

  char uri[64];

  char version[25];

  char header[50];

  char value[50];

  char *message_ptr;

  int length;

  int offset;

  int er_flg;

  int size;

  int request_id;
};

void refresh_req(struct Request *req) {

  memset(req->version, 0, 25);

  memset(req->value, 0, 50);

  memset(req->uri, 0, 64);

  memset(req->method, 0, 10);

  memset(req->header, 0, 50);

  req->message_ptr = NULL;

  return;
}

void refresh_res(struct Response *res){

    memset(res, 0, sizeof(struct Response));

    return;
}

void refresh_log(struct logbook *log){

  memset(log, 0, sizeof(struct logbook));

  return;
}

struct Request process_rquest(char read_buffer[], int connfd, int bytes_read) {

   

    char buffer[1024];

    memset(buffer, 0, sizeof(buffer));

    struct Request req;

    refresh_req(&req);

    struct Response res;

    refresh_res(&res);

    const char delim[2] = "\n";

    char *token;

    token = NULL;

    char *context;

    context = NULL;

    int total = 0;


    int trial =0;

    char header_buf[30];

    memset(header_buf ,0, 30);

    char val_buf[30];

    memset(val_buf, 0, 30);

    int check = 0;

    req.er_flg = 0;

    int ctr = 0;

    req.request_id = 0;

    req.size = bytes_read;

    token = strtok_r(read_buffer, delim, &context);

    if( token == NULL){

      req.er_flg = 2;

      return req;
    }


    strcpy(buffer, token);

    trial = strlen(token);


    // check for the valid input format

    if ((check = sscanf(buffer, "%s /%s %s %n", req.method, req.uri, req.version,
                      &req.offset)) != 3) {
        
      

        sscanf(buffer, "%s %s %s", req.method, req.uri, req.version);

        if (strcmp(req.uri, "/") == 0 && strcmp(req.version, "HTTP/1.1") == 0) {

            strcpy(res.version, "HTTP/1.1");
            res.status_code = 500;
            strcpy(res.status_phrase, "Internal Server Error");
            strcpy(res.header, "Content-Length");
            strcpy(res.message, "Internal Server Error\n");
            res.length = strlen(res.message);
            sprintf(buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
                      res.status_code, res.status_phrase, res.header, res.length,
                      res.message);
            write(connfd, buffer, strlen(buffer));

            req.er_flg = 1;

            memset(buffer, 0, 1024);
            return req;
        }

        strcpy(res.version, "HTTP/1.1");
        res.status_code = 400;
        strcpy(res.status_phrase, "Bad Request");
        strcpy(res.header, "Content-Length");
        res.length = 12;
        sprintf(buffer, "%s %d %s\r\n%s: %ld\r\n\r\n", res.version, res.status_code,
            res.status_phrase, res.header, res.length);
            write(connfd, buffer, strlen(buffer));
        write(connfd, "Bad Request\n", 12);

        req.er_flg = 1;

        memset(buffer, 0, 1024);
        return req;
    }

 // IF the method uri and version are fine check the headers now


    token = strtok_r(NULL, delim, &context);

    total = req.offset;

    while (token != NULL){

        strcpy(buffer, token);

        trial += strlen(token);

        sscanf(buffer, "%s %s %n", req.header, req.value, &req.offset);

        char temp_1[125], temp_2[125], temp_3[125];

        memset(temp_1, 0, 125);
        memset(temp_1, 0, 125);
        memset(temp_1, 0, 125);

        if (sscanf(buffer, "%s %s %s", temp_1, temp_2, temp_3) == 3) {

            strcpy(res.version, "HTTP/1.1");
            res.status_code = 400;
            strcpy(res.status_phrase, "Bad Request");
            strcpy(res.header, "Content-Length");
            res.length = 12;
            sprintf(buffer, "%s %d %s\r\n%s: %ld\r\n\r\n", res.version,
                res.status_code, res.status_phrase, res.header, res.length);
             write(connfd, buffer, strlen(buffer));
             write(connfd, "Bad Request\n", 12);

            req.er_flg = 1;

            memset(buffer, 0, 1024);
            memset(temp_1, 0, 125);
            memset(temp_2, 0, 125);
            memset(temp_3, 0, 125);

            return req;
        }

        if (strcmp(req.header, "") == 0 && strcmp(req.value, "") == 0){


            if( ((ctr == 0) && ((strcmp(req.header, "get") != 0 ))) ||((ctr == 0 )&& (strcmp(req.method, "GET") != 0))) {


                strcpy(res.version, "HTTP/1.1");
                res.status_code = 400;
                strcpy(res.status_phrase, "Bad Request");
                    strcpy(res.header, "Content-Length");
                res.length = 12;
                sprintf(buffer, "%s %d %s\r\n%s: %ld\r\n\r\n", res.version,
                    res.status_code, res.status_phrase, res.header, res.length);
                write(connfd, buffer, strlen(buffer));
                write(connfd, "Bad Request\n", 12);

                req.er_flg = 1;

                memset(buffer, 0, 1024);
                return req;
            }

            break;

        }


        if( strcmp(req.header, "Request-Id:") == 0) {

          

            
            char temp1[125], temp2[125], temp3[125];

            memset(temp1, 0, 125);
            memset(temp3, 0, 125);
            memset(temp2, 0, 125);

            int bad_flag = 0;

            if (sscanf(buffer, "%s %s %s", temp1, temp2, temp3) == 3) {

                bad_flag = 1;

                memset(temp1, 0, 125);
                memset(temp2, 0, 125);
                memset(temp3, 0, 125);
            }

            int val_ln = 0;

            val_ln = strlen(req.value);

            int i;

            for (i = 0; i < val_ln; i++) {

                if (!isdigit(req.value[i])) {

                    bad_flag = 1;
                    break;
                }
            }

            if ((strcmp(req.value, "") == 0 )|| (bad_flag == 1 )|| (strcmp(req.header, "") == 0)) {

         

                strcpy(res.version, "HTTP/1.1");
                res.status_code = 400;
                strcpy(res.status_phrase, "Bad Request");
                strcpy(res.header, "Content-Length");
                res.length = 12;
                sprintf(buffer, "%s %d %s\r\n%s: %ld\r\n\r\n", res.version,
                    res.status_code, res.status_phrase, res.header, res.length);
                write(connfd, buffer, strlen(buffer));
                write(connfd, "Bad Request\n", 12);

                req.er_flg = 1;

                memset(buffer, 0, 1024);
                return req; // bad request

            }

            req.request_id = atoi(req.value);

            
        }

        if (strcmp(req.header, "Content-Length:") == 0) {

            strcpy(header_buf, req.header);

            char temp1[125], temp2[125], temp3[125];

            memset(temp1, 0, 125);
            memset(temp2, 0, 125);
            memset(temp3, 0, 125);

            int bad_flag = 0;

            if (sscanf(buffer, "%s %s %s", temp1, temp2, temp3) == 3) {

            bad_flag = 1;

            memset(temp1, 0, 125);
            memset(temp2, 0, 125);
            memset(temp3, 0, 125);
            }

            int val_ln = 0;

            val_ln = strlen(req.value);

            int i;

            for (i = 0; i < val_ln; i++) {

                if (!isdigit(req.value[i])) {

                  bad_flag = 1;
                  break;
                }
            }

            if ((strcmp(req.value, "") == 0 )|| (bad_flag == 1)) {

              

                strcpy(res.version, "HTTP/1.1");
                res.status_code = 400;
                strcpy(res.status_phrase, "Bad Request");
                strcpy(res.header, "Content-Length");
                res.length = 12;
                sprintf(buffer, "%s %d %s\r\n%s: %ld\r\n\r\n", res.version,
                    res.status_code, res.status_phrase, res.header, res.length);
                write(connfd, buffer, strlen(buffer));
                write(connfd, "Bad Request\n", 12);

                req.er_flg = 1;

                memset(buffer, 0, 1024);
                return req; // bad request
            }

            strcpy(val_buf, req.value);
    

            if (strcmp(req.header, "") == 0) {

              

                strcpy(res.version, "HTTP/1.1");
                res.status_code = 400;
                strcpy(res.status_phrase, "Bad Request");
                strcpy(res.header, "Content-Length");
                res.length = 12;
                sprintf(buffer, "%s %d %s\r\n%s: %ld\r\n\r\n", res.version,
                     res.status_code, res.status_phrase, res.header, res.length);
                write(connfd, buffer, strlen(buffer));
                write(connfd, "Bad Request\n", 12);

                req.er_flg = 1;

                memset(buffer, 0, 1024);

                return req;
            }
        }

        strcpy(req.header, "");

        strcpy(req.value, "");

        token = strtok_r(NULL, delim, &context);

        total += req.offset;

        ctr += 1;
    }

    token = strtok_r(NULL, delim, &context);

    req.message_ptr = token;

    strcpy(req.header, header_buf);

    strcpy(req.value, val_buf);

    req.offset = trial;

    req.length = atoi(val_buf);

  

    return req;

}

// Impliment the functions 


struct Response Get(struct Request req, int connfd) {

  char resp_buf[1024];

  memset(resp_buf, 0, 1024);

  struct Response res;

  refresh_res(&res);

  strcpy(res.version, "HTTP/1.1");

  int fd;

  fd = 0;

  int bytes_read;

  bytes_read = 0;

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

  res.status_code = 200;
  strcpy(res.status_phrase, "OK");
  strcpy(res.header, "Content-Length:");
  res.length = ln.st_size;
  sprintf(resp_buf, "%s %d %s\r\n%s %ld\r\n\r\n", res.version, res.status_code, res.status_phrase, res.header, res.length);

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
//******************************************************************

// Function implementation --- PUT ---

//*******************************************************************
struct Response Put(struct Request req, int connfd, char parser[]) {

  int bytes;

  struct Response res;

  refresh_res(&res);

  int fd;

  long limit;

  res.length = req.length;

  limit = res.length;

  char resp_buffer[1024];

  memset(resp_buffer, 0, sizeof(resp_buffer));

  struct stat ln = {0};

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

  
  if ((limit <= (4096 - req.offset)) && req.message_ptr != NULL) {
    write(fd, req.message_ptr, limit);
    close(fd);
  }

  if ((limit <= (4096- req.offset)) && req.message_ptr == NULL){
    read(connfd, parser, 4096);
    write(fd, parser, limit);
    close(fd);
  }

  else {

    int total = 0;

    if(req.message_ptr != NULL){

      write(fd, req.message_ptr, 4096 - req.offset );

      total = 4096 - req.offset;

    }


    while ((bytes = read(connfd, parser, 4096)) > 0) {

      total += bytes;

      if (limit > total) {

        if (write(fd, parser, bytes) == -1 ) {

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

        if (write(fd, parser, bytes - (total - limit)) == -1  ) {

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
      memset(parser, 0, 4096);
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
  memset(parser, 0, 4096);

  return res;
}
//******************************************************************

//-------- Implementing Append function -----

//*******************************************************************
struct Response Append(struct Request req, int connfd,
            char parser[]) {

  int bytes;

  struct Response res;

  refresh_res(&res);

  int fd;

  long limit;

  res.length = req.length;

  limit = res.length;

  struct stat ln = {0};

  char resp_buffer[1024];

  memset(resp_buffer, 0, sizeof(resp_buffer));


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

  if ((limit <= (4096 - req.offset)) && req.message_ptr != NULL) {
    write(fd, req.message_ptr, limit);
    close(fd);

  } 
  if((limit <= (4096 - req.offset)) && req.message_ptr == NULL){

    read(fd, parser, 4096);
    write(fd, parser, limit);
    close(fd);

  }
  
  else {
    int total = 0;

    if(req.message_ptr != NULL){

      write(fd, req.message_ptr, 4096 - req.offset);

      total = 4096 - req.offset;

    }


    while ((bytes = read(connfd, parser, 4096)) > 0) {

      total += bytes;

      if (limit > total) {
        if (write(fd, parser, bytes) == -1) {

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

        if (write(fd, parser, bytes - (total - limit)) == -1) {

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
      memset(parser, 0, 4096);
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

    memset(parser, 0, 4096);

    close(fd);
  }

  sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
          res.status_code, res.status_phrase, res.header, res.length,
          res.message);

  write(connfd, resp_buffer, strlen(resp_buffer));

  memset(resp_buffer, 0, 1024);
  memset(parser, 0, 4096);

  return res;
}

// the log file

void enter_log(struct logbook data, FILE *l_file ){

    char buf[1024];

    memset(buf, 0, sizeof(buf));

    sprintf(buf, "%s,/%s,%d,%d\n", data.oper, data.uri, data.status_code, data.request_id);

    fwrite(buf, 1, strlen(buf),l_file);

    fflush(l_file);

    return;
}

void clean_entry(struct logbook *data){

    memset(data, 0, sizeof(struct logbook));

    return;
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

    ssize_t bytes_read = 0;

    struct Request req;

    refresh_req(&req);

    struct Response res_return;

    refresh_res(&res_return);

    struct logbook data;

    refresh_log(&data);

    
    do {
        // Read from connfd until EOF or error.
        bytes_read = read(connfd, buf, sizeof(buf));

        buf[bytes_read] = '\0'
      
        if (bytes_read <= 0) {
            return;
        }

        req = process_rquest(buf, connfd, bytes_read);

        if(req.er_flg == 2){

          memset(buf, 0, sizeof(buf));
          
          return;
        }

        if (req.er_flg == 1) {

          refresh_req(&req);

          memset(buf, 0, sizeof(buf));

          return;
        }

        if((strcmp(req.method, "GET") == 0 )||( strcmp(req.method, "get") == 0)){

            res_return = Get(req, connfd);

            strcpy(data.oper, req.method);
            data.request_id = req.request_id;
            data.status_code = res_return.status_code;
            strcpy(data.uri, req.uri);
            enter_log(data, logfile);
            refresh_res(&res_return);

            clean_entry(&data);

            return;

        }

        if((strcmp(req.method, "PUT") == 0)||( strcmp(req.method, "put") == 0)){
            
            res_return = Put(req, connfd, buf);
            strcpy(data.oper, req.method);
            data.request_id = req.request_id;
            data.status_code = res_return.status_code;
            strcpy(data.uri, req.uri);
            enter_log(data, logfile);
            refresh_res(&res_return);

            clean_entry(&data);

            return;

        }

        if((strcmp(req.method, "APPEND") == 0 )|| (strcmp(req.method, "append") == 0)){

           
            res_return = Append(req, connfd, buf);
            strcpy(data.oper, req.method);
            data.request_id = req.request_id;
            data.status_code = res_return.status_code;
            strcpy(data.uri, req.uri);
            enter_log(data, logfile);

            refresh_res(&res_return);

            clean_entry(&data);

            return;

        }

    memset(buf, 0, BUF_SIZE);
  } while (bytes_read > 0);
}

static void sigterm_handler(int sig) {
    if (sig == SIGTERM) {
      fflush(logfile);
      fclose(logfile);
      exit(EXIT_SUCCESS);
    }
    if (sig == SIGINT){
      fflush(logfile);
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
    signal(SIGINT, sigterm_handler);

    int listenfd = create_listen_socket(port);
    //LOG("port=%" PRIu16 ", threads=%d\n", port, threads);

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
