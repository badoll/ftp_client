#ifndef CLIENT_H
#define CLIENT_H
#include "common.h"
#include <termios.h>

enum COMMAND
{
    OPEN,
    CD,
    LCD,
    LS,
    LLS,
    GET,
    MGET,
    PUT,
    MPUT,
    MKDIR,
    SYST,
    QUIT
};

#define N_COMMAND 12

extern char SERVER_IP[BUFFERLEN];

int creat_socket(char* ip, int port);
int input_to_cmd(char* cmd);
char** get_param(char* cmd, int* n_param);
int receive(char* data, int sockfd, const char* kind);
int login(int control_fd);
int send_userinfo(int control_fd, const char* info);

int pasv(int control_fd, char* type);

int command_open(int n_param,char** params);
void command_cd(int control_fd,int n_param,char* pathname);
void command_lcd(int n_param,char* pathname);
void command_ls(int control_fd,int n_param,char** param);
void command_lls(int n_param);
void command_get(int control_fd, int n_param, char* pathname);
void command_mget(int control_fd, int n_param, char** params);
void command_put(int control_fd, int n_param, char* pathname);
void command_mput(int control_fd, int n_param, char** params);
void command_mkdir(int control_fd, int n_param, char* pathname);
void command_syst(int control_fd, int n_param);
void command_quit(int control_fd, int n_param);

int qsort_compar(const void* s1, const void* s2);

#endif