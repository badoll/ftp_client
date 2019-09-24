#include "client.h"
static char commands[N_COMMAND][10] =  {
    "open",
    "cd","!cd",
    "ls","!ls",
    "get","mget",
    "put","mput",
    "mkdir","syst",
    "quit"
}; /* the sequence must be the same as enum COMMAND */

int
input_to_cmd(char* cmd)
{
    for (int i = 0; i < BUFFERLEN; ++i) {
        if (cmd[i] == '\n')
            cmd[i] = '\0';
    }/* remove '\n' from fgets() */
    char* input = (char*) malloc (sizeof(char)*BUFFERLEN);
    strcpy(input,cmd);
    const char delim[2] = " ";
    char *cmdstr;
    cmdstr = strtok(input, delim);
    int cmdlen = strlen(cmdstr);
    int cmdid = -1;
    for (int i = 0; i < cmdlen; ++i) {
        if (cmdstr[i] >='A' && cmdstr[i] <= 'Z') {
            cmdstr[i] += 'a' - 'A';
        }
    } /* transform command to lower-case */
    for (int i = 0; i < N_COMMAND; ++i) {
        if (strcmp(cmdstr,commands[i]) == 0) {
            cmdid = i;
            break;
        }
    }
    if (cmdid == -1) {
        printf("Error: Invalid command\n");
    }
    free(input);
    return cmdid;
}

char**
get_param(char* cmd, int* n_param) {
    char** params = NULL;
    const char delim[2] = " ";
    char *token;
    token = strtok(cmd,delim);
    token = strtok(NULL,delim);
    while( token != NULL ) {
        params = (char**) realloc (params, sizeof(char*) * (*n_param+1));
        params[*n_param] = (char*) malloc (sizeof(char) * (strlen(token)+1));
        strcpy(params[(*n_param)++],token);
        token = strtok(NULL, delim);
    }
    return params;
}

int
creat_socket(char* ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Create socket failed.");
        return -1;
    }
    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(ip);
    sockaddr.sin_port = htons(port);
    if (connect(sockfd,(const struct sockaddr*)&sockaddr,
        (socklen_t)ADDR_SIZE) == -1) {
        perror("Connect to server failed.");
        return -1;
    }
    return sockfd;
}

int
receive(char* data, int sockfd, const char* kind)
{
    bzero(data,BUFFERLEN);
    char buf[BUFFERLEN];
    int recvlen = recv(sockfd,buf,BUFFERLEN-1,0);
    if (recvlen == -1) {
        printf("Error: receive %s failed.\n",kind);
        return -1;
    }
    buf[recvlen] = '\0';
    // printf("recvlen: %d, %s: %s",recvlen,kind,buf);
    strcpy(data,buf);
    if (recvlen < BUFFERLEN - 1) {
        return 1;
    }
    return 0; 
}

int
pasv(int control_fd, char* type)
{
    char buf[BUFFERLEN];
    char reply[BUFFERLEN];
    strcpy(buf,"TYPE ");
    strcat(buf,type);
    strcat(buf,"\r\n");
    if (send(control_fd,buf,strlen(buf),0) == -1) {
        printf("Error: send command failed.\n");
        return -1;
    }
    if (receive(reply,control_fd,"reply") == -1) {
        return -1;
    }
    bzero(buf,BUFFERLEN);
    bzero(reply,BUFFERLEN);
    strcpy(buf,"PASV\r\n");
    if (send(control_fd,buf,strlen(buf),0) == -1) {
        printf("Error: send command failed\n");
        return -1;
    }
    if (receive(reply,control_fd,"reply") == -1) {
        return -1;
    }
    if (strncmp(reply,"421",3) == 0) {
        printf("Error: Connection failed.\n");
        return -1;
    } else if (strncmp(reply,"227",3) != 0) {
        printf("Error: receive data port failed.\n");
        return -1;
    }
    char* token;
    char* delim = "(,)";
    token = strtok(reply,delim);
    for(int i = 0; i < 4; ++i) {
        token = strtok(NULL,delim);
    }
    token = strtok(NULL,delim);
    int num1 = atoi(token);
    token = strtok(NULL,delim);
    int num2 = atoi(token); 
    int dataport = num1 * 256 + num2;
    int data_fd = creat_socket(SERVER_IP,dataport);
    if (data_fd == -1) {
        return -1;
    }
    return data_fd;
}

int
send_userinfo(int control_fd, const char* info)
{
    char buf[BUFFERLEN];
    char reply[BUFFERLEN];
    char str[BUFFERLEN];
    int user = 0;
    struct termios stty_info;
    if (strcmp(info,"username") == 0) {
        strcpy(buf,"USER ");
        user = 1;
    } else {
        strcpy(buf,"PASS ");
        if (tcgetattr(0,&stty_info) == -1) {
            perror("get stty attribute failed.");
            return -1;
        }
        stty_info.c_lflag &= ~ECHO;
        /* set password ~ECHO */
        if (tcsetattr(0,TCSANOW,&stty_info) == -1) {
            perror("set stty attribute failed.");
            return -1;
        }
    }
    printf("Please enter your %s:",info);
    scanf("%s",str);
    if (strcmp(info,"password") == 0) {
        stty_info.c_lflag |= ECHO;
        if (tcsetattr(0,TCSANOW,&stty_info) == -1) {
            perror("set stty attribute failed.");
            return -1;
        }
        printf("\n");
    }
    strcat(buf,str);
    strcat(buf,"\r\n");
    if (send(control_fd,buf,strlen(buf),0) == -1) {
        printf("Error: send %s failed\n",info);
        return -1;
    }
    if (receive(reply,control_fd,"reply") == -1) {
        return -1;
    }
    if (user == 1) {  /* deal USER reply */
        if (strncmp(reply,"331",3) == 0) {
            return 1;
        }
    } else {  /* deal PASS reply */
        if (strncmp(reply,"230",3) == 0) {
            return 1;
        } else if (strncmp(reply,"530",3) == 0) {
            printf("Error: Login incorrect.\n");
            return 0;
        }
    }
    if (strncmp(reply,"421",3) == 0) {
        printf("Error: Connection failed.\n");
    } else if (strncmp(reply,"5",1) == 0) {
        printf("Error: send %s failed.\n",info);
    }
    printf("Error: Unknown error.\n");
    return -1;
}

int
login(int control_fd)
{
    int times = 3;
    int res;
    while (times--) {
        if (send_userinfo(control_fd,"username") != 1) {
           return -1;
        }
        if ((res = send_userinfo(control_fd,"password")) == 0) {
            if (times != 0) {
                printf("Warning: You have %d time(s) to try.\n",times);
            }
            continue;
        } else {
            return res;
        }
    }
    return -1;
}

int
command_open(int n_param, char** params)
{
    if (n_param > 2 || n_param < 1) {
        printf("Usage: open server_ipaddress "
        "[control_port](default:21)\n");
        return -1;
    } else if (n_param == 2) {
        if (strcmp(params[1],"21") != 0) {
            printf("Error: Control port must be 21.\n");
            return -1;
        }
    }
    strcpy(SERVER_IP,params[0]);
    /* default control port is 20. */
    int control_fd = creat_socket(SERVER_IP,21);
    if (control_fd == -1) {
        return -1;
    }
    char reply[BUFFERLEN];
    if (receive(reply,control_fd,"reply") == -1) {
        return -1;
    }
    if (strncmp(reply,"220",3) != 0) {
        printf("Error: Connection failed.\n");
    }
    printf("Connect to server successfully.\n");
    if (login(control_fd) == 1) {
        printf("> Login in successfully.\n");
        return control_fd;
    } /* login in successfully */
    return -1;
}

void
command_cd(int control_fd, int n_param, char* pathname)
{
    if (n_param != 1) {
        printf("Usage: cd path\n");
        return;
    }
    if (control_fd == -1) {
        printf("Error: Not logined in.\n");
        return;
    }
    char buf[BUFFERLEN];
    char reply[BUFFERLEN];
    strcpy(buf,"CWD ");
    strcat(buf,pathname);
    strcat(buf,"\r\n");
    if (send(control_fd,buf,strlen(buf),0) == -1) {
        printf("Error: send command failed\n");
        return;
    }
    if (receive(reply,control_fd,"reply") == -1) {
        return;
    }
    if (strncmp(reply,"250",3) == 0) { /* success */
        return;
    } else if (strncmp(reply,"550",3) == 0) {
        printf("Error: No such directory.\n");
        return;
    } else if (strncmp(reply,"421",3) == 0) {
        printf("Error: Connection failed.\n");
        return;
    } else {
        printf("Error: Can't change to this directory.\n");
    }
}

void
command_lcd(int n_param, char* pathname)
{
    if (n_param != 1) {
        printf("Usage: !cd path\n");
        return;
    }
    if (chdir(pathname) == -1) {
        perror("change directory error");
    }
}

void
command_ls(int control_fd, int n_param, char** params)
{
    if (n_param != 0 ) {
        if (!(n_param == 1 && strcmp(params[0],"-l") == 0)) {
            printf("Usage: ls [-l]\n");
            return;
        }
    }
    if (control_fd == -1) {
        printf("Error: Not logined in.\n");
        return;
    }
    int data_fd;
    if ((data_fd = pasv(control_fd,"A")) == -1) { /* ASCII mode */
        return;
    } /* connect to data port */
    char buf[BUFFERLEN];
    char data[BUFFERLEN];
    char reply[BUFFERLEN];
    strcpy(buf,"LIST\r\n");
    if (send(control_fd,buf,strlen(buf),0) == -1) {
        printf("Error: send command failed.\n");
        return;
    }
    if (receive(reply,control_fd,"reply") == -1) {
        return;
    }
    if (strncmp(reply,"125",3) == 0 || strncmp(reply,"150",3) == 0) {
        /* start to transfer data */
        int done = 0;
        int entry_num = 0;
        char** entries = NULL;
        char remainder[BUFFERLEN] = "none";
        while (done != 1) {
            done = receive(data,data_fd,"data");
            if (done == -1) { /* error */
                return;
            }
            int remain = 0;
            if (done == 0) { 
                if (strncmp(data+BUFFERLEN-3,"\r\n",2) != 0) {
                    remain = 1;
                } /* data incomplete */
            }
            /* deal data format */
            char line[BUFFERLEN];
            char info[BUFFERLEN],name[BUFFERLEN],type[BUFFERLEN];
            char format[BUFFERLEN];
            char* token;
            char* delim = "\r\n";
            token = strtok(data,delim);
            if (strcmp(remainder,"none") != 0) {
                strcat(remainder,token); /* merge to complete message */
                token = (char*) malloc (sizeof(char) * (strlen(remainder)+1));
                strcpy(token,remainder);
                strcpy(remainder,"none");
            }
            while (token != NULL) {
                strcpy(line,token);
                if ((token = strtok(NULL,delim)) == NULL && remain) {
                    strcpy(remainder,line); /* wait for remaining data */
                    break;
                }
                if (line[0] == 'd') {
                    strcpy(type,"DIR");
                } else if(line[0] == '-') {
                    strcpy(type,"FILE");
                } else {
                    strcpy(type,"UNKN");
                }
                int i = strlen(line);
                while(line[i-1] != ' ') {
                    i--;
                }
                strncpy(name,line+i,strlen(line)-i+1);
                if (n_param == 1) {
                    strncpy(info,line,i);
                    info[i] = '\0';
                    sprintf(format,"\t>%-4s\t%-16s\t%s",type,name,info);
                } else {
                    sprintf(format,"\t>%-4s\t%-16s",type,name);
                }
                entries = (char**) realloc (entries,sizeof(char*) * (entry_num+1));
                entries[entry_num] = (char*) malloc(sizeof(char) * (strlen(format)+1));
                strcpy(entries[entry_num++],format);
                bzero(line,BUFFERLEN);
                bzero(name,BUFFERLEN);
                bzero(info,BUFFERLEN);
                bzero(type,BUFFERLEN);
                bzero(format,BUFFERLEN);
            }
        }
        qsort(entries,entry_num,sizeof(char*),qsort_compar);
        for (int i = 0; i < entry_num; ++i) {
            printf("%s\n",entries[i]); 
        }
        if (receive(reply,control_fd,"reply") == -1) {
            close(data_fd);
            return;
        }
        if (strncmp(reply,"226",3) == 0 || strncmp(reply,"250",3) == 0) {
            printf("Reply> LIST completed, %d entry(s) in total.\n",entry_num);
        } else {
            printf("Error: LIST error.\n");
        }
    } else if (strncmp(reply,"421",3) == 0) {
        printf("Error: Connection failed.\n");
    } else {
        printf("Error: LIST failed.\n");
    }
    close(data_fd); /* end data connection */
    return;
}

void
command_lls(int n_param)
{
    if (n_param != 0 ) {
        printf("Usage: !ls\n");
        return;
    }
    DIR* dirp;
    struct dirent* entry;
    char** entries = NULL;
    int entry_num = 0;
    char type[BUFFERLEN],name[BUFFERLEN];
    char format[BUFFERLEN];
    if ((dirp = opendir(".")) == NULL) {
        perror("Open this directory failed.");
        return;
    }
    while ((entry = readdir(dirp)) != NULL) {
        strcpy(name,entry->d_name);
        if (entry->d_type == DT_DIR) {
            strcpy(type,"DIR");
        } else if (entry->d_type == DT_REG) {
            strcpy(type,"FILE");
        } else {
            strcpy(type,"UNKN");
        }
        sprintf(format,"\t>%-4s\t%-16s",type,name);
        entries = (char**) realloc (entries,sizeof(char*) * (entry_num+1));
        entries[entry_num] = (char*) malloc (sizeof(char) * (strlen(format)+1));
        strcpy(entries[entry_num++],format);
    }
    qsort(entries,entry_num,sizeof(char*),qsort_compar);
    for (int i = 0; i < entry_num; ++i) {
        printf("%s\n",entries[i]); 
    }
    printf("\t%d entry(s) in total.\n",entry_num);
    return;
}

void
command_get(int control_fd, int n_param, char* pathname)
{
    if (n_param != 1) {
        printf("Usage: get pathname or mget p1 p2 p3\n");
        return;
    }
    if (control_fd == -1) {
        printf("Error: Not logined in.\n");
        return;
    }

    int data_fd;
    if ((data_fd = pasv(control_fd,"I")) == -1) { /* binary mode */
        return;
    } /* connect to data port */
    char buf[BUFFERLEN];
    char data[BUFFERLEN];
    char reply[BUFFERLEN];
    strcpy(buf,"RETR ");
    strcat(buf,pathname);
    strcat(buf,"\r\n");
    if (send(control_fd,buf,strlen(buf),0) == -1) {
        printf("Error: send command failed.\n");
        close(data_fd);
        return;
    }
    if (receive(reply,control_fd,"reply") == -1) {
        close(data_fd);
        return;
    }
    if (strncmp(reply,"125",3) == 0 || strncmp(reply,"150",3) == 0) {
        /* start to transfer data */
        FILE* f;
        if ((f = fopen(pathname,"wb")) == NULL) {
            fprintf(stderr,"open/create File: %s failed.\n",pathname);
            close(data_fd);
            return;
        }
        int done = 0;
        printf("> Start to download File: %s\n", pathname);
        while ( done != 1) {
            /* each data binary string has a terminating '\0' */
            done = receive(data,data_fd,"data");
            int datalen = strlen(data);
            if (fwrite(data,1,datalen,f) < datalen) {
                fprintf(stderr,"write to File: %s failed.\n",pathname);
                close(data_fd);
                return;
            }
            printf("> received %d bytes.\n",datalen);
            bzero(data,BUFFERLEN);
        }
        fclose(f);
        if (receive(reply,control_fd,"reply") == -1) {
            close(data_fd);
            return;
        }
        if (strncmp(reply,"226",3) == 0 || strncmp(reply,"250",3) == 0) {
            printf("Reply> RETR completed, download File: %s successfully.\n",pathname);
        } else if (strncmp(reply,"450",3) == 0 || strncmp(reply,"550",3) == 0) {
            printf("Error: File: %s unavailable.\n", pathname);
        } else {
            printf("Error: RETR error.\n");
        }
        
    }  else if (strncmp(reply,"421",3) == 0) {
        printf("Error: Connection failed.\n");
    } else {
        printf("Error: Download failed.\n");
    }
    close(data_fd);
    return;
}

void
command_mget(int control_fd, int n_param, char** params)
{
    if (n_param == 0) {
        printf("Usage: get pathname or mget p1 p2 p3\n");
        return;
    }
    if (control_fd == -1) {
        printf("Error: Not logined in.\n");
        return;
    }
    for (int i = 0; i < n_param; ++i) {
        command_get(control_fd,1,params[i]);
        if (i != n_param-1) {
            printf("\n");
        }
    }
}
void
command_put(int control_fd, int n_param, char* pathname)
{
    if (n_param != 1) {
        printf("Usage: put pathname or mput p1 p2 p3\n");
        return;
    }
    if (control_fd == -1) {
        printf("Error: Not logined in.\n");
        return;
    }
    int data_fd;
    if ((data_fd = pasv(control_fd,"I")) == -1) { /* binary mode */
        return;
    } /* connect to data port */
    char buf[BUFFERLEN];
    char data[BUFFERLEN];
    char reply[BUFFERLEN];
    strcpy(buf,"STOR ");
    strcat(buf,pathname);
    strcat(buf,"\r\n");
    if (send(control_fd,buf,strlen(buf),0) == -1) {
        printf("Error: send command failed.\n");
        close(data_fd);
        return;
    }
    if (receive(reply,control_fd,"reply") == -1) {
        close(data_fd);
        return;
    }
    if (strncmp(reply,"125",3) == 0 || strncmp(reply,"150",3) == 0) {
        /* start to transfer data */
        FILE* f;
        if ((f = fopen(pathname,"rb")) == NULL) {
            fprintf(stderr,"open File: %s failed.\n",pathname);
            close(data_fd);
            return;
        }
        printf("> Start to upload File: %s\n", pathname);
        while (!feof(f)) { /* transfer data */
        /* each data binary string has a terminating '\0' */
            int datalen;
            if ((datalen = fread(data,1,BUFFERLEN,f)) == 0) {
                fprintf(stderr,"read data from File: %s failed.\n", pathname);
                close(data_fd);
                return;
            }
            if (send(data_fd,data,datalen,0) == -1) {
                printf("Error: send data in File: %s failed.\n", pathname);
                close(data_fd);
                return;
            }
            printf("> sended %d bytes.\n",datalen);
            bzero(data,BUFFERLEN);
        }
        fclose(f);
        close(data_fd); /* end data connection actively when transfer ended */
        if (receive(reply,control_fd,"reply") == -1) {
            close(data_fd);
            return;
        }
        if (strncmp(reply,"226",3) == 0 || strncmp(reply,"250",3) == 0) {
            printf("Reply> STOR completed, upload File: %s successfully.\n",pathname);
        } else if (strncmp(reply,"452",3) == 0 || strncmp(reply,"552",3) == 0) {
            printf("Error: File: %s too large.\n", pathname);
        } else if (strncmp(reply,"450",3) == 0) {
            printf("Error: File: %s unavailable.\n", pathname);
        } else {
            printf("Error: STOR error.\n");
        }
        
    } else if (strncmp(reply,"421",3) == 0) {
        printf("Error: Connection failed.\n");
    } else {
        printf("Error: Upload failed.\n");
    }
    close(data_fd);
    return;
}

void
command_mput(int control_fd, int n_param, char** params)
{
    if (n_param == 0) {
        printf("Usage: put pathname or mput p1 p2 p3\n");
        return;
    }
    if (control_fd == -1) {
        printf("Error: Not logined in.\n");
        return;
    }
    for (int i = 0; i < n_param; ++i) {
        command_put(control_fd,1,params[i]);
        if (i != n_param-1) {
            printf("\n");
        }
    }
}

void
command_mkdir(int control_fd, int n_param, char* pathname)
{
    if (n_param != 1) {
        printf("Usage: mkdir pathname\n");
        return;
    }
    if (control_fd == -1) {
        printf("Error: Not logined in.\n");
        return;
    }
    char buf[BUFFERLEN];
    char reply[BUFFERLEN];
    strcpy(buf,"MKD ");
    strcat(buf,pathname);
    strcat(buf,"\r\n");
    if (send(control_fd,buf,strlen(buf),0) == -1) {
        printf("Error: send command failed.\n");
        return;
    }
    if (receive(reply,control_fd,"reply") == -1) {
        return;
    }
    if (strncmp(reply,"257",3) == 0) {
        /* success */
    } else if (strncmp(reply,"550",3) == 0) {
        printf("Error: File unavailable.\n");
    } else if (strncmp(reply,"421",3) == 0) {
        printf("Error: Connection failed.\n");
    } else {
        printf("Error: Make directory failed.\n");
    }
    return;
}

void
command_syst(int control_fd, int n_param)
{
    if (n_param != 0) {
        printf("Usage: syst\n");
        return;
    }
    if (control_fd == -1) {
        printf("Error: Not logined in.\n");
        return;
    }
    char buf[BUFFERLEN];
    char reply[BUFFERLEN];
    strcpy(buf,"SYST\r\n");
    if (send(control_fd,buf,strlen(buf),0) == -1) {
        printf("Error: send command failed.\n");
        return;
    }
    if (receive(reply,control_fd,"reply") == -1) {
        return;
    }
    if (strncmp(reply,"215",3) == 0) {
        printf("Reply> Server System: %s", reply+4);
    } else if (strncmp(reply,"421",3) == 0) {
        printf("Error: Connection failed.\n");
    } else {
        printf("Error: SYST failed.\n");
    }
    return;
}

void
command_quit(int control_fd, int n_param)
{
    if (n_param != 0) {
        printf("Usage: quit\n");
        return;
    }
    if (control_fd == -1) {
        printf("Exit FTP CLIENT.\n");
        exit(0);
    } 
    char buf[BUFFERLEN];
    char reply[BUFFERLEN];
    strcpy(buf,"QUIT\r\n");
    if (send(control_fd,buf,strlen(buf),0) == -1) {
        printf("Error: send command failed.\n");
        return;
    }
    if (receive(reply,control_fd,"reply") == -1) {
        return;
    }
    if (strncmp(reply,"221",3) == 0) {
        close(control_fd);
        printf("Exit FTP CLIENT.\n");
        exit(0);
    } else {
        printf("Error: QUIT failed.\n");
        return;
    }
}

int
qsort_compar(const void* s1, const void* s2)
{
    char* var1 = *(char**)s1;
    char* var2 = *(char**)s2;
    char* str1 = (char*) malloc (sizeof(char)* (strlen(var1)+1));
    char* str2 = (char*) malloc (sizeof(char)* (strlen(var2)+1));
    strcpy(str1,var1);
    strcpy(str2,var2);
    for (int i = 0; i < strlen(str1); ++i) {
        if (str1[i] >= 'A' && str1[i] <= 'Z') {
            str1[i] += 'a' - 'A';
        }
    }
    for (int i = 0; i < strlen(str2); ++i) {
        if (str2[i] >= 'A' && str2[i] <= 'Z') {
            str2[i] += 'a' - 'A';
        }
    }
    int res = strcmp(str1,str2);
    free(str1);
    free(str2);
    return res;
}