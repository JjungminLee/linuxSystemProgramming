
#include <stdio.h>
#include <string.h>
char *commands[10] = {
    "backup",
    "remove",
    "recover",
    "list",
    "help",
    "rm",
    "rc",
    "vi",
    "vim",
    "exit"};

void help(char *command)
{
    if (command == NULL)
    {
        printf("Usage:\n");
        printf("  > backup <PATH> [OPTION]... : backup file if <PATH> is file\n");
        printf("    -d : backup files in directory if <PATH> is directory \n");
        printf("    -r : backup files in directory recursive if <PATH> is directory \n");
        printf("    -y : backup file although already backuped\n");
        printf("  > remove <PATH> [OPTION]... : remove backuped file if <PATH> is file\n");
        printf("    -d : remove backuped files in directory if <PATH> is directory \n");
        printf("    -r : remove backuped files in directory recursive if <PATH> is directory \n");
        printf("    -a : remove all backuped files \n");
        printf("  > recover <PATH> [OPTION]... : recover backuped file if <PATH> is file\n");
        printf("    -d : recover backuped files in directory if <PATH> is directory\n");
        printf("    -r : recover backuped files in directory recursive if <PATH> is directory \n");
        printf("    -l : recover latest backuped files \n");
        printf("    -n <NEW_PATH> : recover backuped file with new path \n");
        printf("  > list [PATH] : show backup list by directory structure \n");
        printf("    >> rm <INDEX> [OPTION]... : remove backuped files of [INDEX] with [OPTION] \n");
        printf("    >> rc <INDEX> [OPTION]... : recover backuped files of [INDEX] with [OPTION] \n");
        printf("    >> vi(m) <INDEX> : edit original file of [INDEX] \n");
        printf("    >> exit : exit program \n");
        printf("  > help [COMMAND] : show commands of program \n");
    }
    else if (!strcmp(command, commands[0]))
    {
        printf("Usage:\n");
        printf("  > backup <PATH> [OPTION]... : backup file if <PATH> is file\n");
        printf("    -d : backup files in directory if <PATH> is directory \n");
        printf("    -r : backup files in directory recursive if <PATH> is directory \n");
        printf("    -y : backup file although already backuped\n");
    }
    else if (!strcmp(command, commands[1]))
    {
        printf("Usage:\n");
        printf("  > remove <PATH> [OPTION]... : remove backuped file if <PATH> is file\n");
        printf("    -d : remove backuped files in directory if <PATH> is directory \n");
        printf("    -r : remove backuped files in directory recursive if <PATH> is directory \n");
        printf("    -a : remove all backuped files \n");
    }

    else if (!strcmp(command, commands[2]))
    {
        printf("Usage:\n");
        printf("  > recover <PATH> [OPTION]... : recover backuped file if <PATH> is file\n");
        printf("    -d : recover backuped files in directory if <PATH> is directory\n");
        printf("    -r : recover backuped files in directory recursive if <PATH> is directory \n");
        printf("    -l : recover latest backuped files \n");
        printf("    -n <NEW_PATH> : recover backuped file with new path \n");
    }
    else if (!strcmp(command, commands[3]))
    {
        printf("Usage:\n");
        printf("  > list [PATH] : show backup list by directory structure \n");
        printf("    >> rm <INDEX> [OPTION]... : remove backuped files of [INDEX] with [OPTION] \n");
        printf("    >> rc <INDEX> [OPTION]... : recover backuped files of [INDEX] with [OPTION] \n");
        printf("    >> vi(m) <INDEX> : edit original file of [INDEX] \n");
        printf("    >> exit : exit program \n");
    }
}
