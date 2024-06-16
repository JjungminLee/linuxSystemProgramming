#include "ssu_header.h"

int backup_list_remove(dirNode *dirList, char *command, char *path, char *backup_path)
{
  if (dirList == NULL)
  {
    return 0;
  }

  // 경로에서 다음 '/' 위치를 찾아 하위 디렉토리 또는 파일 이름을 분리
  char *slash_pos = strchr(path, '/');
  if (slash_pos != NULL)
  {
    // '/'를 기준으로 디렉토리 이름을 분리
    char dir_name[256];
    int dir_name_length = slash_pos - path;
    strncpy(dir_name, path, dir_name_length);
    dir_name[dir_name_length] = '\0';

    // 해당 디렉토리 노드 찾기
    dirNode *subdir = dirNode_get(dirList->subdir_head, dir_name);
    if (subdir != NULL)
    {
      // 재귀 호출로 하위 경로 처리
      if (backup_list_remove(subdir, command, slash_pos + 1, backup_path))
      {
        // 하위 항목이 성공적으로 제거되면 상위 디렉토리의 카운터 감소
        if (subdir->file_cnt == 0 && subdir->subdir_cnt == 0 && subdir->backup_cnt == 0)
        {
          dirNode_remove(subdir);
          dirList->subdir_cnt--;
          return 1;
        }
      }
    }
  }
  else
  {
    // 파일 노드 찾기
    fileNode *file = fileNode_get(dirList->file_head, path);
    if (file != NULL)
    {
      // 파일의 모든 백업 노드 제거
      while (file->backup_head->next_backup != NULL)
      {
        backupNode_remove(file->backup_head->next_backup);
      }
      if (file->backup_cnt == 0)
      {
        fileNode_remove(file);
        dirList->file_cnt--;
        return 1;
      }
    }
  }
  return 0;
}

int backup_list_insert(dirNode *dirList, char *command, char *path,
                       char *backup_path, char *commit_message)
{

  char *ptr;

  if (ptr = strchr(path, '/'))
  {
    char *dir_name = substr(path, 0, strlen(path) - strlen(ptr));
    dirNode *curr_dir =
        dirNode_insert(dirList->subdir_head, dir_name, dirList->dir_path);
    backup_list_insert(curr_dir, command, ptr + 1, backup_path, commit_message);
    curr_dir->backup_cnt++;
  }
  else
  {
    char *file_name = path;
    fileNode *curr_file =
        fileNode_insert(dirList->file_head, file_name, dirList->dir_path);
    backupNode_insert(curr_file->backup_head, command, file_name,
                      dirList->dir_path, backup_path, commit_message);
  }

  return 0;
}

void print_depth(int depth, int is_last_bit)
{
  for (int i = 1; i <= depth; i++)
  {
    if (i == depth)
    {
      if ((1 << depth) & is_last_bit)
      {
        printf("┗ ");
      }
      else
      {
        printf("┣ ");
      }
      break;
    }
    if ((1 << i) & is_last_bit)
    {
      printf("  ");
    }
    else
    {
      printf("┃ ");
    }
  }
}

void print_list(dirNode *dirList, int depth, int last_bit)
{
  dirNode *curr_dir = dirList->subdir_head->next_dir;
  fileNode *curr_file = dirList->file_head->next_file;

  while (curr_dir != NULL && curr_file != NULL)
  {
    if (strcmp(curr_dir->dir_name, curr_file->file_name) < 0)
    {
      print_depth(depth, last_bit);
      printf("%s/ %d %d %d\n", curr_dir->dir_name, curr_dir->file_cnt,
             curr_dir->subdir_cnt, curr_dir->backup_cnt);
      print_list(curr_dir, depth + 1, last_bit);
      curr_dir = curr_dir->next_dir;
    }
    else
    {
      print_depth(depth, last_bit);
      printf("%s %d\n", curr_file->file_name, curr_file->backup_cnt);

      backupNode *curr_backup = curr_file->backup_head->next_backup;
      while (curr_backup != NULL)
      {
        print_depth(depth + 1, last_bit);
        printf("%s\n", curr_backup->command);
        curr_backup = curr_backup->next_backup;
      }
      curr_file = curr_file->next_file;
    }
  }

  while (curr_dir != NULL)
  {
    last_bit |= (curr_dir->next_dir == NULL) ? (1 << depth) : 0;

    print_depth(depth, last_bit);
    printf("%s/ %d %d %d\n", curr_dir->dir_name, curr_dir->file_cnt,
           curr_dir->subdir_cnt, curr_dir->backup_cnt);
    print_list(curr_dir, depth + 1, last_bit);
    curr_dir = curr_dir->next_dir;
  }

  while (curr_file != NULL)
  {
    last_bit |= (curr_file->next_file == NULL) ? (1 << depth) : 0;

    print_depth(depth, last_bit);
    printf("%s %d\n", curr_file->file_name, curr_file->backup_cnt);

    backupNode *curr_backup = curr_file->backup_head->next_backup;
    while (curr_backup != NULL)
    {
      print_depth(depth + 1, (last_bit | ((curr_backup->next_backup == NULL)
                                              ? (1 << depth + 1)
                                              : 0)));
      printf("%s\n", curr_backup->command);
      curr_backup = curr_backup->next_backup;
    }
    curr_file = curr_file->next_file;
  }
}

void remove_quotes_in_place(char *str)
{
  int len = strlen(str);
  int writeIndex = 0;

  for (int i = 0; i < len; i++)
  {
    // 큰따옴표가 아닌 문자만 복사
    if (str[i] != '\"')
    {
      str[writeIndex++] = str[i];
    }
  }
  str[writeIndex] = '\0';
}

int init_commit_list(int commit_fd)
{
  int len;
  char buf[BUF_MAX];

  char *command, *origin_path, *backup_path;
  commit_dir_list = (dirNode *)malloc(sizeof(dirNode));
  commit_logs_list = (dirNode *)malloc(sizeof(dirNode));
  dirNode_init(commit_dir_list);
  dirNode_init(commit_logs_list);

  while (len = read(commit_fd, buf, BUF_MAX))
  {
    char *ptr = strchr(buf, '\n');
    ptr[0] = '\0';

    lseek(commit_fd, -(len - strlen(buf)) + 1, SEEK_CUR);

    // commit 메시지 시작점 찾기
    char commitMessage[STR_MAX];
    memset(commitMessage, 0, STR_MAX);
    char *commitStart = strstr(buf, COMMIT_PREFIX);

    commitStart += strlen(COMMIT_PREFIX);
    char *commitEnd = strstr(commitStart, " - ");
    if (commitEnd)
    {
      char *temp = substr(buf, commitStart - buf, commitEnd - buf);
      strncpy(commitMessage, temp, STR_MAX - 1);
      commitMessage[STR_MAX - 1] = '\0';
      free(temp);
    }

    char fileName[PATH_MAX];
    memset(fileName, 0, sizeof(fileName));
    char *fileStart;
    if ((fileStart = strstr(commitEnd, NEWFILE_SEP)) != NULL)
    {
      int start = fileStart - commitEnd + strlen(NEWFILE_SEP); // add sep 이후 인덱스
      int end = strlen(commitEnd) - 1;
      char *path = substr(commitEnd, start, end);
      remove_quotes_in_place(path);
      char backupTmp[PATH_MAX] = "";
      strcpy(backupTmp, path);
      char *backPtr = strstr(backupTmp, exePATH);
      int backStart = backPtr - backupTmp + strlen(exePATH);
      int backEnd = strlen(backupTmp);
      char *backupPrefix = substr(backupTmp, backStart, backEnd);
      char backup_path[PATH_MAX] = "";
      strcat(backup_path, repoPATH);
      strcat(backup_path, "/");
      remove_quotes_in_place(commitMessage);
      strcat(backup_path, commitMessage);
      strcat(backup_path, backupPrefix);

      backup_list_insert(commit_dir_list, "new file", path, backup_path, commitMessage);
      backup_list_insert(commit_logs_list, "new file", path, backup_path, commitMessage);
    }
    else if ((fileStart = strstr(commitEnd, MODIFIED_SEP)))
    {
      int start = fileStart - commitEnd + strlen(MODIFIED_SEP); // add sep 이후 인덱스
      int end = strlen(commitEnd) - 1;
      char *path = substr(commitEnd, start, end);
      remove_quotes_in_place(path);
      char backupTmp[PATH_MAX] = "";
      strcpy(backupTmp, path);
      char *backPtr = strstr(backupTmp, exePATH);
      int backStart = backPtr - backupTmp + strlen(exePATH);
      int backEnd = strlen(backupTmp);
      char *backupPrefix = substr(backupTmp, backStart, backEnd);
      char backup_path[PATH_MAX] = "";
      strcat(backup_path, repoPATH);
      strcat(backup_path, "/");
      remove_quotes_in_place(commitMessage);
      strcat(backup_path, commitMessage);
      strcat(backup_path, backupPrefix);

      // backup_list_insert(commit_dir_list, "modified", path, backup_path, commitMessage);

      backup_list_insert(commit_logs_list, "modified", path, backup_path, commitMessage);
    }
    else if ((fileStart = strstr(commitEnd, REMOVED_SEP)))
    {
      int start = fileStart - commitEnd + strlen(REMOVED_SEP); // add sep 이후 인덱스
      int end = strlen(commitEnd) - 1;
      char *path = substr(commitEnd, start, end);
      remove_quotes_in_place(path);
      char backupTmp[PATH_MAX] = "";
      strcpy(backupTmp, path);
      char *backPtr = strstr(backupTmp, exePATH);
      int backStart = backPtr - backupTmp + strlen(exePATH);
      int backEnd = strlen(backupTmp);
      char *backupPrefix = substr(backupTmp, backStart, backEnd);
      char backup_path[PATH_MAX] = "";
      strcat(backup_path, repoPATH);
      strcat(backup_path, "/");
      remove_quotes_in_place(commitMessage);
      strcat(backup_path, commitMessage);
      strcat(backup_path, backupPrefix);

      backup_list_remove(commit_dir_list, "delete", path, backup_path);
      backup_list_insert(commit_logs_list, "removed", path, backup_path, commitMessage);
    }
  }
  return 0;
}

void bfs_backup(char *path, char *cmd)
{

  struct dirent **namelist;
  struct stat statbuf;
  dirNode *dirList;
  dirNode *curr_dir;
  char sub_backup_path[PATH_MAX];
  int cnt;
  int sub_backup_cnt = 0;
  int backup_cnt = 0;
  // bfs위한 큐
  dirList = (dirNode *)malloc(sizeof(dirNode));
  dirNode_init(dirList);

  dirNode_append(dirList, path, "");

  curr_dir = dirList->next_dir;

  while (curr_dir != NULL)
  {
    // @todo -> 여기서 안됨!

    if (lstat(curr_dir->dir_name, &statbuf) < 0)
    {
      fprintf(stderr, "ERROR3: lstat error for %s\n", curr_dir->dir_name);
      return;
    }

    if (S_ISREG(statbuf.st_mode))
    {
      if (!strcmp(cmd, "add"))
      {
        backup_list_insert(backup_dir_list, "add", curr_dir->dir_name, "", "");
        backup_list_insert(staging_dir_list, "add", curr_dir->dir_name, "", "");
        backup_list_remove(staging_remove_dir_list, "add", curr_dir->dir_name, "");
      }
      else if (!strcmp(cmd, "remove"))
      {
        backup_list_insert(backup_dir_list, "remove", curr_dir->dir_name, "", "");
        backup_list_insert(staging_remove_dir_list, "remove", curr_dir->dir_name, "", "");

        backup_list_remove(staging_dir_list, "remove", curr_dir->dir_name, "");
      }
    }
    else
    {

      if ((cnt = scandir(curr_dir->dir_name, &namelist, NULL, alphasort)) == -1)
      {
        fprintf(stderr, "ERROR: scandir error for %s\n", curr_dir->dir_name);
        return;
      }

      for (int i = 0; i < cnt; i++)
      {
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
          continue;

        char tmp_path[PATH_MAX];
        sprintf(tmp_path, "%s/%s", curr_dir->dir_name, namelist[i]->d_name);
        if (lstat(tmp_path, &statbuf) < 0)
        {
          fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
          return;
        }

        if (S_ISREG(statbuf.st_mode))
        {

          if (!strcmp(cmd, "add"))
          {
            backup_list_insert(backup_dir_list, "add", tmp_path, "", "");
            backup_list_insert(staging_dir_list, "add", tmp_path, "", "");
            backup_list_remove(staging_remove_dir_list, "add", tmp_path, "");
          }
          else if (!strcmp(cmd, "remove"))
          {

            backup_list_insert(backup_dir_list, "remove", tmp_path, "", "");
            backup_list_insert(staging_remove_dir_list, "remove", tmp_path, "", "");

            backup_list_remove(staging_dir_list, "remove", tmp_path, "");
          }
        }
        else if (S_ISDIR(statbuf.st_mode))

        {
          dirNode_append(dirList, tmp_path, "");
        }
      }
    }

    curr_dir = curr_dir->next_dir;
  }
}

int init_backup_list(int log_fd)
{
  int len;
  char buf[BUF_MAX];

  char *command, *origin_path, *backup_path;
  backup_dir_list = (dirNode *)malloc(sizeof(dirNode));
  staging_dir_list = (dirNode *)malloc(sizeof(dirNode));
  staging_remove_dir_list = (dirNode *)malloc(sizeof(dirNode));

  dirNode_init(backup_dir_list);
  dirNode_init(staging_dir_list);
  dirNode_init(staging_remove_dir_list);

  while (len = read(log_fd, buf, BUF_MAX))
  {
    char *ptr = strchr(buf, '\n');
    ptr[0] = '\0';

    lseek(log_fd, -(len - strlen(buf)) + 1, SEEK_CUR);
    struct stat statbuf;

    // add시
    if ((ptr = strstr(buf, ADD_SEP)) != NULL)
    {
      int start = ptr - buf + strlen(ADD_SEP); // add sep 이후 인덱스
      int end = strlen(buf);                   // 종료인덱스
      origin_path = substr(buf, start, end);
      remove_quotes_in_place(origin_path);
      backup_path = "";
      if (lstat(origin_path, &statbuf) < 0)
      {
        fprintf(stderr, "lstat error %s\n", origin_path);
        exit(1);
      }
      if (S_ISDIR(statbuf.st_mode))
      {
        backup_list_insert(backup_dir_list, "add", origin_path, backup_path, "");
        backup_list_insert(staging_dir_list, "add", origin_path, backup_path, "");
        backup_list_remove(staging_remove_dir_list, "add", origin_path,
                           backup_path);

        bfs_backup(origin_path, "add");
      }
      else if (S_ISREG(statbuf.st_mode))
      {
        backup_list_insert(backup_dir_list, "add", origin_path, backup_path, "");
        backup_list_insert(staging_dir_list, "add", origin_path, backup_path, "");
        backup_list_remove(staging_remove_dir_list, "add", origin_path,
                           backup_path);
      }
    }
    else if ((ptr = strstr(buf, REMOVE_SEP)) != NULL)
    {
      int start = ptr - buf + strlen(REMOVE_SEP); // add sep 이후 인덱스
      int end = strlen(buf);                      // 종료인덱스
      origin_path = substr(buf, start, end);
      remove_quotes_in_place(origin_path);
      backup_path = "";
      // origin_path에서 가장 상위 실행 디렉토리도 삭제
      char originPathBuf[PATH_MAX] = "";
      strcpy(originPathBuf, origin_path);
      ptr = strstr(originPathBuf, exePATH);
      if (ptr != NULL)
      {

        int exePathLength = strlen(exePATH);

        char *resultPath = ptr + exePathLength;
        char topDir[2];
        char *firstSlash = strchr(resultPath, '/');

        if (firstSlash != NULL && firstSlash[1] != '\0')
        {
          // 첫 번째 슬래시 다음 문자 추출 (상위 디렉토리만)
          topDir[0] = firstSlash[1];
          topDir[1] = '\0';
        }
        char removePath[PATH_MAX] = "";
        strcat(removePath, exePATH);
        strcat(removePath, "/");
        strcat(removePath, topDir);

        if (lstat(origin_path, &statbuf) < 0)
        {
          fprintf(stderr, "lstat error %s\n", origin_path);
          exit(1);
        }
        if (S_ISDIR(statbuf.st_mode))
        {
          backup_list_insert(backup_dir_list, "remove", origin_path, backup_path, "");
          backup_list_insert(staging_remove_dir_list, "remove", origin_path,
                             backup_path, "");

          backup_list_remove(staging_dir_list, "remove", origin_path, backup_path);
          bfs_backup(origin_path, "remove");
        }
        else if (S_ISREG(statbuf.st_mode))
        {

          backup_list_insert(backup_dir_list, "remove", origin_path, backup_path, "");
          backup_list_insert(staging_remove_dir_list, "remove", origin_path,
                             backup_path, "");

          backup_list_remove(staging_dir_list, "remove", origin_path, backup_path);
        }
      }
    }
  }

  return 0;
}

int get_version_list(dirNode *version_dir_head)
{
  int cnt;
  int ret = 0;
  struct dirent **namelist;
  struct stat statbuf;

  if ((cnt = scandir(version_dir_head->dir_path, &namelist, NULL, alphasort)))
  {
    printf("cnt:%d\n", cnt);
    fprintf(stderr, "ERROR: scandir error for %s\n",
            version_dir_head->dir_path);
    return -1;
  }

  for (int i = 0; i < cnt; i++)
  {
    if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
      continue;

    char tmp_path[PATH_MAX];
    strcpy(tmp_path, version_dir_head->dir_path);
    strcat(tmp_path, namelist[i]->d_name);

    if (lstat(tmp_path, &statbuf) < 0)
    {
      fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
      return -1;
    }

    if (S_ISDIR(statbuf.st_mode))
    {
      dirNode *version_subdir_node =
          dirNode_insert(version_dir_head->subdir_head, namelist[i]->d_name,
                         version_dir_head->dir_path);
      ret |= get_version_list(version_subdir_node);
    }
  }
  return ret;
}

int link_backup_node(dirNode *dirList)
{
  dirNode *curr_dir = dirList->subdir_head->next_dir;
  fileNode *curr_file = dirList->file_head->next_file;
  backupNode *curr_backup;
  dirNode *curr_version_dir;

  while (curr_dir != NULL)
  {
    link_backup_node(curr_dir);
    curr_dir = curr_dir->next_dir;
  }

  while (curr_file != NULL)
  {
    curr_backup = curr_file->backup_head->next_backup;
    while (curr_backup != NULL)
    {
      char *tmp_path = substr(curr_backup->backup_path, strlen(stagingLogPATH),
                              strlen(curr_backup->backup_path));
      char *ptr = strrchr(tmp_path, '/');
      ptr[0] = '\0';

      // curr_version_dir = get_dirNode_from_path(version_dir_list, tmp_path);
      curr_version_dir = NULL;
      curr_backup->root_version_dir = curr_version_dir;
      // add_cnt_root_dir(curr_backup->root_version_dir, 1);

      curr_backup = curr_backup->next_backup;
    }
    curr_file = curr_file->next_file;
  }
}

int init_version_list()
{
  version_dir_list = (dirNode *)malloc(sizeof(dirNode));
  dirNode_init(version_dir_list);
  strcpy(version_dir_list->dir_path, stagingLogPATH);
  get_version_list(version_dir_list);

  link_backup_node(backup_dir_list);

  // print_list(version_dir_list, 0, 0);
}
void print_backup_origin_paths(dirNode *dirNode)
{

  if (dirNode == NULL)
    return;

  fileNode *fNode = dirNode->file_head;
  while (fNode != NULL)
  {
    backupNode *bNode = fNode->backup_head;
    while (bNode != NULL)
    {
      if (strlen(bNode->origin_path) > 0)
      {
        printf("%s\n", bNode->origin_path);
      }
      bNode = bNode->next_backup;
    }
    fNode = fNode->next_file;
  }
  dirNode = dirNode->subdir_head;
  while (dirNode != NULL)
  {
    print_backup_origin_paths(dirNode); // 재귀 호출
    dirNode = dirNode->next_dir;
  }
}

int init(char *path)
{
  int staging_fd;
  int commit_fd;
  // 사용자 정보 가지는 구조체
  struct passwd *pw;
  char *loginName;
  uid_t uid;
  // 현재 사용자의 로그인 이름을 얻기
  loginName = getlogin();
  if (loginName == NULL)
  {
    // getlogin이 실패한 경우, 사용자 ID를 사용
    uid = getuid();
    pw = getpwuid(uid);
  }
  else
  {
    // 특정 사용자 이름(loginName)에 대한 사용자 계정 정보를 검색하고, 그 결과를
    // pw 변수에 저장
    pw = getpwnam(loginName);
  }

  if (pw == NULL)
  {
    printf("ERROR: Failed to get user information\n");
    return 1;
  }

  // 실행경로 /home/ubuntu/lsp/p2
  getcwd(exePATH, PATH_MAX);
  // 현재  /home/ubuntu
  sprintf(pwdPATH, "%s", pw->pw_dir);

  // .repo 경로 받아오기
  sprintf(repoPATH, "%s/.repo", pw->pw_dir);
  // staging log경로
  sprintf(stagingLogPATH, "%s/.repo/.staging.log", pw->pw_dir);
  // commit log 경로
  sprintf(commitLogPATH, "%s/.repo/.commit.log", pw->pw_dir);

  if (access(repoPATH, F_OK))
  {
    if (mkdir(repoPATH, 0755) == -1)
    {
      return -1;
    };
  }

  if ((staging_fd = open(stagingLogPATH, O_RDWR | O_CREAT, 0777)) == -1)
  {
    fprintf(stderr, "ERROR: open error for '%s'\n", stagingLogPATH);
    return -1;
  }
  if ((commit_fd = open(commitLogPATH, O_RDWR | O_CREAT, 0777)) == -1)
  {
    fprintf(stderr, "ERROR: open error for '%s'\n", commitLogPATH);
    return -1;
  }

  init_backup_list(staging_fd);

  init_commit_list(commit_fd);

  close(staging_fd);
  close(commit_fd);

  return 0;
}