#ifndef SSU_HELP_H
#define SSU_HELP_H

#define OPENSSL_API_COMPAT 0x10100000L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <openssl/md5.h>
#include <ctype.h>
#include <pwd.h>

#define CMD_ADD 0b0000000001
#define CMD_REMOVE 0b0000000010
#define CMD_STATUS 0b0000000100
#define CMD_COMMIT 0b0000001000
#define CMD_REVERT 0b0000010000
#define CMD_LOG 0b0000100000
#define CMD_HELP 0b0010000000
#define CMD_EXIT 0b0100000000
#define NOT_CMD 0b0000000000

void help();
void help_process(int argc, char *arg);

#endif