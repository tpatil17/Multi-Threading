#include <assert.h>
#include <ctype.h>
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
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        err(EXIT_FAILURE, "bind error");
    }
    if (listen(listenfd, 500) < 0) {
        err(EXIT_FAILURE, "listen error");
    }
    return listenfd;
}
/* Structs required for precessing requests and producing responses*/

// ******************************************************************
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

    int length;

    int offset;

    int er_flg;

    int size;
};

void refresh(struct Request req) {

    memset(req.version, 0, 25);

    memset(req.value, 0, 50);

    memset(req.uri, 0, 64);

    memset(req.method, 0, 10);

    memset(req.header, 0, 50);

    return;
}
// *********************** Request Processing *********************

struct Request process_request(char read_buffer[], int connfd) {

    char buffer[1024];

    struct Request req;

    memset(req.method, 0, 10);

    memset(req.uri, 0, 64);

    memset(req.value, 0, 50);

    memset(req.version, 0, 15);

    memset(req.header, 0, 50);

    struct Response res;

    const char delim[2] = "\n";

    char *token;

    int total = 0;

    char perm_header[25];

    char perm_val[24];

    int check;

    req.er_flg = 0;

    int ctr = 0;

    req.size = strlen(read_buffer);

    token = strtok(read_buffer, delim);

    strcpy(buffer, token);

    if ((check = sscanf(buffer, "%s /%s %s %n", req.method, req.uri, req.version, &req.offset))
        != 3) {

        memset(req.version, 0, 25);
        memset(req.uri, 0, 64);
        memset(req.method, 0, 10);

        sscanf(buffer, "%s %s %s", req.method, req.uri, req.version);

        if (strcmp(req.uri, "/") == 0 && strcmp(req.version, "HTTP/1.1") == 0) {

            strcpy(res.version, "HTTP/1.1");
            res.status_code = 500;
            strcpy(res.status_phrase, "Internal Server Error");
            strcpy(res.header, "Content-Length");
            strcpy(res.message, "Internal Server Error\n");
            res.length = strlen(res.message);
            sprintf(buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
                res.status_phrase, res.header, res.length, res.message);
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

    token = strtok(NULL, delim);

    total = req.offset;

    while (token != NULL) {

        strcpy(buffer, token);

        sscanf(buffer, "%s %s %n", req.header, req.value, &req.offset);

        char temp_1[25], temp_2[25], temp_3[25];

        if (sscanf(buffer, "%s %s %s", temp_1, temp_2, temp_3) == 3) {

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
            memset(temp_1, 0, 25);
            memset(temp_2, 0, 25);
            memset(temp_3, 0, 25);

            return req;
        }

        if (strcmp(req.header, "") == 0 && strcmp(req.value, "") == 0) {

            if (ctr == 0 && (strcmp(req.method, "GET") != 0 | strcmp(req.method, "get") != 0)) {

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

            break;
        }

        if (strcmp(req.header, "Content-Length:") == 0) {

            strcpy(perm_header, req.header);

            char temp1[25], temp2[25], temp3[25];

            int bad_flag = 0;

            if (sscanf(buffer, "%s %s %s", temp1, temp2, temp3) == 3) {

                bad_flag = 1;

                memset(temp1, 0, 25);
                memset(temp2, 0, 25);
                memset(temp3, 0, 25);
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

            if (strcmp(req.value, "") == 0 | bad_flag == 1) {
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
                return req; // bad request
            }

            strcpy(perm_val, req.value);
        }

        if (strcmp(req.header, "") == 0) {

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

        strcpy(req.header, "");

        strcpy(req.value, "");

        token = strtok(NULL, delim);

        total += req.offset;

        ctr += 1;
    }

    strcpy(req.header, perm_header);

    strcpy(req.value, perm_val);

    req.offset = total + 8;

    if (ctr == 1) {

        req.offset -= 4;
    }

    req.length = atoi(perm_val);

    return req;
}

//******************************************************************

// Function implementations -- GET --

// *****************************************************************

void Get(struct Request req, int connfd, struct Response res) {

    char resp_buf[1024];

    int fd;

    int bytes_read;

    struct stat ln;

    stat(req.uri, &ln);

    if (access(req.uri, F_OK) != 0) {

        res.status_code = 404;
        strcpy(res.status_phrase, "File Not Found");
        strcpy(res.header, "Content-Length");
        strcpy(res.message, "File Not Found\n");
        res.length = strlen(res.message);
        sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
            res.status_phrase, res.header, res.length, res.message);
        write(connfd, resp_buf, strlen(resp_buf));

        memset(resp_buf, 0, 1024);

        refresh(req);

        return;
    }

    if (S_ISREG(ln.st_mode) == 0) {

        res.status_code = 403;
        strcpy(res.status_phrase, "Forbidden");
        strcpy(res.header, "Content-Length");
        strcpy(res.message, "Forbidden\n");
        res.length = strlen(res.message);
        sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
            res.status_phrase, res.header, res.length, res.message);
        write(connfd, resp_buf, strlen(resp_buf));

        memset(resp_buf, 0, 1024);

        refresh(req);

        return;
    }

    if (access(req.uri, R_OK) != 0) {

        res.status_code = 403;
        strcpy(res.status_phrase, "Forbidden");
        strcpy(res.header, "Content-Length");
        strcpy(res.message, "Forbidden\n");
        res.length = strlen(res.message);
        sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
            res.status_phrase, res.header, res.length, res.message);
        write(connfd, resp_buf, strlen(resp_buf));

        memset(resp_buf, 0, 1024);

        refresh(req);

        return;
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
            sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
                res.status_phrase, res.header, res.length, res.message);
            write(connfd, resp_buf, strlen(resp_buf));
            close(fd);

            memset(resp_buf, 0, 1024);

            refresh(req);

            return;
        }
    }

    if (bytes_read == -1) {
        res.status_code = 500;
        strcpy(res.status_phrase, "Internal Server Error");
        strcpy(res.header, "Content-Length");
        strcpy(res.message, "Internal Server Error\n");
        res.length = strlen(res.message);
        sprintf(resp_buf, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
            res.status_phrase, res.header, res.length, res.message);
        write(connfd, resp_buf, strlen(resp_buf));
        close(fd);
        memset(resp_buf, 0, 1024);

        refresh(req);

        return;
    }

    memset(resp_buf, 0, 1024);

    close(fd);
    return;
}
//******************************************************************

// Function implementation --- PUT ---

//*******************************************************************
void Put(struct Request req, int connfd, struct Response res, char parser[]) {

    int bytes;

    int fd;

    long limit;

    res.length = req.length;

    limit = res.length;

    char resp_buffer[1024];

    struct stat ln;

    stat(req.uri, &ln);

    if (S_ISDIR(ln.st_mode) != 0) {

        res.status_code = 403;
        strcpy(res.status_phrase, "Forbidden");
        strcpy(res.header, "Content-Length");
        strcpy(res.message, "Forbidden\n");
        res.length = strlen(res.message);
        sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
            res.status_phrase, res.header, res.length, res.message);
        write(connfd, resp_buffer, strlen(resp_buffer));

        memset(resp_buffer, 0, 1024);

        refresh(req);

        return;
    }

    if (access(req.uri, F_OK) == 0) {

        if (S_ISREG(ln.st_mode) == 0 | access(req.uri, W_OK) != 0) {

            res.status_code = 403;
            strcpy(res.status_phrase, "Forbidden");
            strcpy(res.header, "Content-Length");
            strcpy(res.message, "Forbidden\n");
            res.length = strlen(res.message);
            sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
                res.status_phrase, res.header, res.length, res.message);
            write(connfd, resp_buffer, strlen(resp_buffer));

            memset(resp_buffer, 0, 1024);

            refresh(req);

            return;
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

    if (limit < (4096 - req.offset)) {
        write(fd, parser + req.offset, limit);
        close(fd);
    }

    else {

        int total = 0;

        write(fd, parser + req.offset, req.size - req.offset);

        total = req.size - req.offset;

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
                        res.status_code, res.status_phrase, res.header, res.length, res.message);
                    write(connfd, resp_buffer, strlen(resp_buffer));
                    close(fd);

                    memset(resp_buffer, 0, 1024);

                    refresh(req);

                    return;
                }
            }

            if (total >= limit) {

                if (write(fd, parser, bytes - (total - limit)) == -1) {

                    res.status_code = 500;
                    strcpy(res.status_phrase, "Internal Server Error");
                    strcpy(res.header, "Content-Length");
                    strcpy(res.message, "Internal Server Error\n");
                    res.length = strlen(res.message);
                    sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version,
                        res.status_code, res.status_phrase, res.header, res.length, res.message);
                    write(connfd, resp_buffer, strlen(resp_buffer));
                    close(fd);

                    memset(resp_buffer, 0, 1024);

                    refresh(req);

                    return;
                }

                break;
            }
        }

        if (bytes == -1) {
            res.status_code = 500;
            strcpy(res.status_phrase, "Internal Server Error");
            strcpy(res.header, "Content-Length");
            strcpy(res.message, "Internal Server Error\n");
            res.length = strlen(res.message);
            sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
                res.status_phrase, res.header, res.length, res.message);
            write(connfd, resp_buffer, strlen(resp_buffer));
            close(fd);

            memset(resp_buffer, 0, 1024);

            refresh(req);

            return;
        }

        close(fd);
    }

    // Write the formal response

    sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
        res.status_phrase, res.header, res.length, res.message);
    write(connfd, resp_buffer, strlen(resp_buffer));

    memset(resp_buffer, 0, 1024);
    memset(parser, 0, 4096);

    return;
}
//******************************************************************

//-------- Implementing Append function -----

//*******************************************************************
void Append(struct Request req, int connfd, struct Response res, char parser[]) {

    int bytes;

    int fd;

    long limit;

    res.length = req.length;

    limit = res.length;

    struct stat ln;

    char resp_buffer[1024];

    if (access(req.uri, F_OK) != 0) {

        res.status_code = 404;
        strcpy(res.status_phrase, "File Not Found");
        res.length = 15;
        strcpy(res.header, "Content-Length");
        strcpy(res.version, "HTTP/1.1");
        strcpy(res.message, "File Not Found\n");
        sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
            res.status_phrase, res.header, res.length, res.message);

        write(connfd, resp_buffer, strlen(resp_buffer));

        memset(resp_buffer, 0, 1024);

        refresh(req);

        return;
    }

    stat(req.uri, &ln);

    if (S_ISREG(ln.st_mode) == 0) {
        res.status_code = 403;
        strcpy(res.status_phrase, "Forbidden");
        strcpy(res.header, "Content-Length");
        strcpy(res.message, "Forbidden\n");
        res.length = strlen(res.message);
        sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
            res.status_phrase, res.header, res.length, res.message);
        write(connfd, resp_buffer, strlen(resp_buffer));

        memset(resp_buffer, 0, 1024);

        refresh(req);

        return;
    }
    if (access(req.uri, W_OK) != 0) {
        res.status_code = 403;
        strcpy(res.status_phrase, "Forbidden");
        strcpy(res.header, "Content-Length");
        strcpy(res.message, "Forbidden\n");
        res.length = strlen(res.message);
        sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
            res.status_phrase, res.header, res.length, res.message);
        write(connfd, resp_buffer, strlen(resp_buffer));

        memset(resp_buffer, 0, 1024);

        refresh(req);

        return;
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

    if (limit < (4096 - req.offset)) {
        write(fd, parser + req.offset, limit);
        close(fd);

    } else {
        int total = 0;

        write(fd, parser + req.offset, req.size - req.offset);

        total = req.size - req.offset;

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
                        res.status_code, res.status_phrase, res.header, res.length, res.message);
                    write(connfd, resp_buffer, strlen(resp_buffer));
                    close(fd);
                    memset(resp_buffer, 0, 1024);

                    refresh(req);

                    return;
                }

            } else {

                if (write(fd, parser, bytes - (total - limit)) == -1) {

                    res.status_code = 500;
                    strcpy(res.status_phrase, "Internal Server Error");
                    strcpy(res.header, "Content-Length");
                    strcpy(res.message, "Internal Server Error\n");
                    res.length = strlen(res.message);
                    sprintf(resp_buffer, "%s %d %s\r\n%s:% ld\r\n\r\n%s", res.version,
                        res.status_code, res.status_phrase, res.header, res.length, res.message);
                    write(connfd, resp_buffer, strlen(resp_buffer));
                    close(fd);
                    memset(resp_buffer, 0, 1024);

                    refresh(req);

                    return;
                }
                break;
            }
        }
        if (bytes == -1) {
            res.status_code = 500;
            strcpy(res.status_phrase, "Internal Server Error");
            strcpy(res.header, "Content-Length");
            strcpy(res.message, "Internal Server Error\n");
            res.length = strlen(res.message);
            sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
                res.status_phrase, res.header, res.length, res.message);
            write(connfd, resp_buffer, strlen(resp_buffer));
            close(fd);
            memset(resp_buffer, 0, 1024);

            refresh(req);

            return;
        }

        close(fd);
    }

    sprintf(resp_buffer, "%s %d %s\r\n%s: %ld\r\n\r\n%s", res.version, res.status_code,
        res.status_phrase, res.header, res.length, res.message);

    write(connfd, resp_buffer, strlen(resp_buffer));

    memset(resp_buffer, 0, 1024);
    memset(parser, 0, 4096);

    return;
}

//*************************************************************

// ---- Handle Connection --------------------------------

//*************************************************************

void handle_connection(int connfd) {
    // make the compiler not complain
    char parser[4096];

    struct Response res;

    struct Request req;

    // get the method
    read(connfd, parser, 4096);

    req = process_request(parser, connfd);

    if (req.er_flg == 1) {

        refresh(req);

        memset(parser, 0, 4096);

        return;
    }

    char buff_res[1024];

    strcpy(res.version, "HTTP/1.1");

    if (strcmp(req.method, "GET") == 0 | strcmp(req.method, "get") == 0) {

        res.status_code = 200;
        strcpy(res.status_phrase, "OK");
        strcpy(res.header, "Content-Length");
        strcpy(res.version, req.version);

        Get(req, connfd, res);
        refresh(req);
        return;
    }

    if (strcmp(req.method, "PUT") == 0 | strcmp(req.method, "put") == 0) {

        strcpy(res.version, req.version);

        Put(req, connfd, res, parser);
        refresh(req);
        return;
    }

    if (strcmp(req.method, "APPEND") == 0 | strcmp(req.method, "append") == 0) {

        strcpy(res.version, req.version);

        Append(req, connfd, res, parser);
        refresh(req);
        return;
    }

    else {
        res.status_code = 500;
        strcpy(res.status_phrase, "Not Implemented");
        strcpy(res.header, "Content-Length");
        res.length = 16;
        sprintf(buff_res, "HTTP/1.1 %d %s\r\n%s: %ld\r\n\r\n", res.status_code, res.status_phrase,
            res.header, res.length);
        write(connfd, buff_res, strlen(buff_res));
        write(connfd, "Not Implemented\n", 16);
        memset(buff_res, 0, 1024);

        refresh(req);

        memset(parser, 0, 4096);

        return;
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
