#define _XOPEN_SOURCE 700

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "client.h"
#include "log.h"

char * status_command = "userCount";
char * users_count_command = "userCount";
char * check_connection_command = "check connection";

char * get_status(int port, char * host) {
  char * res = run_command(port, host, status_command);
  if (res == NULL) {
	return "DOWN";
  }
  free(res);
  return "UP";
}

char * get_connection(int port, char * host) {
  char * res = run_command(port,host,check_connection_command);
  if (res == NULL) {
	return "NO";
  }
  return res;
}

int get_users_count(int port, char * host) {
  char * res = run_command(port,host,users_count_command);
  if (res == NULL) {
	return 0;
  }
  int result = strtol(res,NULL,10);
  free(res);
  return result;
}

char * run_command(int port, char * host, char * command) {
    char * buffer = calloc(1, BUFSIZ);
    char protoname[] = "tcp";
    struct protoent *protoent;
    char *server_hostname = host;
    in_addr_t in_addr;
    in_addr_t server_addr;
    int sockfd;
    size_t getline_buffer = 0;
    ssize_t nbytes_read;
    struct hostent *hostent;
    /* This is the struct used by INet addresses. */
    struct sockaddr_in sockaddr_in;
    unsigned short server_port = port;
    char command_limited[1024];
    int len = strlen(command);

    strcpy(command_limited, command);
    protoent = getprotobyname(protoname);
    if (protoent == NULL) {
        perror("getprotobyname");
        return NULL;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (sockfd == -1) {
        perror("socket");
        return NULL;
    }
        if (sockfd == -1) {
        perror("socket");
        return NULL;
    }

    /* Prepare sockaddr_in. */
    hostent = gethostbyname(server_hostname);
    if (hostent == NULL) {
	log_error("error: gethostbyname(\"%s\")\n", server_hostname);
        return NULL;
    }
    in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
    if (in_addr == (in_addr_t)-1) {
	log_error("error: inet_addr(\"%s\")\n", server_hostname);
        return NULL;
    }
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    /* Do the actual connection. */
    if (connect(sockfd, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
	log_error("Could not connect to %s in port %d", server_hostname, server_port);
        perror("connect");
        return NULL;
    }
    if (write(sockfd, command, strlen(command)) == -1) {
	    log_error("Could not write to %s in port %d", server_hostname, server_port);
            perror("write");
            return NULL;
    }
    nbytes_read = read(sockfd, buffer, BUFSIZ);
    if (nbytes_read <= 0) {
	log_error("Could not read from %s in port %d", server_hostname, server_port);
        perror("read");
        return NULL;
    }
    close(sockfd);
    return buffer;
}
