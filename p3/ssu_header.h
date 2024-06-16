#ifndef SSU_HEADER_H
#define SSU_HEADER_H

#define OPENSSL_API_COMPAT 0x10100000L

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <openssl/md5.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#define DEBUG printf("DEBUG\n");

#define PATH_MAX 4096
#define BUF_MAX 4096
#define FILE_MAX 255
#define STR_MAX 255

#define CMD_ADD 0b0000000001
#define CMD_REMOVE 0b0000000010
#define CMD_LIST 0b0000000100
#define CMD_HELP 0b0000001000
#define CMD_EXIT 0b0000010000
#define NOT_CMD 0b0000000000

#define OPT_D 0b000001
#define OPT_R 0b000010
#define OPT_Y 0b000100
#define OPT_A 0b001000
#define OPT_T 0b010000
#define OPT_N 0b100000

#define ADD_SEP "add "
#define REMOVE_SEP "remove "
#define RECOVER_SEP "\" recovered to \""
#define COMMIT_PREFIX "commit: "
#define NEWFILE_SEP " - new file: "
#define MODIFIED_SEP " - modified: "
#define REMOVED_SEP " - removed: "

#define CREATE_SEP "[create]"
#define MODIFY_SEP "[modify]"
#define DELETE_SEP "[remove]"

#define HASH_MD5 33

typedef struct command_parameter
{
  char *command;
  char *filename;
  char *tmpname;
  int commandopt;
  char *argv[10];
} command_parameter;

typedef struct _backupNode
{
  struct _dirNode *root_version_dir;
  struct _fileNode *root_file;

  char time[STR_MAX];

  char origin_path[PATH_MAX];
  char backup_path[PATH_MAX];
  char pid[PATH_MAX];

  struct _backupNode *prev_backup;
  struct _backupNode *next_backup;
} backupNode;

typedef struct _fileNode
{
  struct _dirNode *root_dir;

  int backup_cnt;
  char file_name[FILE_MAX];
  char file_path[PATH_MAX];
  backupNode *backup_head;

  struct _fileNode *prev_file;
  struct _fileNode *next_file;
} fileNode;

typedef struct _dirNode
{
  struct _dirNode *root_dir;

  int file_cnt;
  int subdir_cnt;
  int backup_cnt;
  char dir_name[FILE_MAX];
  char dir_path[PATH_MAX];
  fileNode *file_head;
  struct _dirNode *subdir_head;

  struct _dirNode *prev_dir;
  struct _dirNode *next_dir;
} dirNode;

typedef struct _pathNode
{
  char path_name[FILE_MAX];
  int depth;

  struct _pathNode *prev_path;
  struct _pathNode *next_path;

  struct _pathNode *head_path;
  struct _pathNode *tail_path;
} pathNode;

extern dirNode *backup_dir_list;
extern dirNode *monitor_dir_list;
// list명령어시 create된 시간만 담는 리스트
extern dirNode *print_create_dir_list;

extern dirNode *log_dir_list;
// 실행파일 이름
extern char *exeNAME;
// 현재 실행 디렉 경로
extern char *exePATH;
// HOME으로 지정한경로
extern char *pwdPATH;
extern char *backupPATH;
extern char *monitorLogPATH;

extern int hash;

void help();
void help_process(int argc, char *arg);

char *quote_check(char **str, char del);
char **get_substring(char *str, int *cnt, char *del);
char *tokenize(char *str, char *del);
char *get_file_name(char *path);
char *cvt_time_2_str(time_t time);
char *substr(char *str, int beg, int end);
char *c_str(char *str);
char *cvt_path_2_realpath(char *path);
int make_dir_path(char *path);

int md5(char *target_path, char *hash_result);
int cvtHash(char *target_path, char *hash_result);
int cmpHash(char *path1, char *path2);

int path_list_init(pathNode *curr_path, char *path);
char *getRelativePath(char *absolutePath);
char *remove_base_path(char *full_path, const char *base_path);

// ssu_struct.c
void dirNode_init(dirNode *dir_node);
dirNode *dirNode_get(dirNode *dir_head, char *dir_name);
dirNode *dirNode_insert(dirNode *dir_head, char *dir_name, char *dir_path);
dirNode *dirNode_append(dirNode *dir_head, char *dir_name, char *dir_path);
void dirNode_remove(dirNode *dir_node);

int add_cnt_root_dir(dirNode *dir_node, int cnt);

void fileNode_init(fileNode *file_node);
fileNode *fileNode_get(fileNode *file_head, char *file_name);
fileNode *fileNode_insert(fileNode *file_head, char *file_name, char *dir_path);
void fileNode_remove(fileNode *file_node);

void backupNode_init(backupNode *backup_node);
backupNode *backupNode_get(backupNode *backup_head, char *command);
backupNode *backupNode_insert(backupNode *backup_head, char *command,
                              char *file_name, char *dir_path,
                              char *backup_path, char *commit_message);
void backupNode_remove(backupNode *backup_node);

dirNode *get_dirNode_from_path(dirNode *dirList, char *path);
fileNode *get_fileNode_from_path(dirNode *dirList, char *path);
backupNode *get_backupNode_from_path(dirNode *dirList, char *path,
                                     char *command);

// ssu_init.c
int init(char *path);
int init_backup_list();
void print_list(dirNode *dirList, int depth, int last_bit);
void print_depth(int depth, int is_last_bit);
int backup_list_insert(dirNode *dirList, char *command, char *path,
                       char *backup_path, char *commit_message);

#endif