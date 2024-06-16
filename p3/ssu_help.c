#include "ssu_header.h"

dirNode *backup_dir_list = NULL;
dirNode *monitor_dir_list = NULL;
dirNode *print_create_dir_list = NULL;
dirNode *log_dir_list = NULL;
char *exeNAME = NULL;
char *exePATH = NULL;
char *pwdPATH = NULL;
char *backupPATH = NULL;
char *monitorLogPATH = NULL;

int hash = 0;

void help(int cmd_bit)
{
    if (!cmd_bit)
    {
        printf("Usage: \n");
    }

    if (!cmd_bit || cmd_bit & CMD_ADD)
    {
        printf("%s add <PATH> [OPTION]... : add new daemon process of <PATH> if <PATH> is file\n", (cmd_bit ? "Usage:" : "  >"));
        printf("%s  -d : add new daemon process of <PATH> if <PATH> is directory\n", (!cmd_bit ? "  " : ""));
        printf("%s  -r : add new daemon process of <PATH> recursive if <PATH> is directory\n", (!cmd_bit ? "  " : ""));
        printf("%s  -t <TIME> : set daemon process time to <TIME> sec (default : 1sec)\n", (!cmd_bit ? "  " : ""));
    }

    if (!cmd_bit || cmd_bit & CMD_REMOVE)
    {
        printf("%s remove <DAEMON_PID> : delete daemon process with <DAEMON_PID>\n", (cmd_bit ? "Usage:" : "  >"));
    }

    if (!cmd_bit || cmd_bit & CMD_LIST)
    {
        printf("%s list [DAEMON_PID] : show daemon process list or dir tree\n", (cmd_bit ? "Usage:" : "  >"));
    }

    if (!cmd_bit || cmd_bit & CMD_HELP)
    {
        printf("%s help [COMMAND] : show commands for program\n", (cmd_bit ? "Usage:" : "  >"));
    }
    if (!cmd_bit || cmd_bit & CMD_EXIT)
    {
        printf("%s exit : exit program\n", (cmd_bit ? "Usage:" : "  >"));
    }
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        help(0);
    }
    else if (!strcmp(argv[1], "add"))
    {
        help(CMD_ADD);
    }
    else if (!strcmp(argv[1], "remove"))
    {
        help(CMD_REMOVE);
    }
    else if (!strcmp(argv[1], "list"))
    {
        help(CMD_LIST);
    }

    else if (!strcmp(argv[1], "help"))
    {
        help(CMD_HELP);
    }
    else if (!strcmp(argv[1], "exit"))
    {
        help(CMD_EXIT);
    }
    exit(0);
}
