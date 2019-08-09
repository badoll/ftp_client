#include "client.h"
char SERVER_IP[BUFFERLEN];
int
main() 
{
    int control_fd = -1;
    while(1) {
        printf("ftp> ");
        char cmd[BUFFERLEN];
        fpurge(stdin);
        fgets(cmd,BUFFERLEN,stdin);
        int c = input_to_cmd(cmd);
        int n_param = 0;
        char** params = get_param(cmd,&n_param);
        switch (c)
        {
        case OPEN:
            control_fd = command_open(n_param,params);
            break;
        case CD:
            command_cd(control_fd,n_param,params[0]);
            break;
        case LCD:
            command_lcd(n_param,params[0]);
            break;
        case LS:
            command_ls(control_fd,n_param,params);
            break;
        case LLS:
            command_lls(n_param);
            break;
        case GET:
            command_get(control_fd,n_param,params[0]);
            break;
        case MGET:
            command_mget(control_fd,n_param,params);
            break;
        case PUT:
            command_put(control_fd,n_param,params[0]);
            break;
        case MPUT:
            command_mput(control_fd,n_param,params);
            break;
        case MKDIR:
            command_mkdir(control_fd,n_param,params[0]);
            break;
        case SYST:
            command_syst(control_fd,n_param);
            break;
        case QUIT:
            command_quit(control_fd,n_param);
            break;
        default:
            break;
        }
        for (int i = 0; i < n_param; ++i) {
            free(params[i]);
        }
        free(params);
    }
}
