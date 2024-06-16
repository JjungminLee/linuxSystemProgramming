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

int check_already_remove(dirNode *dirNode, char *absolutePath,
                         char *relativePath)
{
  if (dirNode == NULL)
    return 0;

  fileNode *fNode = dirNode->file_head->next_file;
  while (fNode != NULL)
  {
    backupNode *bNode = fNode->backup_head->next_backup;
    while (bNode != NULL)
    {
      if (strlen(bNode->origin_path) > 0)
      {

        if (!strcmp(absolutePath, bNode->origin_path))
        {
          printf("\"%s\" already removed from staging area \n", relativePath);
          return 1;
        }
      }
      bNode = bNode->next_backup;
    }
    fNode = fNode->next_file;
  }
  dirNode = dirNode->subdir_head->next_dir;
  while (dirNode != NULL)
  {
    // 재귀호출하는데 유망한지 한번 더 확인해야함!!
    if (check_already_remove(dirNode, absolutePath, relativePath) == 1)
    {
      return 1;
    }
    dirNode = dirNode->next_dir;
  }
}

int ssu_remove(char *path)
{
  int fd;
  int len;
  char buf[BUF_MAX + 200];
  char pathBuf[BUF_MAX];
  char absolutePathBuf[BUF_MAX];
  char relativePathBuf[BUF_MAX];
  char absolutePath[BUF_MAX];
  // stagingLogPath열기
  if ((fd = open(stagingLogPATH, O_WRONLY | O_APPEND)) == -1)
  {
    fprintf(stderr, "ERROR: open error for '%s'\n", stagingLogPATH);
    return 1;
  }
  strcpy(pathBuf, path);
  // 절대경로
  realpath(pathBuf, absolutePathBuf);
  strcpy(absolutePath, absolutePathBuf);
  // 상대경로
  char *relativePath = getRelativePath(absolutePathBuf);

  if (check_already_remove(staging_remove_dir_list, absolutePath,
                           relativePath) == 1)
  {
    return 1;
  }
  // log와 표준출력쓰기
  sprintf(buf, "remove \"%s\"\n", absolutePath);
  if (write(fd, buf, strlen(buf)) == -1)
  {
    fprintf(stderr, "ERROR: write error for %s\n", stagingLogPATH);
    return 1;
  }
  printf("remove \"%s\" \n", relativePath);

  return 0;
}

int main(int argc, char *argv[])
{
  exePATH = (char *)malloc(PATH_MAX);
  exeNAME = (char *)malloc(PATH_MAX);
  pwdPATH = (char *)malloc(PATH_MAX);
  repoPATH = (char *)malloc(PATH_MAX);
  stagingLogPATH = (char *)malloc(PATH_MAX);
  commitLogPATH = (char *)malloc(PATH_MAX);

  if (init(argv[0]) == -1)
  {
    fprintf(stderr, "ERROR: init error.\n");
    return -1;
  }

  ssu_remove(argv[1]);
  exit(0);
}