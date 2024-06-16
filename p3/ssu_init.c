#include "ssu_header.h"

int backup_list_remove(dirNode *dirList, char *time, char *path, char *backup_path, char *pid)
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
      if (backup_list_remove(subdir, time, slash_pos + 1, backup_path, pid))
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
    // if (file != NULL)
    // {
    //   // 파일의 모든 백업 노드 제거
    //   while (file->backup_head->next_backup != NULL)
    //   {
    //     backupNode_remove(file->backup_head->next_backup);
    //   }
    //   if (file->backup_cnt == 0)
    //   {
    //     fileNode_remove(file);
    //     dirList->file_cnt--;
    //     return 1;
    //   }
    // }

    if (file != NULL)
    {
      // 파일의 백업 노드 중 pid가 일치하는 백업 노드만 제거
      backupNode *prev_backup = file->backup_head;
      backupNode *current_backup = prev_backup->next_backup;
      while (current_backup != NULL)
      {
        if (!strcmp(current_backup->pid, pid))
        {
          backupNode_remove(current_backup);
          prev_backup->next_backup = current_backup->next_backup;
          file->backup_cnt--;
          if (file->backup_cnt == 0)
          {
            fileNode_remove(file);
            dirList->file_cnt--;
            return 1; // 성공적으로 파일 노드를 제거하면 1 반환
          }
          return 1; // PID가 일치하는 백업 노드를 제거하면 1 반환
        }
        prev_backup = current_backup;
        current_backup = current_backup->next_backup;
      }
    }
  }
  return 0;
}

int backup_list_insert(dirNode *dirList, char *time, char *path,
                       char *backup_path, char *pid)
{

  char *ptr;

  if (ptr = strchr(path, '/'))
  {
    char *dir_name = substr(path, 0, strlen(path) - strlen(ptr));
    dirNode *curr_dir =
        dirNode_insert(dirList->subdir_head, dir_name, dirList->dir_path);
    backup_list_insert(curr_dir, time, ptr + 1, backup_path, pid);
    curr_dir->backup_cnt++;
  }
  else
  {
    char *file_name = path;
    fileNode *curr_file =
        fileNode_insert(dirList->file_head, file_name, dirList->dir_path);
    backupNode_insert(curr_file->backup_head, time, file_name,
                      dirList->dir_path, backup_path, pid);
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
        printf("%s\n", curr_backup->time);
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
      printf("%s\n", curr_backup->time);
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

int init_backup_list()
{
  int len;
  int cnt;
  struct stat statbuf;
  struct dirent **namelist;
  char buf[BUF_MAX];
  char tmp_path[PATH_MAX];
  int log_fd;

  if ((cnt = scandir(backupPATH, &namelist, NULL, alphasort)) < 0)
  {
    fprintf(stderr, "ERROR: scandir error\n");
    exit(1);
  }

  backup_dir_list = (dirNode *)malloc(sizeof(dirNode));

  dirNode_init(backup_dir_list);

  for (int i = 0; i < cnt; i++)
  {
    if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "monitor_list.log"))
    {
      continue;
    }
    memset(tmp_path, 0, PATH_MAX);

    sprintf(tmp_path, "%s/%s", backupPATH, namelist[i]->d_name);

    if (stat(tmp_path, &statbuf) < 0)
    {
      fprintf(stderr, "ERROR: stat error\n");
      exit(1);
    }

    // 5230.log에서
    // [create]면 넣어주기 , 시간 추가
    // [modify]면 이전꺼 삭제하고, 시간 추가
    // [delete]면 삭제
    if (S_ISREG(statbuf.st_mode))
    {
      char logName[PATH_MAX];
      strcpy(logName, namelist[i]->d_name);
      char *baseName;

      baseName = strtok(logName, ".");

      if ((log_fd = open(tmp_path, O_RDONLY)) < 0)
      {
        fprintf(stderr, "ERROR: open error\n");
      }
      while (len = read(log_fd, buf, BUF_MAX))
      {

        char *ptr = strchr(buf, '\n');
        ptr[0] = '\0';

        lseek(log_fd, -(len - strlen(buf)) + 1, SEEK_CUR);
        struct stat statbuf;
        char *before;
        char *after;

        // create시
        if ((ptr = strstr(buf, CREATE_SEP)) != NULL)
        {

          // '[2024-05-03 10:00:00][create][/home/oslab/a.txt]' 이런 형태의 문자열을 가정
          char *startBracket = strchr(buf, '[');
          char *endBracket = strchr(startBracket + 1, ']');
          if (endBracket)
          {
            int beforeLength = endBracket - (startBracket + 1);
            before = (char *)malloc(beforeLength + 1);
            strncpy(before, startBracket + 1, beforeLength);
            before[beforeLength] = '\0';
          }

          char *createEnd = strchr(endBracket + 1, ']'); // '[create]'의 끝을 찾는다.
          if (createEnd)
          {
            createEnd += 1; // ']' 뒤로 한 칸 이동
            if (*createEnd == ' ')
              createEnd++; // 공백이 있다면 건너뛰기

            // 이제 createEnd는 '[/home/oslab/a.txt]' 시작 직전을 가리키므로,
            // 경로를 추출하기 위해 다음 '['부터 시작
            char *pathStart = strchr(createEnd, '[');
            if (pathStart)
            {
              pathStart++;                            // '[' 다음으로 이동하여 실제 경로 시작
              char *pathEnd = strchr(pathStart, ']'); // 경로 끝인 ']' 찾기
              if (pathEnd)
              {
                int pathLength = pathEnd - pathStart;
                after = (char *)malloc(pathLength + 1);
                strncpy(after, pathStart, pathLength);
                after[pathLength] = '\0'; // 문자열 종료
              }
            }
          }

          backup_list_insert(backup_dir_list, before, after, "", baseName);
        }
        else if ((ptr = strstr(buf, MODIFY_SEP)) != NULL)
        {

          // '[2024-05-03 10:00:00][create][/home/oslab/a.txt]' 이런 형태의 문자열을 가정
          char *startBracket = strchr(buf, '[');
          char *endBracket = strchr(startBracket + 1, ']');
          if (endBracket)
          {
            int beforeLength = endBracket - (startBracket + 1);
            before = (char *)malloc(beforeLength + 1);
            strncpy(before, startBracket + 1, beforeLength);
            before[beforeLength] = '\0';
          }

          char *createEnd = strchr(endBracket + 1, ']'); // '[create]'의 끝을 찾는다.
          if (createEnd)
          {
            createEnd += 1; // ']' 뒤로 한 칸 이동
            if (*createEnd == ' ')
              createEnd++; // 공백이 있다면 건너뛰기

            // 이제 createEnd는 '[/home/oslab/a.txt]' 시작 직전을 가리키므로,
            // 경로를 추출하기 위해 다음 '['부터 시작
            char *pathStart = strchr(createEnd, '[');
            if (pathStart)
            {
              pathStart++;                            // '[' 다음으로 이동하여 실제 경로 시작
              char *pathEnd = strchr(pathStart, ']'); // 경로 끝인 ']' 찾기
              if (pathEnd)
              {
                int pathLength = pathEnd - pathStart;
                after = (char *)malloc(pathLength + 1);
                strncpy(after, pathStart, pathLength);
                after[pathLength] = '\0'; // 문자열 종료
              }
            }
          }

          backup_list_remove(backup_dir_list, before, after, "", baseName);
          backup_list_insert(backup_dir_list, before, after, "", baseName);
        }

        else if ((ptr = strstr(buf, DELETE_SEP)) != NULL)
        {

          // '[2024-05-03 10:00:00][create][/home/oslab/a.txt]' 이런 형태의 문자열을 가정
          char *startBracket = strchr(buf, '[');
          char *endBracket = strchr(startBracket + 1, ']');
          if (endBracket)
          {
            int beforeLength = endBracket - (startBracket + 1);
            before = (char *)malloc(beforeLength + 1);
            strncpy(before, startBracket + 1, beforeLength);
            before[beforeLength] = '\0';
          }

          char *createEnd = strchr(endBracket + 1, ']'); // '[create]'의 끝을 찾는다.
          if (createEnd)
          {
            createEnd += 1; // ']' 뒤로 한 칸 이동
            if (*createEnd == ' ')
              createEnd++; // 공백이 있다면 건너뛰기

            // 이제 createEnd는 '[/home/oslab/a.txt]' 시작 직전을 가리키므로,
            // 경로를 추출하기 위해 다음 '['부터 시작
            char *pathStart = strchr(createEnd, '[');
            if (pathStart)
            {
              pathStart++;                            // '[' 다음으로 이동하여 실제 경로 시작
              char *pathEnd = strchr(pathStart, ']'); // 경로 끝인 ']' 찾기
              if (pathEnd)
              {
                int pathLength = pathEnd - pathStart;
                after = (char *)malloc(pathLength + 1);
                strncpy(after, pathStart, pathLength);
                after[pathLength] = '\0'; // 문자열 종료
              }
            }
          }

          backup_list_remove(backup_dir_list, before, after, "", baseName);
        }
      }
      close(log_fd);
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

  if ((cnt = scandir(version_dir_head->dir_path, &namelist, NULL, alphasort)) < 0)
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
      char *tmp_path = substr(curr_backup->backup_path, strlen(backupPATH),
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
  int logfd;
  int backupfd;
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

  // 실행경로 /home/ubuntu/lsp/p3
  getcwd(exePATH, PATH_MAX);
  // 현재  /home/ubuntu
  sprintf(pwdPATH, "%s", pw->pw_dir);

  // backup 경로 받아오기
  sprintf(backupPATH, "%s/backup", pw->pw_dir);
  sprintf(monitorLogPATH, "%s/backup/monitor_list.log", pw->pw_dir);

  if (access(backupPATH, F_OK))
  {
    if (mkdir(backupPATH, 0755) == -1)
    {
      return -1;
    };
  }

  if ((logfd = open(monitorLogPATH, O_RDWR | O_CREAT, 0777)) == -1)
  {
    fprintf(stderr, "ERROR: open error for '%s'\n", monitorLogPATH);
    return -1;
  }

  init_backup_list();

  return 0;
}