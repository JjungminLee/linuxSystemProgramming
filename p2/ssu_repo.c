#include "ssu_header.h"

dirNode *backup_dir_list = NULL;
dirNode *version_dir_list = NULL;
dirNode *staging_dir_list = NULL;
dirNode *commit_dir_list = NULL;
dirNode *staging_remove_dir_list = NULL;
dirNode *current_commit_dir_list = NULL;
dirNode *untrack_dir_list = NULL;
dirNode *commit_logs_list = NULL;
char *exeNAME = NULL;
char *exePATH = NULL;
char *pwdPATH = NULL;
char *repoPATH = NULL;
char *stagingLogPATH = NULL;
char *commitLogPATH = NULL;
int hash = 0;

dirNode *get_dirNode_from_path(dirNode *dirList, char *path)
{
  char *ptr;

  if (dirList == NULL)
    return NULL;

  if (ptr = strchr(path, '/'))
  {
    char *dir_name = substr(path, 0, strlen(path) - strlen(ptr));
    dirNode *curr_dir = dirNode_get(dirList->subdir_head, dir_name);
    return get_dirNode_from_path(curr_dir, ptr + 1);
  }
  else
  {
    char *dir_name = path;
    dirNode *curr_dir = dirNode_get(dirList->subdir_head, dir_name);
    return curr_dir;
  }
}

fileNode *get_fileNode_from_path(dirNode *dirList, char *path)
{
  char *ptr;
  dirNode *curr_dir;
  fileNode *curr_file;

  ptr = strrchr(path, '/');

  if ((curr_dir = get_dirNode_from_path(
           dirList, substr(path, 0, strlen(path) - strlen(ptr)))) == NULL)
    return NULL;
  curr_file = fileNode_get(curr_dir->file_head, ptr + 1);
  return curr_file;
}

backupNode *get_backupNode_from_path(dirNode *dirList, char *path,
                                     char *backup_time)
{
  fileNode *curr_file;
  backupNode *curr_backup;

  if ((curr_file = get_fileNode_from_path(dirList, path)) == NULL)
    return NULL;
  curr_backup = backupNode_get(curr_file->backup_head, backup_time);
  return curr_backup;
}

int add_cnt_root_dir(dirNode *dir_node, int cnt)
{
  if (dir_node == NULL)
    return 0;
  dir_node->backup_cnt += cnt;
  return add_cnt_root_dir(dir_node->root_dir, cnt);
}

void removeExec(char *path)
{
  pid_t pid;

  if ((pid = fork()) < 0)
  {
    fprintf(stderr, "ERROR: fork error\n");
    exit(1);
  }
  else if (pid == 0)
  {
    if ((execl("./ssu_remove", "./ssu_remove", path, (char *)NULL) == -1))
    {
      fprintf(stderr, "Error: execl error\n");
      exit(1);
    }
  }
  else
  {
    pid = wait(NULL);
  }
}

void addExec(char *path)
{
  pid_t pid;

  if ((pid = fork()) < 0)
  {
    fprintf(stderr, "ERROR: fork error\n");
    exit(1);
  }
  else if (pid == 0)
  {
    if ((execl("./ssu_add", "./ssu_add", path, (char *)NULL) == -1))
    {
      fprintf(stderr, "Error: execl error\n");
      exit(1);
    }
  }
  else
  {
    pid = wait(NULL);
  }
}
void commitExec(char *arg)
{
  pid_t pid;

  if ((pid = fork()) < 0)
  {
    fprintf(stderr, "ERROR: fork error\n");
    exit(1);
  }
  else if (pid == 0)
  {
    if ((execl("./ssu_commit", "./ssu_commit", arg, (char *)NULL) == -1))
    {
      fprintf(stderr, "Error: execl error\n");
      exit(1);
    }
  }
  else
  {
    pid = wait(NULL);
  }
}

void statusExec()
{
  pid_t pid;

  if ((pid = fork()) < 0)
  {
    fprintf(stderr, "ERROR: fork error\n");
    exit(1);
  }
  else if (pid == 0)
  {

    if ((execl("./ssu_status", "./ssu_status", (char *)NULL) == -1))
    {
      fprintf(stderr, "Error: execl error\n");
      exit(1);
    }
  }
  else
  {
    pid = wait(NULL);
  }
}

void revertExec(char *arg)
{
  pid_t pid;

  if ((pid = fork()) < 0)
  {
    fprintf(stderr, "ERROR: fork error\n");
    exit(1);
  }
  else if (pid == 0)
  {

    if ((execl("./ssu_revert", "./ssu_revert", arg, (char *)NULL) == -1))
    {
      fprintf(stderr, "Error: execl error\n");
      exit(1);
    }
  }
  else
  {
    pid = wait(NULL);
  }
}

void logExec(char *arg)
{
  pid_t pid;

  if ((pid = fork()) < 0)
  {
    fprintf(stderr, "ERROR: fork error\n");
    exit(1);
  }
  else if (pid == 0)
  {

    if ((execl("./ssu_log", "./ssu_log", arg, (char *)NULL) == -1))
    {
      fprintf(stderr, "Error: execl error\n");
      exit(1);
    }
  }
  else
  {
    pid = wait(NULL);
  }
}

void helpExec(char *arg)
{
  pid_t pid;

  if ((pid = fork()) < 0)
  {
    fprintf(stderr, "ERROR: fork error\n");
    exit(1);
  }
  else if (pid == 0)
  {
    if ((execl("./ssu_help", "./ssu_help", arg, (char *)NULL) == -1))
    {
      fprintf(stderr, "Error: execl error\n");
      exit(1);
    }
  }
  else
  {
    pid = wait(NULL);
  }
}

// 예외처리 담당
int parameter_processing(int argcnt, char **arglist, int command,
                         command_parameter *parameter)
{
  struct stat buf;
  switch (command)
  {
  case CMD_ADD:
  {
    // 경로를 입력하지 않은 경우
    if (argcnt < 2)
    {
      fprintf(stderr,
              "ERROR: <PATH> is not include \nUsage : add <PATH> : add "
              "files/directories to staging area \n");
      return -1;
    }
    char *resolved = realpath(arglist[1], parameter->filename);
    // 경로가 올바르지 않은경우
    if (resolved == NULL)
    {
      fprintf(stderr, "ERROR: %s is wrong filepath\n", parameter->filename);
      return -1;
    }
    // 경로가 길이 제한을 넘는 경우
    if (strlen(resolved) > PATH_MAX)
    {
      fprintf(stderr, "ERROR : %s exceeds 4096 bytes \n",
              parameter->filename);
      return -1;
    }

    // 파일이나 디렉토리가 존재하지 않는 경우
    if (lstat(parameter->filename, &buf) < 0)
    {
      fprintf(stderr, "ERROR: lstat error for %s\n", parameter->filename);
      return -1;
    }
    // 일반 파일이거나 디렉토리가 아닌경우
    if (!S_ISREG(buf.st_mode) && !S_ISDIR(buf.st_mode))
    {
      fprintf(stderr, "ERROR: %s is not regular file\n", parameter->filename);
      return -1;
    }
    // 해당경로에 대한 접근 권한이 없는 경우
    if (access(parameter->filename, R_OK | W_OK) < 0)
    {
      fprintf(stderr, "ERROR: %s permission denied \n", parameter->filename);
      return -1;
    }
    // 경로가 홈 디렉토리를 벗어나는 경우, 레포 디렉토리에 백업되려고 할때
    if (strncmp(parameter->filename, exePATH, strlen(exePATH)) ||
        !strncmp(parameter->filename, repoPATH, strlen(repoPATH)))
    {
      fprintf(stderr, "ERROR: filename %s can't be add \n",
              parameter->filename);

      return -1;
    }
    break;
  }
  case CMD_REMOVE:
  {
    // 경로를 입력하지 않은 경우
    if (argcnt < 2)
    {
      fprintf(stderr,
              "ERROR: <PATH> is not include \nUsage : remove <PATH> : record "
              "path to staging area, path will not tracking modification \n");
      return -1;
    }
    char *resolved = realpath(arglist[1], parameter->filename);
    // 경로가 올바르지 않은경우
    if (resolved == NULL)
    {
      fprintf(stderr, "ERROR: %s is wrong filepath\n", parameter->filename);
      return -1;
    }
    // 경로가 길이 제한을 넘는 경우
    if (strlen(resolved) > PATH_MAX)
    {
      fprintf(stderr, "ERROR : %s exceeds 4096 bytes \n",
              parameter->filename);
      return -1;
    }

    // 파일이나 디렉토리가 존재하지 않는 경우
    if (lstat(parameter->filename, &buf) < 0)
    {
      fprintf(stderr, "ERROR: lstat error for %s\n", parameter->filename);
      return -1;
    }
    // 일반 파일이거나 디렉토리가 아닌경우
    if (!S_ISREG(buf.st_mode) && !S_ISDIR(buf.st_mode))
    {
      fprintf(stderr, "ERROR: %s is not regular file\n", parameter->filename);
      return -1;
    }
    // 해당경로에 대한 접근 권한이 없는 경우
    if (access(parameter->filename, R_OK | W_OK) < 0)
    {
      fprintf(stderr, "ERROR: %s permission denied \n", parameter->filename);
      return -1;
    }
    // 경로가 홈 디렉토리를 벗어나는 경우, 레포 디렉토리에 백업되려고 할때
    if (strncmp(parameter->filename, exePATH, strlen(exePATH)) ||
        !strncmp(parameter->filename, repoPATH, strlen(repoPATH)))
    {
      fprintf(stderr, "ERROR: filename %s can't be add \n",
              parameter->filename);

      return -1;
    }

    break;
  }
  case CMD_COMMIT:
  {
    // <NAME>입력 안한 경우
    if (argcnt < 2)
    {
      fprintf(
          stderr,
          "Usage : commit <NAME> : backup staging area with commit name\n");
      return -1;
    }
    // <NAME>이 길이 제한을 넘는 경우
    if (strlen(arglist[1]) > STR_MAX)
    {
      fprintf(stderr, "ERROR : %s exceeds 255 bytes \n", parameter->filename);
      return -1;
    }
    break;
  }
  case CMD_REVERT:
  {
    // <NAME>입력 안한 경우
    if (argcnt < 2)
    {
      fprintf(stderr,
              "ERROR: <NAME> is not include\nUsage : revert <NAME> : recover commit version with commit "
              "name\n");
      return -1;
    }
    // <NAME>이 길이 제한을 넘는 경우
    if (strlen(arglist[1]) > STR_MAX)
    {
      fprintf(stderr, "ERROR : %s exceeds 255 bytes \n", parameter->filename);
      return -1;
    }
    break;
  }
  case CMD_LOG:
  {
    // <NAME>이 길이 제한을 넘는 경우
    if (strlen(arglist[1]) > STR_MAX)
    {
      fprintf(stderr, "ERROR : %s exceeds 255 bytes \n", parameter->filename);
      return -1;
    }
    break;
  }
  }
}

void parameterInit(command_parameter *parameter)
{
  parameter->command = (char *)malloc(sizeof(char) * PATH_MAX);
  parameter->filename = (char *)malloc(sizeof(char) * PATH_MAX);
  parameter->tmpname = (char *)malloc(sizeof(char) * PATH_MAX);
  // 명령어의 옵션 나타내는 플래그
  parameter->commandopt = 0;
}
int prompt()
{
  char input[STR_MAX];
  int argcnt = 0;
  char **arglist = NULL;
  int command;
  command_parameter parameter = {(char *)0, (char *)0, (char *)0, 0};

  while (1)
  {
    arglist = NULL;
    printf("20221678> ");
    if (fgets(input, sizeof(input), stdin) == NULL)
      break;

    if (input[strlen(input) - 1] == '\n')
      input[strlen(input) - 1] = '\0'; // 개행 문자 제거

    arglist = get_substring(input, &argcnt, " \t");
    if (argcnt == 0)
      continue;

    if (!strcmp(arglist[0], "add"))
    {
      command = CMD_ADD;
    }
    else if (!strcmp(arglist[0], "remove"))
    {
      command = CMD_REMOVE;
    }
    else if (!strcmp(arglist[0], "status"))
    {
      command = CMD_STATUS;
    }
    else if (!strcmp(arglist[0], "revert"))
    {
      command = CMD_REVERT;
    }
    else if (!strcmp(arglist[0], "commit"))
    {
      command = CMD_COMMIT;
    }
    else if (!strcmp(arglist[0], "log"))
    {
      command = CMD_LOG;
    }

    else if (!strcmp(arglist[0], "help"))
    {
      command = CMD_HELP;
    }
    else if (!strcmp(arglist[0], "exit"))
    {
      command = CMD_EXIT;
      fprintf(stdout, "Program exit(0)\n");
      exit(0);
    }
    else
    {
      command = NOT_CMD;
    }

    if (command & (CMD_ADD | CMD_REMOVE | CMD_REVERT | CMD_COMMIT | CMD_LOG | CMD_STATUS))
    {
      parameterInit(&parameter);
      parameter.command = arglist[0];

      if (parameter_processing(argcnt, arglist, command, &parameter) == -1)
      {
        continue;
      }

      if (command & CMD_ADD)
      {
        addExec(arglist[1]);
      }
      else if (command & CMD_REMOVE)
      {
        removeExec(arglist[1]);
      }
      else if (command & CMD_COMMIT)
      {
        commitExec(arglist[1]);
      }
      else if (command & CMD_REVERT)
      {
        revertExec(arglist[1]);
      }
      else if (command & CMD_STATUS)
      {
        statusExec();
      }
      else if (command & CMD_LOG)
      {
        if (argcnt == 2)
        {
          logExec(arglist[1]);
        }
        else if (argcnt == 1)
        {
          logExec(NULL);
        }
      }
    }

    else if (command & CMD_HELP || command == NOT_CMD)
    {
      if (argcnt == 1)
      {
        helpExec(NULL);
      }
      else if (argcnt == 2)
      {
        helpExec(arglist[1]);
      }
    }
  }
}

int main(int argc, char *argv[])
{
  exePATH = (char *)malloc(PATH_MAX);
  exeNAME = (char *)malloc(PATH_MAX);
  pwdPATH = (char *)malloc(PATH_MAX);
  repoPATH = (char *)malloc(PATH_MAX);
  stagingLogPATH = (char *)malloc(PATH_MAX);
  commitLogPATH = (char *)malloc(PATH_MAX);

  char argvStr[PATH_MAX] = "";
  int argcnt = 0;
  char **arglist = NULL;
  command_parameter parameter = {(char *)0, (char *)0, (char *)0, 0};
  strcpy(exeNAME, argv[0]);

  for (int i = 0; i < argc; i++)
  {
    strcat(argvStr, argv[i]);
    strcat(argvStr, " ");
  }

  arglist = get_substring(argvStr, &argcnt, " ");

  if (init(argv[0]) == -1)
  {
    fprintf(stderr, "ERROR: init error.\n");
    return -1;
  }

  prompt();

  return 0;
}