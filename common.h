#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
/*
	scoket();
	bind();
	recv();
	send();
	inet_aton();
	inet_ntoa();
	AF_INET
	SOCK_STREAM
*/

#include <arpa/inet.h>
/*
	htons();
	ntonl();
	ntohs();
	inet_aton();
	inet_ntoa();
*/

#include <netinet/in.h>
/*
	inet_aton();
	inet)ntoa();
*/

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#define BUFFERLEN 1024
#define ADDR_SIZE sizeof(struct sockaddr_in)

#endif
