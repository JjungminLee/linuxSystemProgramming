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

int change = 0;
int insert = 0;
int delete = 0;

int dircnt = 0;

int backup_file(char *origin_path, char *backup_path, char *repo_name)
{
  struct stat statbuf;
  int fd1, fd2;
  int len;
  char buf[BUF_MAX];
  char backup_file_path[PATH_MAX];
  char backupTmp[PATH_MAX] = "";
  char backupdirtmp[PATH_MAX] = "";
  strcpy(backupTmp, origin_path);
  char *backPtr = strstr(backupTmp, exePATH);
  int backStart = backPtr - backupTmp + strlen(exePATH);
  int backEnd = strlen(backupTmp);
  char *backupPrefix = substr(backupTmp, backStart, backEnd);
  // backupPrefix에서 맨 뒤에서 부터 순회하면서 처음 나온 슬래시를 기준으로 그 뒤를 전부 자르기

  char *last_slash = strrchr(backupPrefix, '/'); // 마지막 '/'의 위치를 찾기

  if (last_slash != NULL)
  {
    *last_slash = '\0'; // 마지막 '/' 위치에 널 문자를 삽입하여 문자열을 종료
  }
  char backupDirPath[PATH_MAX] = "";
  strcat(backupDirPath, repoPATH);
  strcat(backupDirPath, "/");
  strcat(backupDirPath, repo_name);

  strcat(backupDirPath, backupPrefix);
  strcpy(backupdirtmp, backupPrefix);
  char newString[PATH_MAX] = "";
  char tempPath[PATH_MAX] = "";
  char currentPath[PATH_MAX] = "";
  // 마지막 '/' 위치 찾기
  char *lastSlash = strrchr(backupdirtmp, '/');
  if (lastSlash != NULL)
  {
    strcpy(newString, lastSlash + 1);
    *lastSlash = '\0';
  }
  strcpy(tempPath, backupdirtmp);
  char *token = strtok(tempPath, "/");

  strcat(currentPath, repoPATH);
  strcat(currentPath, "/");
  strcat(currentPath, repo_name);

  while (token != NULL)
  {

    strcat(currentPath, "/");
    strcat(currentPath, token);

    // 디렉터리 존재 여부 확인 후 없으면 생성
    if (access(currentPath, F_OK) != 0)
    {
      if (mkdir(currentPath, 0777) == -1)
      {
        fprintf(stderr, "Failed to create directory");
        return -1;
      }
    }
    token = strtok(NULL, "/");
  }

  if (access(backupDirPath, F_OK))
    mkdir(backupDirPath, 0777);

  if ((fd1 = open(origin_path, O_RDONLY)) == -1)
  {
    fprintf(stderr, "ERROR: open error for '%s'\n", origin_path);
    return -1;
  }

  if ((fd2 = open(backup_path, O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1)
  {
    fprintf(stderr, "ERROR: open error for '%s'\n", backup_path);
    return -1;
  }

  while (len = read(fd1, buf, BUF_MAX))
  {
    write(fd2, buf, len);
  }
  // 원본 파일의 소유자 및 그룹 설정
  if (chown(backup_path, statbuf.st_uid, statbuf.st_gid) == -1)
  {
    fprintf(stderr, "ERROR: chown error\n");
    return -1;
  }

  // 타임스탬프 설정
  struct timespec times[2];
  times[0].tv_sec = statbuf.st_atime;
  // 나노초 부분은 0으로 설정
  times[0].tv_nsec = 0;
  times[1].tv_sec = statbuf.st_mtime;
  times[1].tv_nsec = 0;

  if (utimensat(AT_FDCWD, backup_path, times, 0) == -1)
  {
    fprintf(stderr, "ERROR : Failed to update timestamps");
    return -1;
  }

  close(fd1);
  close(fd2);

  return 1;
}

void current_commit_list(dirNode *dirNode, char *commit_repo_name)
{
  if (dirNode == NULL)
    return;

  fileNode *fNode = dirNode->file_head;
  while (fNode != NULL)
  {
    backupNode *bNode = fNode->backup_head;
    while (bNode != NULL)
    {
      if (strlen(bNode->origin_path) > 0 && (!strcmp(bNode->command, "new file") || !strcmp(bNode->command, "modified")))
      {
        struct stat statbuf;
        if (lstat(bNode->origin_path, &statbuf) < 0)
        {
          fprintf(stderr, "ERROR5: lstat error\n");
          exit(1);
        }
        char backupTmp[PATH_MAX] = "";
        strcpy(backupTmp, bNode->origin_path);
        char *backPtr = strstr(backupTmp, exePATH);
        int backStart = backPtr - backupTmp + strlen(exePATH);
        int backEnd = strlen(backupTmp);
        char *backupPrefix = substr(backupTmp, backStart, backEnd);
        char backup_path[PATH_MAX] = "";
        strcat(backup_path, repoPATH);
        strcat(backup_path, "/");
        strcat(backup_path, commit_repo_name);
        strcat(backup_path, backupPrefix);
        backup_file(bNode->origin_path, backup_path, commit_repo_name);
      }
      bNode = bNode->next_backup;
    }
    fNode = fNode->next_file;
  }
  dirNode = dirNode->subdir_head->next_dir;
  while (dirNode != NULL)
  {
    current_commit_list(dirNode, commit_repo_name); // 재귀 호출
    dirNode = dirNode->next_dir;
  }
}
// 5. current_commit_list에서 Modify랑 new file에 대해서만 .repo에 백업
int backup_proccess(char *repo_name)
{
  char backupDirPath[PATH_MAX] = "";
  strcat(backupDirPath, repoPATH);
  strcat(backupDirPath, "/");
  strcat(backupDirPath, repo_name);
  if (access(backupDirPath, F_OK))
    mkdir(backupDirPath, 0777);
  current_commit_list(current_commit_dir_list, repo_name);
  return 0;
}

void print_current_commit(dirNode *dirNode, char *commit_name, int commit_fd)
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
        char buf[BUF_MAX + 20];
        memset(buf, 0, sizeof(buf));
        char *relativePath = getRelativePath(bNode->origin_path);
        sprintf(buf, "commit: \"%s\" - %s: \"%s\" \n", commit_name, bNode->command, bNode->origin_path);
        if (write(commit_fd, buf, strlen(buf)) == -1)
        {
          fprintf(stderr, "ERROR: write error for %s\n", stagingLogPATH);
          return;
        }
        printf("    %s : %s\n", bNode->command, relativePath);
      }
      bNode = bNode->next_backup;
    }
    fNode = fNode->next_file;
  }
  dirNode = dirNode->subdir_head->next_dir;
  while (dirNode != NULL)
  {
    print_current_commit(dirNode, commit_name, commit_fd); // 재귀 호출
    dirNode = dirNode->next_dir;
  }
}

void lookup_current_commit(dirNode *dirNode)
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

        if (!strcmp(bNode->command, "new file"))
        {

          insert++;
        }
        else if (!strcmp(bNode->command, "modified"))
        {
          change++;
        }
        else if (!strcmp(bNode->command, "removed"))
        {
          delete ++;
        }
      }
      bNode = bNode->next_backup;
    }
    fNode = fNode->next_file;
  }
  dirNode = dirNode->subdir_head->next_dir;
  while (dirNode != NULL)
  {
    lookup_current_commit(dirNode); // 재귀 호출
    dirNode = dirNode->next_dir;
  }
}

int check_new_file(dirNode *dirNode, char *target_path)
{
  // 더 이상 탐색할 요소가 없으므로, 새 파일이라고 가정
  if (dirNode == NULL)
    return 1;

  // 파일 노드 순회
  fileNode *fNode = dirNode->file_head;
  while (fNode != NULL)
  {
    backupNode *bNode = fNode->backup_head;
    while (bNode != NULL)
    {
      if (strlen(bNode->origin_path) > 0)
      {

        if (!strcmp(target_path, bNode->origin_path))
        {
          // 일치하는 경우, 새 파일이 아니므로 0 반환
          return 0;
        }
      }

      bNode = bNode->next_backup;
    }
    fNode = fNode->next_file;
  }

  // 하위 디렉토리 노드 순회
  int found = 1; // 초기값을 1로 설정하여, 새 파일이라고 가정
  dirNode = dirNode->subdir_head;
  while (dirNode != NULL && found)
  {
    // 재귀 호출을 통해 하위 디렉토리 탐색
    found = check_new_file(dirNode, target_path);
    if (found == 0)
    {
      // 일치하는 파일을 찾았으므로 0 반환
      return 0;
    }
    dirNode = dirNode->next_dir;
  }

  // 전체 리스트를 순회했으나 target_path와 일치하는 경로를 찾지 못한 경우, 새 파일로 간주하여 1 반환
  return found;
}

// staging에는 없고 commit에는 있으면 -1리턴
// remove할거 없으면 1리턴(하나라도 둘다같은게 발견되면 1리턴)
// 커밋 리스트 path ,staging리스트 path
int remove_bfs(char *commit_path, char *staging_path)
{

  struct dirent **namelist;
  struct stat statbuf;
  dirNode *dirList;
  dirNode *curr_dir;
  char sub_backup_path[PATH_MAX];
  int cnt;
  int sub_backup_cnt = 0;
  int backup_cnt = 0;

  dirList = (dirNode *)malloc(sizeof(dirNode));
  dirNode_init(dirList);
  dirNode_append(dirList, staging_path, "");
  int isAvailable = 0; // true

  curr_dir = dirList->next_dir;
  int found = -1;

  while (curr_dir != NULL)
  {
    // @todo 여기 scandir에러 뜨는거

    if (lstat(curr_dir->dir_name, &statbuf) < 0)
    {
      fprintf(stderr, "ERROR: lstat error for %s\n", curr_dir->dir_name);
      return -1;
    }
    if (S_ISREG(statbuf.st_mode))
    {

      if (S_ISREG(statbuf.st_mode) && !strcmp(commit_path, curr_dir->dir_name))
      {

        return 1;
      }
    }
    else if (S_ISDIR(statbuf.st_mode))
    {
      if ((cnt = scandir(curr_dir->dir_name, &namelist, NULL, alphasort)) == -1)
      {
        fprintf(stderr, "ERROR: scandir error for %s\n", curr_dir->dir_name);
        return -1;
      }

      for (int i = 0; i < cnt; i++)
      {
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
          continue;

        char tmp_path[PATH_MAX] = "";

        sprintf(tmp_path, "%s/%s", curr_dir->dir_name, namelist[i]->d_name);

        if (lstat(tmp_path, &statbuf) < 0)
        {
          fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
          return -1;
        }

        // 커밋경로랑 tmp_path가 같다는건 스테이징 경로에 해당 커밋 경로가 있다는거! 즉 안없어진걸 의미
        if (S_ISREG(statbuf.st_mode) && !strcmp(commit_path, tmp_path))
        {

          return 1;
        }
        else if (S_ISDIR(statbuf.st_mode))
        {

          dirNode_append(dirList, tmp_path, "");
        }
      }
    }

    curr_dir = curr_dir->next_dir;
  }
  return -1;
}
int traverseRemoveBackupNodesReverse(backupNode *backup_head, char *exe_path)
{
  if (backup_head == NULL)
    return -1;

  backupNode *lastBackup = backup_head;
  while (lastBackup && lastBackup->next_backup != NULL)
  {
    lastBackup = lastBackup->next_backup;
  }

  backupNode *backup = lastBackup;
  while (backup != NULL)
  {
    if (backup && strlen(backup->origin_path) > 0)
    {
      // 커밋 리스트 path ,staging리스트 path
      // 백업에만 남아있으면

      if (remove_bfs(backup->origin_path, exe_path) < 0)
      {
        // 전역 currentlist에 추가
        dircnt++;
        delete ++;
        backup_list_insert(current_commit_dir_list, "removed", backup->origin_path, "", "");

        return 1;
      }
      else
      {

        return -1;
      }
      backup = backup->prev_backup;
    }
  }
}
int traverseRemoveFileNodesReverse(fileNode *file_head, char *exe_path)
{
  if (file_head == NULL)
    return -1;

  fileNode *lastFile = file_head;
  while (lastFile && lastFile->next_file != NULL)
  {
    lastFile = lastFile->next_file;
  }

  while (lastFile != NULL)
  {
    if (lastFile->backup_head)
    {
      int result = traverseRemoveBackupNodesReverse(lastFile->backup_head, exe_path);
      if (result == 1)
        return 1; // 파일 수정 발견
    }
    lastFile = lastFile->prev_file;
  }
  return -1;
}

int traverseRemoveDirNodesReverse(dirNode *dir_head, char *exe_path)
{
  if (dir_head == NULL)
    return -1;

  dirNode *lastDir = dir_head;
  while (lastDir && lastDir->next_dir != NULL)
  {
    lastDir = lastDir->next_dir;
  }

  while (lastDir != NULL)
  {
    if (lastDir->file_head)
    {
      int result = traverseRemoveFileNodesReverse(lastDir->file_head, exe_path);
      if (result == 1)
        return 1; // 파일 수정 발견
    }
    if (lastDir->subdir_head)
    {
      int result = traverseRemoveDirNodesReverse(lastDir->subdir_head, exe_path);
      if (result == 1)
        return 1; // 하위 디렉토리에서 파일 수정 발견
    }
    lastDir = lastDir->prev_dir;
  }
  return -1;
}
int check_remove_file(dirNode *dirNode, char *exe_path)
{

  // 역순으로 commit list순회
  if (traverseRemoveDirNodesReverse(commit_dir_list, exe_path) == 1)
  {
    return 1;
  }
  else
  {
    return -1;
  }
}

// 수정된게 있으면1 없으면 -1
int traverseBackupNodesReverse(backupNode *backup_head, char *target_path)
{
  if (backup_head == NULL)
    return -1;

  backupNode *lastBackup = backup_head;
  // 마지막 백업 노드로 이동
  while (lastBackup && lastBackup->next_backup != NULL)
  {
    lastBackup = lastBackup->next_backup;
  }
  // 역순으로 노드를 검사
  backupNode *backup = lastBackup;
  while (backup != NULL)
  {
    if (backup->origin_path && strlen(backup->origin_path) > 0)
    {

      // 해시값이 다르다 -> 수정 됨
      if (!strcmp(backup->origin_path, target_path) && cmpHash(backup->backup_path, target_path))
      {

        return 1;
      }
      // 해시값이 같다 -> 수정 안됨
      if (!strcmp(backup->origin_path, target_path) && !cmpHash(backup->backup_path, target_path))
      {

        return -1;
      }
    }
    backup = backup->prev_backup; // 이전 백업 노드로 이동
  }
  return -1; // 수정된 파일 없음
}
int traverseFileNodesReverse(fileNode *file_head, char *target_path)
{
  if (file_head == NULL)
    return -1;

  fileNode *lastFile = file_head;
  while (lastFile && lastFile->next_file != NULL)
  {
    lastFile = lastFile->next_file;
  }

  while (lastFile != NULL)
  {
    if (lastFile->backup_head)
    {
      int result = traverseBackupNodesReverse(lastFile->backup_head, target_path);
      if (result == 1)
        return 1; // 파일 수정 발견
    }
    lastFile = lastFile->prev_file;
  }
  return -1;
}

int traverseDirNodesReverse(dirNode *dir_head, char *target_path)
{
  if (dir_head == NULL)
    return -1;

  dirNode *lastDir = dir_head;
  while (lastDir && lastDir->next_dir != NULL)
  {
    lastDir = lastDir->next_dir;
  }

  while (lastDir != NULL)
  {
    if (lastDir->file_head)
    {
      int result = traverseFileNodesReverse(lastDir->file_head, target_path);
      if (result == 1)
        return 1; // 파일 수정 발견
    }
    if (lastDir->subdir_head)
    {
      int result = traverseDirNodesReverse(lastDir->subdir_head, target_path);
      if (result == 1)
        return 1; // 하위 디렉토리에서 파일 수정 발견
    }
    lastDir = lastDir->prev_dir;
  }
  return -1;
}

int check_modify_file(char *target_path)
{

  if (traverseDirNodesReverse(commit_logs_list, target_path) == 1)
  {
    return 1;
  }
  else
  {
    return -1;
  }
}

// remove staging list에 있는 것과 같은 경로라면 1 반환 아니면 -1

int check_remove_staging_list(dirNode *dirNode, char *target_path)
{
  if (dirNode == NULL)
  {
    return -1;
  }

  fileNode *fNode = dirNode->file_head;
  while (fNode != NULL)
  {
    backupNode *bNode = fNode->backup_head;
    while (bNode != NULL)
    {

      if (strlen(bNode->origin_path) > 0)
      {

        if (!strcmp(target_path, bNode->origin_path))
        {
          return 1; // 정확히 일치하는 경로 발견
        }
      }
      bNode = bNode->next_backup;
    }
    fNode = fNode->next_file;
  }

  // 하위 디렉토리 순회
  dirNode = dirNode->subdir_head;
  while (dirNode != NULL)
  {
    if (check_remove_staging_list(dirNode, target_path) == 1)
    {
      return 1; // 하위에서 일치하는 경로 발견
    }
    dirNode = dirNode->next_dir;
  }

  return -1; // 일치하는 경로를 찾지 못함
}

void bfs(char *path)
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
      if (check_remove_staging_list(staging_remove_dir_list, curr_dir->dir_name) == 1)
      {
        return;
      }

      // newfile 인경우
      if (check_new_file(commit_dir_list, curr_dir->dir_name) == 1)
      {
        dircnt++;

        backup_list_insert(current_commit_dir_list, "new file", curr_dir->dir_name, "", "");
        return;
      }
      // modify인경우
      else if (check_modify_file(curr_dir->dir_name) == 1)
      {
        dircnt++;

        backup_list_insert(current_commit_dir_list, "modified", curr_dir->dir_name, "", "");
        return;
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
          if (check_remove_staging_list(staging_remove_dir_list, tmp_path) == 1)
          {
            return;
          }

          // 파일인경우 커밋 로그에서 판단을 해준다.
          // newfile 인경우
          if (check_new_file(commit_dir_list, tmp_path) == 1)
          {
            dircnt++;

            backup_list_insert(current_commit_dir_list, "new file", tmp_path, "", "");
          }
          // modify인경우
          else if (check_modify_file(tmp_path) == 1)
          {
            dircnt++;

            backup_list_insert(current_commit_dir_list, "modified", tmp_path, "", "");
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
// 스테이징 경로가 현재 작업경로에 존재하지 않으면 -1반환
int check_exist_pwd(char *target_path)
{

  struct dirent **namelist;
  struct stat statbuf;
  dirNode *dirList;
  dirNode *curr_dir;
  char sub_backup_path[PATH_MAX];
  int cnt;
  int sub_backup_cnt = 0;
  int backup_cnt = 0;

  dirList = (dirNode *)malloc(sizeof(dirNode));
  dirNode_init(dirList);
  dirNode_append(dirList, exePATH, "");
  int isAvailable = 0; // true

  curr_dir = dirList->next_dir;
  int found = -1;

  while (curr_dir != NULL)
  {
    // @todo 여기 scandir에러 뜨는거

    if (lstat(curr_dir->dir_name, &statbuf) < 0)
    {
      fprintf(stderr, "ERROR: lstat error for %s\n", curr_dir->dir_name);
      return -1;
    }
    if (S_ISREG(statbuf.st_mode))
    {
      if (S_ISREG(statbuf.st_mode) && !strcmp(target_path, curr_dir->dir_name))
      {

        return 1;
      }
    }
    else if (S_ISDIR(statbuf.st_mode))
    {
      if ((cnt = scandir(curr_dir->dir_name, &namelist, NULL, alphasort)) == -1)
      {
        fprintf(stderr, "ERROR!!: scandir error for %s\n", curr_dir->dir_name);
        return -1;
      }

      for (int i = 0; i < cnt; i++)
      {
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
          continue;

        char tmp_path[PATH_MAX] = "";

        sprintf(tmp_path, "%s/%s", curr_dir->dir_name, namelist[i]->d_name);

        if (lstat(tmp_path, &statbuf) < 0)
        {
          fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
          return -1;
        }

        int len = strlen(target_path) < strlen(tmp_path) ? strlen(target_path) : strlen(tmp_path);
        // 커밋경로랑 tmp_path가 같다는건 스테이징 경로에 해당 커밋 경로가 있다는거! 즉 안없어진걸 의미
        if (S_ISREG(statbuf.st_mode) && !strncmp(target_path, tmp_path, len))
        {

          return 1;
        }
        else if (S_ISDIR(statbuf.st_mode))
        {

          dirNode_append(dirList, tmp_path, "");
        }
      }
    }

    curr_dir = curr_dir->next_dir;
  }
  return -1;
}

// staging dirlist가 매개변수로
void check_staging_list(dirNode *dirNode)
{
  struct stat statbuf;
  if (dirNode == NULL)
    return;

  // 파일 노드 순회 시작
  fileNode *fNode = dirNode->file_head->next_file;
  while (fNode != NULL)
  {
    backupNode *bNode = fNode->backup_head->next_backup;
    while (bNode != NULL)
    {
      if (strlen(bNode->origin_path) > 0)
      {
        if (lstat(bNode->origin_path, &statbuf) < 0)
        {
          printf(stderr, "lstat error %s\n", bNode->origin_path);
        }

        // bfs돌기전에 현재 디렉토리에 있는 파일인지 부터 확인

        if (S_ISREG(statbuf.st_mode))
        {
          if (check_exist_pwd(bNode->origin_path) > 0)
          {
            bfs(bNode->origin_path);
          }
        }

        bNode = bNode->next_backup;
      }
    }
    fNode = fNode->next_file;
  }

  // 하위 디렉토리 순회 시작
  dirNode = dirNode->subdir_head->next_dir;
  while (dirNode != NULL)
  {
    check_staging_list(dirNode);

    dirNode = dirNode->next_dir;
  }
}

int ssu_commit(char *commit_name)
{
  int fd;
  if ((fd = open(commitLogPATH, O_WRONLY | O_CREAT | O_APPEND)) < 0)
  {
    fprintf(stderr, "ERROR: file open error %s\n", commitLogPATH);
  }

  // 현재 커밋 담는 리스트 init

  check_staging_list(staging_dir_list);

  check_remove_file(commit_dir_list, exePATH);

  if (dircnt == 0)
  {
    printf("Nothing to commit\n");
  }
  else
  {
    lookup_current_commit(current_commit_dir_list);
    // @todo : commit 버전 레포 만들고 로그 쓰기
    backup_proccess(commit_name);
    printf("commit to %s\n", commit_name);
    printf("%d files changed, %d insertions(+), %d deletions(-)\n", change, insert, delete);
    print_current_commit(current_commit_dir_list, commit_name, fd);
  }
}

// 같은 이름의 커밋이 있다면 -1 리턴
int find_same_commit(dirNode *dirNode, char *commit_name)
{
  if (dirNode == NULL)
    return 1;

  fileNode *fNode = dirNode->file_head;
  while (fNode != NULL)
  {
    backupNode *bNode = fNode->backup_head;

    while (bNode != NULL)
    {

      if (strlen(bNode->origin_path) > 0)
      {

        remove_quotes_in_place(bNode->commit_message);
        if (!strcmp(bNode->commit_message, commit_name))
        {
          return -1;
        }
      }
      bNode = bNode->next_backup;
    }
    fNode = fNode->next_file;
  }
  dirNode = dirNode->subdir_head;
  while (dirNode != NULL)
  {
    if (find_same_commit(dirNode, commit_name) == -1)
    {
      return -1;
    }
    dirNode = dirNode->next_dir;
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
  current_commit_dir_list = (dirNode *)malloc(sizeof(dirNode));
  dirNode_init(current_commit_dir_list);

  if (init(argv[0]) == -1)
  {
    fprintf(stderr, "ERROR: init error.\n");
    return -1;
  }

  // @todo : 해당 커밋이 이미 레포에 있는지
  if (find_same_commit(commit_dir_list, argv[1]) < 0)
  {
    printf("\"%s\" is already exist in repo\n", argv[1]);
    exit(1);
  }

  ssu_commit(argv[1]);

  exit(0);
}