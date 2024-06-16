#include "ssu_header.h"


void help(int cmd_bit)
{
    if (!cmd_bit)
    {
        printf("Usage: \n");
    }

    if (!cmd_bit || cmd_bit & CMD_ADD)
    {
        printf("%s add <PATH> : add files/directories to staging area\n", (cmd_bit ? "Usage:" : "  >"));
    }

    if (!cmd_bit || cmd_bit & CMD_REMOVE)
    {
        printf("%s remove <PATH> : record path to staging area, path will not tracking modification\n", (cmd_bit ? "Usage:" : "  >"));
    }

    if (!cmd_bit || cmd_bit & CMD_STATUS)
    {
        printf("%s status : show staging area status\n", (cmd_bit ? "Usage:" : "  >"));
    }

    if (!cmd_bit || cmd_bit & CMD_COMMIT)
    {
        printf("%s commit <NAME> : backup staging area with commit name\n", (cmd_bit ? "Usage:" : "  >"));
    }

    if (!cmd_bit || cmd_bit & CMD_REVERT)
    {
        printf("%s revert <NAME> : recover commit version with commit name\n", (cmd_bit ? "Usage:" : "  >"));
    }
    if (!cmd_bit || cmd_bit & CMD_LOG)
    {
        printf("%s log : show commit log\n", (cmd_bit ? "Usage:" : "  >"));
    }
    if (!cmd_bit || cmd_bit & CMD_HELP)
    {
        printf("%s help : show commands for program\n", (cmd_bit ? "Usage:" : "  >"));
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
    else if (!strcmp(argv[1], "status"))
    {
        help(CMD_STATUS);
    }
    else if (!strcmp(argv[1], "commit"))
    {
        help(CMD_COMMIT);
    }
    else if (!strcmp(argv[1], "revert"))
    {
        help(CMD_REVERT);
    }
    else if (!strcmp(argv[1], "log"))
    {
        help(CMD_LOG);
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
