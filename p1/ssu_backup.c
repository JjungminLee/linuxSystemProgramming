#include "ssu_header.h"

int BackupFile(char *path, char *date, command_parameter *parameter)
{

  int len;
  int fd, fd1, fd2, fd3, fd4;
  int idx, cnt;
  int i;
  int fileSize, tmpFileSize;
  char buf[1000000];
  struct stat statbuf, tmpbuf;
  struct dirent **namelist;
  char filepath[PATHMAX] = "";
  char filename[NAMEMAX] = "";
  char tmpdir[PATHMAX] = "";
  char newPath[PATHMAX] = "";
  char pathtmp[PATHMAX] = "";
  // 현재 path에 대한 해시값
  char filehash[NAMEMAX] = "";
  char logStr[PATHMAX * 2] = "";
  char metaPath[PATHMAX * 2] = "";

  strcpy(filepath, path);
  sprintf(pathtmp, "%s", path);
  for (idx = strlen(filepath) - 1; filepath[idx] != '/'; idx--)
    ;
  strcpy(filename, filepath + idx + 1);

  // 파일이름만 가져오기 위해서 문자열자르기
  char *tmpptr = strstr(filepath, exePATH);
  if (tmpptr != NULL)
  {
    strcpy(tmpptr, tmpptr + 1 + strlen(exePATH));
  }

  strcpy(filename, tmpptr);
  filepath[idx] = '\0';

  // path의 속성
  if (lstat(path, &statbuf) < 0)
  {
    fprintf(stderr, "ERROR다!: lstat error for %s\n", path);
    return 1;
  }
  // path의 파일 사이즈
  fileSize = statbuf.st_size;

  // 백업할 디렉토리를 버퍼에 저장
  strcat(tmpdir, backupPATH);
  strcat(tmpdir, "/");
  strcat(tmpdir, date);

  // 이미 y옵션인 경우
  if (parameter->commandopt & OPT_Y)
  {

    if (access(tmpdir, F_OK))
      mkdir(tmpdir, 0777);

    // 파일경로명에 디렉토리명이 포함될경우 삭제
    for (idx = strlen(filename) - 1; idx >= 0; idx--)
    {
      if (filename[idx] == '/')
      {
        strcpy(filename, filename + idx + 1);
        break;
      }
    }
    // 새로운경로 strcat으로 이어주기
    strcat(newPath, tmpdir);
    strcat(newPath, "/");
    strcat(newPath, filename);

    if ((fd1 = open(path, O_RDONLY)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", path);
      return 1;
    }

    if ((fd2 = open(newPath, O_CREAT | O_TRUNC | O_WRONLY, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", newPath);
      return 1;
    }

    while ((len = read(fd1, buf, statbuf.st_size)) > 0)
    {
      write(fd2, buf, len);
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd3 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }
    // ssubagLog에 작성할 로크 스트링 버퍼에 저장
    strcat(logStr, date);
    strcat(logStr, " : ");
    strcat(logStr, "\"");
    strcat(logStr, path);
    strcat(logStr, "\"");
    strcat(logStr, " backuped to ");
    strcat(logStr, "\"");
    strcat(logStr, newPath);
    strcat(logStr, "\"");
    strcat(logStr, "\n");
    if (write(fd3, logStr, strlen(logStr)) == -1)
    {
      fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
      return 1;
    }
    // 메타데이터에 원본경로@백업경로형태로 저장
    if ((fd4 = open(metaPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", newPath);
      return 1;
    }
    strcat(metaPath, path);
    strcat(metaPath, "@");
    strcat(metaPath, newPath);
    strcat(metaPath, "\n");

    if (write(fd4, metaPath, strlen(metaPath)) == -1)
    {
      fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
      return 1;
    }

    printf("\"%s\" backuped to \"%s\"\n", path, newPath);
  }
  else
  {

    // -y 옵션이 아닌 경우
    // 전역으로 관리하는 백업 링크드리스트를 뒤지면서 백업하려는 파일이 존재하는지 확인
    ConvertHash(path, filehash);
    if ((cnt = scandir(backupPATH, &namelist, NULL, alphasort)) == -1)
    {
      fprintf(stderr, "ERROR: scandir error for %s\n", path);
      return 1;
    }
    for (int i = 0; i < cnt; i++)
    {
      free(namelist[i]);
    }
    free(namelist);

    // 백업폴더에 로그가 하나존재하는 경우
    backupList *currBackupLog = mainBackupList;

    if (cnt > 3)
    {
      while (true)
      {
        // 이거 지우면 already backuped가 안됨
        if (currBackupLog->next == NULL)
        {
          break;
        }
        currBackupLog = currBackupLog->next;
      }

      // 역방향 순회로 (최신순 탐색)
      while (true)
      {
        if (currBackupLog == NULL)
        {
          break;
        }
        /*
        [FIX] : 240320 수업에서 교수님이 동일파일인지 비교할떄 해시값비교 & 파일 사이즈 비교 해야한다해서
        로직수정
        */
        struct stat tmpStatBuf;
        if (lstat(currBackupLog->cur->log, &tmpStatBuf) < 0)
        {
          fprintf(stderr, "ERROR: lstat error for %s\n", path);
          return 1;
        }
        tmpFileSize = tmpStatBuf.st_size;

        char tmpHash[PATHMAX] = "";
        ConvertHash(currBackupLog->cur->backuplog, tmpHash);
        if (!strcmp(filehash, tmpHash) && !strcmp(currBackupLog->cur->log, path) && (tmpFileSize == fileSize))
        {
          fprintf(stderr, "\"%s\" is already backuped to \"%s\" \n", path, currBackupLog->cur->backuplog);
          return 1;
        }
        currBackupLog = currBackupLog->prev;
      }
    }

    if (access(tmpdir, F_OK))
      mkdir(tmpdir, 0777);

    // 파일경로명에 디렉토리명이 포함될경우 삭제
    for (idx = strlen(filename) - 1; idx >= 0; idx--)
    {
      if (filename[idx] == '/')
      {
        strcpy(filename, filename + idx + 1);
        break;
      }
    }
    // 새로운경로 strcat으로 이어주기
    strcat(newPath, tmpdir);
    strcat(newPath, "/");
    strcat(newPath, filename);

    if ((fd1 = open(path, O_RDONLY)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", path);
      return 1;
    }

    if ((fd2 = open(newPath, O_CREAT | O_TRUNC | O_WRONLY, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", newPath);
      return 1;
    }

    while ((len = read(fd1, buf, statbuf.st_size)) > 0)
    {
      write(fd2, buf, len);
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd3 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", newPath);
      return 1;
    }
    // ssubagLog에 작성할 로크 스트링 버퍼에 저장
    strcat(logStr, date);
    strcat(logStr, " : ");
    strcat(logStr, "\"");
    strcat(logStr, path);
    strcat(logStr, "\"");
    strcat(logStr, " backuped to ");
    strcat(logStr, "\"");
    strcat(logStr, newPath);
    strcat(logStr, "\"");
    strcat(logStr, "\n");
    if (write(fd3, logStr, strlen(logStr)) == -1)
    {
      fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
      return 1;
    }
    // 메타데이터에 원본경로@백업경로형태로 저장
    if ((fd4 = open(metaPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", newPath);
      return 1;
    }
    strcat(metaPath, path);
    strcat(metaPath, "@");
    strcat(metaPath, newPath);
    strcat(metaPath, "\n");

    if (write(fd4, metaPath, strlen(metaPath)) == -1)
    {
      fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
      return 1;
    }

    printf("\"%s\" backuped to \"%s\"\n", path, newPath);
  }
  return 0;
}

int BackupFileByDir(char *exePath, char *path, char *date, command_parameter *parameter)
{

  int len;
  int fd, fd1, fd2, fd3, fd4;
  int idx, cnt;
  int i;
  int fileSize, tmpFileSize;
  char buf[1000000];
  struct stat statbuf, tmpbuf;
  struct dirent **namelist;
  char filepath[PATHMAX] = "";
  char filename[NAMEMAX] = "";
  char tmpdir[PATHMAX] = "";
  char newPath[PATHMAX] = "";
  char pathtmp[PATHMAX] = "";
  // 현재 path에 대한 해시값
  char filehash[NAMEMAX] = "";
  char logStr[PATHMAX * 2] = "";
  char metaPath[PATHMAX * 2] = "";
  char new_filename[PATHMAX] = "";

  strcpy(filepath, path);
  sprintf(pathtmp, "%s", path);
  for (idx = strlen(filepath) - 1; filepath[idx] != '/'; idx--)
    ;
  strcpy(filename, filepath + idx + 1);

  // 파일명은 실행경로 밑에서부터 입력
  char *tmpptr = strstr(filepath, exePATH);
  if (tmpptr != NULL)
  {
    strcpy(tmpptr, tmpptr + 1 + strlen(exePATH));
  }

  strcpy(filename, tmpptr);
  filepath[idx] = '\0';

  // 주어진 파일의 속성을 확인
  if (lstat(path, &statbuf) < 0)
  {
    fprintf(stderr, "ERROR: lstat error for %s\n", path);
    return 1;
  }
  // path의 파일 사이즈
  fileSize = statbuf.st_size;

  // 백업할 디렉토리를 버퍼에 저장
  strcat(tmpdir, backupPATH);
  strcat(tmpdir, "/");
  strcat(tmpdir, date);

  // 이미 y옵션인 경우
  if (parameter->commandopt & OPT_Y)
  {

    // 현재 path 임시저장소
    char pathTemp[PATHMAX] = "";
    strcpy(pathTemp, path);

    /*
      루트 디렉토리와 서브 디렉토리에서 실행했을 때 처리
    */

    // exePath에 대한 처리 exePath가 루트 디렉토리 일때와 아닐떄 구분을 해야함
    if (access(tmpdir, F_OK))
      mkdir(tmpdir, 0777);

    // 루트디렉이 아난경우
    char exeTemp[PATHMAX] = "";
    char exeTempDirPath[PATHMAX] = "";
    char tempExeDir[PATHMAX] = "";
    char exeDirPath[PATHMAX] = "";

    strcpy(exeTemp, exePath);
    char *ptrExe = strstr(exeTemp, exePATH);
    if (ptrExe != NULL)
    {
      strcpy(exeTempDirPath, ptrExe + 1 + strlen(exePATH));
    }
    strcat(exeDirPath, tmpdir);
    strcat(exeDirPath, "/");

    // 실행 디렉토리중 가장 상위 디렉토리 잘라내기
    char *slashPos = strchr(exeTempDirPath, '/');

    // 밑에 더 디렉토리가 있을 때
    if (slashPos != NULL)
    {

      size_t totalLength = strlen(exeTempDirPath);

      // '/' 이후까지의 문자열 길이 계산
      size_t dirLength = totalLength - (slashPos - exeTempDirPath + 1);

      char *dir = malloc(dirLength + 1);
      if (dir != NULL)
      {

        strncpy(dir, slashPos + 1, dirLength);
        dir[dirLength] = '\0'; // 널 종료 문자 추가

        if (!strcmp(slashPos, dir))
        {
          strcat(exeDirPath, dir);
          strcat(exeDirPath, "/");
        }

        if (access(exeDirPath, F_OK))
          mkdir(exeDirPath, 0777);
      }

      // / 기준으로 밑에 완전히 잘라내기
      char backupTemp[PATHMAX] = "";
      memmove(exeTempDirPath, slashPos + 1, strlen(slashPos + 1) + 1);
      strcpy(backupTemp, exeTempDirPath);
      // strtok를 사용하여 '/'를 구분자로 사용하여 문자열을 토큰으로 분리
      char *token = strtok(backupTemp, "/");
      strcat(exeDirPath, token);

      // token이 NULL이거나 token,backupTemp가 같다(즉 / 가 없어서 그대로 나온경우)
      if (token == NULL || !strcmp(token, exeTempDirPath))
      {
      }
      else
      {

        if (access(exeDirPath, F_OK))
          mkdir(exeDirPath, 0777);
        while (1)
        {
          token = strtok(NULL, "/"); // 다음 토큰으로 이동
          if (token == NULL)
          {
            break;
          }

          strcat(exeDirPath, "/");
          strcat(exeDirPath, token);
          if (access(exeDirPath, F_OK))
            mkdir(exeDirPath, 0777);
        }
      }
    }
    else
    {
      // 밑에 더 디렉토리가 없는경우 (루트디렉토리였을떄)
      if (slashPos == NULL)
      {
        strcat(exeDirPath, "");
      }
      else
      {

        strcat(exeDirPath, exeTempDirPath);
      }
    }

    /*
    실행 디렉토리 이하에서도 (실제로 백업 되는 부분) 디렉토리가 발견된 경우
    */

    char tmpBackupDirPath[PATHMAX] = "";
    char backupDirPath[PATHMAX] = "";
    strcpy(backupDirPath, exeDirPath);

    if (backupDirPath[strlen(backupDirPath) - 1] != '/')
    {
      backupDirPath[strlen(backupDirPath)] = '/';
    }
    if (access(backupDirPath, F_OK))
      mkdir(backupDirPath, 0777);

    // 매개변수로 받아온 실행경로 만큼 잘라내기
    char *ptrDir = strstr(pathTemp, exePath);
    if (ptrDir != NULL)
    {
      strcpy(tmpBackupDirPath, ptrDir + 1 + strlen(exePath));
    }

    // 백업디렉토리중 가장 상위 디렉토리 잘라내기
    char *slashPos2 = strchr(tmpBackupDirPath, '/');

    // 밑에 더 디렉토리가 있을 때
    if (slashPos2 != NULL)
    {
      // 실행 디렉토리 잘라내도 뎁스가 있다면 그에 해당하게 mkdir해주기
      size_t dirLength = slashPos2 - tmpBackupDirPath; // '/' 이전까지의 문자열 길이 계산
      char *dir = malloc(dirLength + 1);
      if (dir != NULL)
      {
        strncpy(dir, tmpBackupDirPath, dirLength); // '/' 이전까지의 문자열 복사
        dir[dirLength] = '\0';                     // 널 종료 문자 추가

        strcat(backupDirPath, dir);
        strcat(backupDirPath, "/");

        if (access(backupDirPath, F_OK))
          mkdir(backupDirPath, 0777);
      }
      // / 기준으로 밑에 완전히 잘라내기
      char backupTemp[PATHMAX] = "";
      memmove(tmpBackupDirPath, slashPos2 + 1, strlen(slashPos2 + 1) + 1);
      strcpy(backupTemp, tmpBackupDirPath);
      // 맨 뒤 파일 명만 가져오기
      // 파일경로명에 디렉토리명이 포함될경우 삭제
      for (idx = strlen(filename) - 1; idx >= 0; idx--)
      {
        if (filename[idx] == '/')
        {
          break;
        }
      }
      // '/' 문자가 발견된 위치부터 문자열의 끝까지의 길이를 구하기
      len = strlen(filename) - idx - 1;

      strncpy(new_filename, filename + idx + 1, len);
      new_filename[len] = '\0'; // 문자열의 끝을 나타내는 NULL 문자를 추가.

      // 맨 뒤 파일 명 지워내기 /a.txt같은거 (디렉토리만 남게)
      // backupTemp에서 '/' 문자를 뒤에서부터 찾기
      char *lastSlashPos = strrchr(backupTemp, '/');
      if (lastSlashPos != NULL)
      {
        // '/' 문자 이전까지의 문자열만 남기고 삭제
        *lastSlashPos = '\0';
      }

      // strtok를 사용하여 '/'를 구분자로 사용하여 문자열을 토큰으로 분리
      char *token = strtok(backupTemp, "/");
      strcat(backupDirPath, token);

      // new_filename랑 token이 같은경우 (파일명으로 같을것) -> 아래에 디렉토리가 더 없는경우
      if (!strcmp(token, new_filename))
      {
      }
      else
      {
        // 다른 경우 -> 아래에 디렉토리가 더 있음
        if (access(backupDirPath, F_OK))
          mkdir(backupDirPath, 0777);
        while (1)
        {
          token = strtok(NULL, "/"); // 다음 토큰으로 이동
          if (token == NULL)
          {
            break;
          }

          strcat(backupDirPath, "/");
          strcat(backupDirPath, token);

          if (access(backupDirPath, F_OK))
            mkdir(backupDirPath, 0777);
        }

        // 마지막에 파일명 붙여주기!
        strcat(backupDirPath, "/");
        strcat(backupDirPath, new_filename);
      }
    }
    else
    {
      // 밑에 더 디렉토리가 없는경우
      strcat(backupDirPath, tmpBackupDirPath);
    }

    if ((fd1 = open(path, O_RDONLY)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", path);
      return 1;
    }

    if ((fd2 = open(backupDirPath, O_CREAT | O_TRUNC | O_WRONLY, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", backupDirPath);
      return 1;
    }

    while ((len = read(fd1, buf, statbuf.st_size)) > 0)
    {
      write(fd2, buf, len);
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd3 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }
    // ssubagLog에 작성할 로크 스트링 버퍼에 저장
    strcat(logStr, date);
    strcat(logStr, " : ");
    strcat(logStr, "\"");
    strcat(logStr, path);
    strcat(logStr, "\"");
    strcat(logStr, " backuped to ");
    strcat(logStr, "\"");
    strcat(logStr, backupDirPath);
    strcat(logStr, "\"");
    strcat(logStr, "\n");
    if (write(fd3, logStr, strlen(logStr)) == -1)
    {
      fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
      return 1;
    }
    // 메타데이터에 원본경로@백업경로형태로 저장
    if ((fd4 = open(metaPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", metaPATH);
      return 1;
    }
    strcat(metaPath, path);
    strcat(metaPath, "@");
    strcat(metaPath, backupDirPath);
    strcat(metaPath, "\n");

    if (write(fd4, metaPath, strlen(metaPath)) == -1)
    {
      fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
      return 1;
    }

    printf("\"%s\" backuped to \"%s\"\n", path, backupDirPath);
  }
  else
  {
    // 전역으로 관리하는 백업 링크드리스트를 뒤지면서 백업하려는 파일이 존재하는지 확인
    ConvertHash(path, filehash);
    if ((cnt = scandir(backupPATH, &namelist, NULL, alphasort)) == -1)
    {
      fprintf(stderr, "ERROR: scandir error for %s\n", path);
      return 1;
    }
    for (int i = 0; i < cnt; i++)
    {
      free(namelist[i]);
    }
    free(namelist);
    // 백업폴더에 로그가 하나존재하는 경우
    backupList *currBackupLog = mainBackupList;
    // @ todo : cnt가 4일때 로그가 존재하며, already backup이 안뜨는 문제

    backupList *currCountBackupLog = mainBackupList;
    int countCnt = 0;

    while (true)
    {
      if (currCountBackupLog == NULL)
      {
        break;
      }

      currCountBackupLog = currCountBackupLog->next;
      countCnt++;
    }

    if (cnt == 4 && countCnt > 0)
    {

      while (true)
      {
        if (currBackupLog->next == NULL)
        {

          break;
        }

        currBackupLog = currBackupLog->next;
      }

      // 역방향 순회로 (최신순 탐색)
      while (true)
      {

        if (currBackupLog == NULL)
        {

          break;
        }

        /*
        [FIX] : 240320 수업에서 교수님이 동일파일인지 비교할떄 해시값비교 & 파일 사이즈 비교 해야한다해서
        로직수정
        */

        struct stat tmpStatBuf;
        if (lstat(currBackupLog->cur->log, &tmpStatBuf) < 0)
        {
          fprintf(stderr, "ERROR: lstat error for %s\n", path);
          return 1;
        }
        tmpFileSize = tmpStatBuf.st_size;

        char tmpHash[PATHMAX] = "";
        ConvertHash(currBackupLog->cur->backuplog, tmpHash);

        if (!strcmp(filehash, tmpHash) && !strcmp(currBackupLog->cur->log, path) && (tmpFileSize == fileSize))
        {
          fprintf(stderr, "\"%s\" is already backuped to \"%s\" \n", path, currBackupLog->cur->backuplog);
          return 1;
        }
        currBackupLog = currBackupLog->prev;
      }
    }
    else if (cnt > 4)
    {
      while (true)
      {
        if (currBackupLog->next == NULL)
        {
          break;
        }

        currBackupLog = currBackupLog->next;
      }

      // 역방향 순회로 (최신순 탐색)
      while (true)
      {

        if (currBackupLog == NULL)
        {
          break;
        }

        /*
        [FIX] : 240320 수업에서 교수님이 동일파일인지 비교할떄 해시값비교 & 파일 사이즈 비교 해야한다해서
        로직수정
        */

        struct stat tmpStatBuf;
        if (lstat(currBackupLog->cur->log, &tmpStatBuf) < 0)
        {
          fprintf(stderr, "ERROR: lstat error for %s\n", path);
          return 1;
        }
        tmpFileSize = tmpStatBuf.st_size;

        char tmpHash[PATHMAX] = "";
        ConvertHash(currBackupLog->cur->backuplog, tmpHash);

        if (!strcmp(filehash, tmpHash) && !strcmp(currBackupLog->cur->log, path) && (tmpFileSize == fileSize))
        {
          fprintf(stderr, "\"%s\" is already backuped to \"%s\" \n", path, currBackupLog->cur->backuplog);
          return 1;
        }
        currBackupLog = currBackupLog->prev;
      }
    }

    // 현재 path 임시저장소
    char pathTemp[PATHMAX] = "";
    strcpy(pathTemp, path);

    /*
      루트 디렉토리와 서브 디렉토리에서 실행했을 때 처리
    */

    // exePath에 대한 처리 exePath가 루트 디렉토리 일때와 아닐떄 구분을 해야함
    if (access(tmpdir, F_OK))
      mkdir(tmpdir, 0777);

    // 루트디렉이 아난경우
    char exeTemp[PATHMAX] = "";
    char exeTempDirPath[PATHMAX] = "";
    char tempExeDir[PATHMAX] = "";
    char exeDirPath[PATHMAX] = "";

    strcpy(exeTemp, exePath);
    char *ptrExe = strstr(exeTemp, exePATH);
    if (ptrExe != NULL)
    {
      strcpy(exeTempDirPath, ptrExe + 1 + strlen(exePATH));
    }
    strcat(exeDirPath, tmpdir);
    strcat(exeDirPath, "/");

    // 실행 디렉토리중 가장 상위 디렉토리 잘라내기
    char *slashPos = strchr(exeTempDirPath, '/');

    // 밑에 더 디렉토리가 있을 때
    if (slashPos != NULL)
    {
      size_t totalLength = strlen(exeTempDirPath);

      // '/' 이후까지의 문자열 길이 계산
      size_t dirLength = totalLength - (slashPos - exeTempDirPath + 1);

      char *dir = malloc(dirLength + 1);
      if (dir != NULL)
      {

        strncpy(dir, slashPos + 1, dirLength);
        dir[dirLength] = '\0'; // 널 종료 문자 추가

        if (!strcmp(slashPos, dir))
        {
          strcat(exeDirPath, dir);
          strcat(exeDirPath, "/");
        }

        if (access(exeDirPath, F_OK))
          mkdir(exeDirPath, 0777);
      }

      // / 기준으로 밑에 완전히 잘라내기
      char backupTemp[PATHMAX] = "";
      memmove(exeTempDirPath, slashPos + 1, strlen(slashPos + 1) + 1);
      strcpy(backupTemp, exeTempDirPath);
      // strtok를 사용하여 '/'를 구분자로 사용하여 문자열을 토큰으로 분리
      char *token = strtok(backupTemp, "/");
      strcat(exeDirPath, token);

      // token이 NULL이거나 token,backupTemp가 같다(즉 / 가 없어서 그대로 나온경우)
      if (token == NULL || !strcmp(token, exeTempDirPath))
      {
      }
      else
      {

        if (access(exeDirPath, F_OK))
          mkdir(exeDirPath, 0777);
        while (1)
        {
          token = strtok(NULL, "/"); // 다음 토큰으로 이동
          if (token == NULL)
          {
            break;
          }

          strcat(exeDirPath, "/");
          strcat(exeDirPath, token);
          if (access(exeDirPath, F_OK))
            mkdir(exeDirPath, 0777);
        }
      }
    }
    else
    {
      // 밑에 더 디렉토리가 없는경우 (루트디렉토리였을떄)
      if (slashPos == NULL)
      {
        strcat(exeDirPath, "");
      }
      else
      {

        strcat(exeDirPath, exeTempDirPath);
      }
    }

    /*
    실행 디렉토리 이하에서도 (실제로 백업 되는 부분) 디렉토리가 발견된 경우
    */

    char tmpBackupDirPath[PATHMAX] = "";
    char backupDirPath[PATHMAX] = "";
    strcpy(backupDirPath, exeDirPath);

    if (backupDirPath[strlen(backupDirPath) - 1] != '/')
    {
      backupDirPath[strlen(backupDirPath)] = '/';
    }
    if (access(backupDirPath, F_OK))
      mkdir(backupDirPath, 0777);

    // 매개변수로 받아온 실행경로 만큼 잘라내기
    char *ptrDir = strstr(pathTemp, exePath);
    if (ptrDir != NULL)
    {
      strcpy(tmpBackupDirPath, ptrDir + 1 + strlen(exePath));
    }

    // 백업디렉토리중 가장 상위 디렉토리 잘라내기
    char *slashPos2 = strchr(tmpBackupDirPath, '/');

    // 밑에 더 디렉토리가 있을 때
    if (slashPos2 != NULL)
    {
      // 실행 디렉토리 잘라내도 뎁스가 있다면 그에 해당하게 mkdir해주기
      size_t dirLength = slashPos2 - tmpBackupDirPath; // '/' 이전까지의 문자열 길이 계산
      char *dir = malloc(dirLength + 1);
      if (dir != NULL)
      {
        strncpy(dir, tmpBackupDirPath, dirLength); // '/' 이전까지의 문자열 복사
        dir[dirLength] = '\0';                     // 널 종료 문자 추가

        strcat(backupDirPath, dir);
        strcat(backupDirPath, "/");

        if (access(backupDirPath, F_OK))
          mkdir(backupDirPath, 0777);
      }
      // / 기준으로 밑에 완전히 잘라내기
      char backupTemp[PATHMAX] = "";
      memmove(tmpBackupDirPath, slashPos2 + 1, strlen(slashPos2 + 1) + 1);
      strcpy(backupTemp, tmpBackupDirPath);
      // 맨 뒤 파일 명만 가져오기
      // 파일경로명에 디렉토리명이 포함될경우 삭제
      for (idx = strlen(filename) - 1; idx >= 0; idx--)
      {
        if (filename[idx] == '/')
        {
          break;
        }
      }
      // '/' 문자가 발견된 위치부터 문자열의 끝까지의 길이를 구하기
      len = strlen(filename) - idx - 1;

      strncpy(new_filename, filename + idx + 1, len);
      new_filename[len] = '\0'; // 문자열의 끝을 나타내는 NULL 문자를 추가.

      // 맨 뒤 파일 명 지워내기 /a.txt같은거 (디렉토리만 남게)
      // backupTemp에서 '/' 문자를 뒤에서부터 찾기
      char *lastSlashPos = strrchr(backupTemp, '/');
      if (lastSlashPos != NULL)
      {
        // '/' 문자 이전까지의 문자열만 남기고 삭제
        *lastSlashPos = '\0';
      }

      // strtok를 사용하여 '/'를 구분자로 사용하여 문자열을 토큰으로 분리
      char *token = strtok(backupTemp, "/");
      strcat(backupDirPath, token);

      // new_filename랑 token이 같은경우 (파일명으로 같을것) -> 아래에 디렉토리가 더 없는경우
      if (!strcmp(token, new_filename))
      {
      }
      else
      {
        // 다른 경우 -> 아래에 디렉토리가 더 있음
        if (access(backupDirPath, F_OK))
          mkdir(backupDirPath, 0777);
        while (1)
        {
          token = strtok(NULL, "/"); // 다음 토큰으로 이동
          if (token == NULL)
          {
            break;
          }

          strcat(backupDirPath, "/");
          strcat(backupDirPath, token);

          if (access(backupDirPath, F_OK))
            mkdir(backupDirPath, 0777);
        }

        // 마지막에 파일명 붙여주기!
        strcat(backupDirPath, "/");
        strcat(backupDirPath, new_filename);
      }
    }
    else
    {
      // 밑에 더 디렉토리가 없는경우
      strcat(backupDirPath, tmpBackupDirPath);
    }

    if ((fd1 = open(path, O_RDONLY)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", path);
      return 1;
    }

    if ((fd2 = open(backupDirPath, O_CREAT | O_TRUNC | O_WRONLY, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", backupDirPath);
      return 1;
    }

    while ((len = read(fd1, buf, statbuf.st_size)) > 0)
    {
      write(fd2, buf, len);
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd3 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }
    // ssubagLog에 작성할 로크 스트링 버퍼에 저장
    strcat(logStr, date);
    strcat(logStr, " : ");
    strcat(logStr, "\"");
    strcat(logStr, path);
    strcat(logStr, "\"");
    strcat(logStr, " backuped to ");
    strcat(logStr, "\"");
    strcat(logStr, backupDirPath);
    strcat(logStr, "\"");
    strcat(logStr, "\n");
    if (write(fd3, logStr, strlen(logStr)) == -1)
    {
      fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
      return 1;
    }
    // 메타데이터에 원본경로@백업경로형태로 저장
    if ((fd4 = open(metaPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", metaPATH);
      return 1;
    }
    strcat(metaPath, path);
    strcat(metaPath, "@");
    strcat(metaPath, backupDirPath);
    strcat(metaPath, "\n");

    if (write(fd4, metaPath, strlen(metaPath)) == -1)
    {
      fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
      return 1;
    }

    printf("\"%s\" backuped to \"%s\"\n", path, backupDirPath);
  }

  return 0;
}

// 주어진 디렉토리와 그 하위 항목들을 백업
int BackupDir(char *path, char *date, command_parameter *parameter)
{

  struct dirent **namelist, **subNameList;
  struct stat statbuf;
  int cnt, subCnt, changeDirmod, idx, len;

  if (lstat(path, &statbuf) < 0)
  {
    fprintf(stderr, "ERROR: lstat error for %s\n", path);
    return 1;
  }
  if (!S_ISDIR(statbuf.st_mode))
  {
    fprintf(stderr, "ERROR: %s is not a directory \n", path);
    return 1;
  }
  // path로 받아온 디렉토리의 권한 부여
  if ((changeDirmod = chmod(path, S_IRWXU | S_IRWXG | S_IRWXO)) != 0)
  {
    fprintf(stderr, "ERROR: chmod error for %s\n", path);
    return 1;
  }

  // d옵션만 입력시
  if (parameter->commandopt == OPT_D)
  {
    if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1)
    {
      fprintf(stderr, "ERROR: scandir error for %s\n", path);
      return 1;
    }

    for (int i = 0; i < cnt; i++)
    {
      if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
        continue;
      char tmppath[PATHMAX] = "";
      strcpy(tmppath, path);
      strcat(tmppath, "/");
      strcat(tmppath, namelist[i]->d_name);
      if (lstat(tmppath, &statbuf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", tmppath);
        return 1;
      }

      if (S_ISREG(statbuf.st_mode))
      {
        BackupFileByDir(path, tmppath, date, parameter);
      }
    }

    // namelist 메모리 해제
    for (int idx = 0; idx < cnt; idx++)
    {
      free(namelist[idx]);
    }
    free(namelist);
  }
  // r옵션 or r,d옵션 동시 입력시 or y옵션 입력시
  else if (parameter->commandopt == OPT_R || parameter->commandopt == OPT_Y)
  {

    if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1)
    {
      fprintf(stderr, "ERROR: scandir error for %s\n", path);
      return 1;
    }

    // bfs를 통해 경로를 재귀적 탐색
    queue Queue;
    initQueue(&Queue);
    for (int i = 0; i < cnt; i++)
    {

      if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
        continue;
      struct stat statbuf;
      char enqueuePath[PATHMAX] = "";
      strcat(enqueuePath, path);
      strcat(enqueuePath, "/");
      strcat(enqueuePath, namelist[i]->d_name);
      enqueue(&Queue, enqueuePath);
    }
    for (int i = 0; i < cnt; i++)
    {
      free(namelist[i]);
    }
    free(namelist);
    while (1)
    {
      if (isEmpty(&Queue))
      {
        break;
      }

      char *nodePath = dequeue(&Queue);
      struct stat statbuf;

      if (lstat(nodePath, &statbuf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", nodePath);
        return 1;
      }
      // 파일이라면 그대로 백업을 시켜본다
      if (!S_ISDIR(statbuf.st_mode))
      {
        BackupFileByDir(path, nodePath, date, parameter);
      }
      else
      {
        // 디렉토리라면 현재 디렉토리 하위의 파일이나 폴더를 탐색 후 path부분만 지우고 다시 큐에 넣는다.
        subCnt = 0;

        if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
          return 1;
        }
        for (int i = 0; i < subCnt; i++)
        {
          if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
            continue;
          struct stat statbuf;

          // 새로운 경로를 만들기
          char newPath[PATHMAX] = "";
          char tmpDirPath[PATHMAX] = "";
          strcpy(tmpDirPath, nodePath);

          // 서브디렉토리가 있는경우 만들어주기
          if (lstat(tmpDirPath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", tmpDirPath);
            return 1;
          }

          // 새로 경로 만들기
          char checkTmpPath[PATHMAX] = "";
          strcat(checkTmpPath, nodePath);
          strcat(checkTmpPath, "/");
          strcat(checkTmpPath, subNameList[i]->d_name);
          if (lstat(checkTmpPath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", checkTmpPath);
            return 1;
          }

          // bfs유망성 판단해주기
          // 파일이면 -> 바로 파일 백업 함수로 가게
          if (S_ISREG(statbuf.st_mode))
          {
            BackupFileByDir(path, checkTmpPath, date, parameter);
          }
          // 디렉토리일때만 큐에 넣어준다.
          if (S_ISDIR(statbuf.st_mode))
          {
            enqueue(&Queue, checkTmpPath);
          }
        }
      }
    }
  }
}

int BackupCommand(command_parameter *parameter)
{

  struct stat statbuf;
  char originPath[PATHMAX];
  char newBackupPath[PATHMAX];
  char **backupPathList = NULL;
  int backupPathDepth = 0;
  int i;
  char buf[PATHMAX] = "";

  strcpy(originPath, parameter->filename);

  // lstat을 사용해 디렉토리 속성을 가져옴
  if (lstat(originPath, &statbuf) < 0)
  {
    fprintf(stderr, "ERROR!!: lstat error for %s %s\n", originPath, parameter->filename);
    return 1;
  }
  // 디렉토리인지 파일인지 확인
  if (!S_ISREG(statbuf.st_mode) && !S_ISDIR(statbuf.st_mode))
  {
    fprintf(stderr, "ERROR: %s is not directory or regular file\n", originPath);
    return -1;
  }

  // 디렉토리인데 옵션으로 -d 또는 -r로 지정되지 않았다면 에러 메시지를 출력하고 -1을 반환
  if (S_ISDIR(statbuf.st_mode) && !((parameter->commandopt & OPT_D) || (parameter->commandopt & OPT_R) || ((parameter->commandopt & OPT_Y))))
  {
    fprintf(stderr, "ERROR: %s is a directory file\n", originPath);
    return -1;
  }

  if (S_ISREG(statbuf.st_mode) && ((parameter->commandopt & OPT_D) || (parameter->commandopt & OPT_R)))
  {
    fprintf(stderr, "ERROR: %s is a directory not a regular file \n", originPath);
    return -1;
  }

  // 백업될 경로를 생성 이는 백업 디렉토리인 backupPATH와 사용자의 홈 디렉토리인 homePATH의 차이를 이용하여 상대 경로를 계산
  char *backupDate = getDate();
  // 새로운 backupPath stcat으로 이어주기
  strcat(newBackupPath, backupPATH);
  strcat(newBackupPath, "/");
  strcat(newBackupPath, backupDate);

  if ((backupPathList = GetSubstring(newBackupPath, &backupPathDepth, "/")) == NULL)
  {
    fprintf(stderr, "ERROR: %s can't be backuped\n", originPath);
    return -1;
  }

  // 파일일 경우 BackupFile() 함수를 호출하여 백업을 수행. 디렉토리일 경우 디렉토리 내의 모든 파일과 하위 디렉토리를 백업
  if (S_ISREG(statbuf.st_mode))
  {
    BackupFile(originPath, backupDate, parameter);
  }
  else if (S_ISDIR(statbuf.st_mode))
  {
    mainDirList = (dirList *)malloc(sizeof(dirList));
    dirNode *head = (dirNode *)malloc(sizeof(dirNode));
    mainDirList->head = head;
    dirNode *curr = head->next;
    dirNode *new = (dirNode *)malloc(sizeof(dirNode));

    strcpy(new->path, originPath);
    curr = new;
    mainDirList->tail = curr;

    while (curr != NULL)
    {
      BackupDir(curr->path, backupDate, parameter);
      curr = curr->next;
    }
  }

  return 0;
}

int RemoveFile(char *path, command_parameter *parameter)
{

  struct stat statbuf;
  char originPath[PATHMAX];
  int fd, cnt, candidateIdx, candidateCnt, fd2;
  struct dirent **namelist, **subNameList;
  logList *candidateLoglist = NULL;
  char originHash[PATHMAX];
  char *tmpPath;
  backupList *currBackupLog = mainBackupList;
  char *originFilePath;

  // exepath제외하고 파일경로만 판단
  originFilePath = strstr(originPath, exePATH);
  if (originFilePath != NULL)
  {
    strcpy(originFilePath, originFilePath + 1 + strlen(exePATH));
  }

  if (parameter->commandopt & OPT_A)
  {
    // 모든 백업파일을 삭제한다.
    // 1) 전역 백업 링크드리스트 돌면서 매개변수로 받아온 path와 원본경로를 가지는 path를 찾는다.
    // 2) 원본경로에 대응하는 백업경로를 candidateList에 담는다.
    while (true)
    {

      if (currBackupLog == NULL)
      {
        break;
      }
      if (!strcmp(path, currBackupLog->cur->log))
      {
        // 동일한 경로는 candidateList애 담아주기
        logList *candidateNode = (logList *)malloc(sizeof(logList));
        // 노드 안의 로그 생성
        logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
        memset(candiNodeElement, 0, sizeof(candiNodeElement));
        strcpy(candiNodeElement->backuplog, currBackupLog->cur->backuplog);
        strcpy(candiNodeElement->log, currBackupLog->cur->log);
        candidateNode->cur = candiNodeElement;
        candidateNode->prev = NULL;
        candidateNode->next = NULL;

        // 로그 리스트에 로그 노드 추가
        logList *curr;
        if (candidateLoglist == NULL)
        {
          // mainlogList가 비어 있는 경우
          candidateLoglist = candidateNode;
          candidateCnt++;
        }
        else
        {
          curr = candidateLoglist;
          while (curr->next != NULL)
          {
            curr = curr->next;
            candidateCnt++;
          }
          curr->next = candidateNode;
          candidateNode->prev = curr;
          candidateCnt++;
        }
      }

      currBackupLog = currBackupLog->next;
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd2 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }
    // 후보로 뽑은 리스트을 순회하면서 전부 삭제한다.
    logList *currCandiLog = candidateLoglist;
    while (true)
    {
      if (currCandiLog == NULL)
      {
        break;
      }
      if (remove(currCandiLog->cur->backuplog) < 0)
      {
        fprintf(stderr, "ERROR : remove error %s\n", currCandiLog->cur->backuplog);
        return 1;
      }
      else
      {
        char logStr[PATHMAX * 2] = "";

        printf("\"%s\" removed by \"%s\"\n", currCandiLog->cur->backuplog, path);
        char *logTime = getDate();
        strcat(logStr, logTime);
        strcat(logStr, " : ");
        strcat(logStr, "\"");
        strcat(logStr, currCandiLog->cur->backuplog);
        strcat(logStr, "\"");
        strcat(logStr, " removed by ");
        strcat(logStr, "\"");
        strcat(logStr, path);
        strcat(logStr, "\"");
        strcat(logStr, "\n");

        if (write(fd2, logStr, strlen(logStr)) == -1)
        {
          fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
          return 1;
        }
        // 메타데이터도 지워주기
        if ((clearMetaData(currCandiLog->cur->backuplog)) < 0)
        {
          fprintf(stderr, "Error : clean meta data error\n");
        }
      }

      // 현재 백업디렉토리가 빈폴더라면 삭제하기
      char backupLog[PATHMAX];
      strcpy(backupLog, currCandiLog->cur->backuplog);
      char backupTimeTemp[PATHMAX];
      strcpy(backupTimeTemp, currCandiLog->cur->backuplog);
      char *backuptime = strstr(backupTimeTemp, backupPATH);
      if (backuptime != NULL)
      {
        strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
      }
      // 백업시간만 잘라내기
      char *realBackupTime = strtok(backuptime, "/");

      // currCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
      // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
      char backupTmpPath[PATHMAX] = "";
      strcat(backupTmpPath, currCandiLog->cur->backuplog);
      char *subDirPath;
      char tmpBackDir[PATHMAX] = "";
      strcat(tmpBackDir, backupPATH);
      strcat(tmpBackDir, "/");
      strcat(tmpBackDir, realBackupTime);
      // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

      queue Queue2;
      initQueue(&Queue2);
      enqueue(&Queue2, tmpBackDir);

      while (1)
      {
        if (isEmpty(&Queue2))
        {
          break;
        }
        char *nodePath = dequeue(&Queue2);

        struct stat statbuf;
        if (access(nodePath, F_OK) == 0)
        {
          if (lstat(nodePath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
            return 1;
          }
          // 디렉토리인 애들 중
          // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
          if (S_ISDIR(statbuf.st_mode))
          {

            int subCnt;
            struct dirent **subNameList;
            char prevPath[PATHMAX] = "";

            if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
              return 1;
            }

            if (subCnt == 2)
            {
              if (remove(nodePath) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                return 1;
              }
            }
            else if (subCnt >= 3)
            {

              for (int i = 0; i < subCnt; i++)
              {
                if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                  continue;

                struct stat substatbuf;
                if (lstat(nodePath, &substatbuf) < 0)
                {
                  fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                  return 1;
                }
                if (S_ISDIR(substatbuf.st_mode))
                {
                  strcpy(prevPath, nodePath);

                  strcat(nodePath, "/");
                  strcat(nodePath, subNameList[i]->d_name);

                  // 만약 새로운경로가 디렉이면 큐에 넣고
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    enqueue(&Queue2, nodePath);
                    // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                    if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                    {
                      fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                      return 1;
                    }
                    if (subCnt == 2)
                    {
                      enqueue(&Queue2, prevPath);
                    }
                  }
                }
              }
            }
          }
        }
      }

      // 백업폴더도 지워주기
      int cnt;
      struct dirent **nameList;

      if (access(tmpBackDir, F_OK) == 0)
      {
        if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
          return 1;
        }
        if (cnt == 2)
        {
          if (remove(tmpBackDir) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
            return 1;
          }
        }
      }

      currCandiLog = currCandiLog->next;
    }
  }
  else
  {
    // 1) 전역 백업 링크드리스트 돌면서 매개변수로 받아온 path와 원본경로를 가지는 path를 찾는다.
    // 2) 원본경로에 대응하는 백업경로를 candidateList에 담는다.
    while (true)
    {

      if (currBackupLog == NULL)
      {
        break;
      }
      if (!strcmp(path, currBackupLog->cur->log))
      {
        // 동일한 경로는 candidateList애 담아주기
        logList *candidateNode = (logList *)malloc(sizeof(logList));
        // 노드 안의 로그 생성
        logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
        memset(candiNodeElement, 0, sizeof(candiNodeElement));
        strcpy(candiNodeElement->backuplog, currBackupLog->cur->backuplog);
        strcpy(candiNodeElement->log, currBackupLog->cur->log);
        candidateNode->cur = candiNodeElement;
        candidateNode->prev = NULL;
        candidateNode->next = NULL;

        // 로그 리스트에 로그 노드 추가
        logList *curr;
        if (candidateLoglist == NULL)
        {
          // mainlogList가 비어 있는 경우
          candidateLoglist = candidateNode;
          candidateCnt++;
        }
        else
        {
          curr = candidateLoglist;
          while (curr->next != NULL)
          {
            curr = curr->next;
            candidateCnt++;
          }
          curr->next = candidateNode;
          candidateNode->prev = curr;
          candidateCnt++;
        }
      }

      currBackupLog = currBackupLog->next;
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd2 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }
    // 후보로 뽑은 리스트 돌려주기
    logList *currCandiLog = candidateLoglist;
    if (candidateCnt == 0)
    {
      // 사용자가 입력한 경로가 백업 링크드 리스트에 없는경우
      fprintf(stderr, "ERROR : invalid remove path : %s\n", path);
      return 1;
    }
    else if (candidateCnt == 1)
    {
      if (remove(currCandiLog->cur->backuplog) < 0)
      {
        fprintf(stderr, "ERROR : remove error %s\n", currCandiLog->cur->backuplog);
        return 1;
      }
      else
      {
        char logStr[PATHMAX * 2] = "";

        char *logTime = getDate();
        printf("\"%s\" removed by \"%s\"\n", currCandiLog->cur->backuplog, path);
        strcat(logStr, logTime);
        strcat(logStr, " : ");
        strcat(logStr, "\"");
        strcat(logStr, currCandiLog->cur->backuplog);
        strcat(logStr, "\"");
        strcat(logStr, " removed by ");
        strcat(logStr, "\"");
        strcat(logStr, path);
        strcat(logStr, "\"");
        strcat(logStr, "\n");

        if (write(fd2, logStr, strlen(logStr)) == -1)
        {
          fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
          return 1;
        }
        // 메타데이터도 지워주기
        if ((clearMetaData(currCandiLog->cur->backuplog)) < 0)
        {
          fprintf(stderr, "Error : clean meta data error\n");
        }
      }

      // 현재 백업디렉토리가 빈폴더라면 삭제하기
      char backupLog[PATHMAX];
      strcpy(backupLog, currCandiLog->cur->backuplog);
      char backupTimeTemp[PATHMAX];
      strcpy(backupTimeTemp, currCandiLog->cur->backuplog);
      char *backuptime = strstr(backupTimeTemp, backupPATH);
      if (backuptime != NULL)
      {
        strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
      }
      // 백업시간만 잘라내기
      char *realBackupTime = strtok(backuptime, "/");

      // currCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
      //  맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
      char backupTmpPath[PATHMAX] = "";
      strcat(backupTmpPath, currCandiLog->cur->backuplog);
      char *subDirPath;
      char tmpBackDir[PATHMAX] = "";
      strcat(tmpBackDir, backupPATH);
      strcat(tmpBackDir, "/");
      strcat(tmpBackDir, realBackupTime);
      // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

      queue Queue2;
      initQueue(&Queue2);
      enqueue(&Queue2, tmpBackDir);

      while (1)
      {
        if (isEmpty(&Queue2))
        {
          break;
        }
        char *nodePath = dequeue(&Queue2);

        struct stat statbuf;
        if (access(nodePath, F_OK) == 0)
        {
          if (lstat(nodePath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
            return 1;
          }
          // 디렉토리인 애들 중
          // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
          if (S_ISDIR(statbuf.st_mode))
          {

            int subCnt;
            struct dirent **subNameList;
            char prevPath[PATHMAX] = "";

            if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
              return 1;
            }

            if (subCnt == 2)
            {
              if (remove(nodePath) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                return 1;
              }
            }
            else if (subCnt >= 3)
            {

              for (int i = 0; i < subCnt; i++)
              {
                if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                  continue;

                struct stat substatbuf;
                if (lstat(nodePath, &substatbuf) < 0)
                {
                  fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                  return 1;
                }
                if (S_ISDIR(substatbuf.st_mode))
                {
                  strcpy(prevPath, nodePath);

                  strcat(nodePath, "/");
                  strcat(nodePath, subNameList[i]->d_name);

                  // 만약 새로운경로가 디렉이면 큐에 넣고
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    enqueue(&Queue2, nodePath);
                    // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                    if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                    {
                      fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                      return 1;
                    }
                    if (subCnt == 2)
                    {
                      enqueue(&Queue2, prevPath);
                    }
                  }
                }
              }
            }
          }
        }
      }

      // 백업폴더도 지워주기
      int cnt;
      struct dirent **nameList;

      if (access(tmpBackDir, F_OK) == 0)
      {
        if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
          return 1;
        }
        if (cnt == 2)
        {
          if (remove(tmpBackDir) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
            return 1;
          }
        }
      }
    }
    else if (candidateCnt > 1)
    {

      printf("backup files of %s\n", path);
      printf("0. exit\n");
      int iidx = 1;
      while (true)
      {
        if (currCandiLog == NULL)
        {
          break;
        }
        char backupLog[PATHMAX] = "";
        strcpy(backupLog, currCandiLog->cur->backuplog);
        char backupTimeTemp[PATHMAX] = "";
        strcpy(backupTimeTemp, currCandiLog->cur->backuplog);
        char *backuptime = strstr(backupTimeTemp, backupPATH);
        if (backuptime != NULL)
        {
          strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
        }
        // 백업시간만 잘라내기
        char *realBackupTime = strtok(backuptime, "/");
        // 파일 크기 알아내기
        struct stat file_stat;
        if (stat(currCandiLog->cur->backuplog, &file_stat) == -1)
        {
          fprintf(stderr, "ERROR: stat error\n");
          return 1;
        }
        printf("%d. %s     %lldbytes\n", iidx, realBackupTime, (long long)file_stat.st_size);

        currCandiLog = currCandiLog->next;
        iidx++;
      }

      // 유저의 입력받기
      int userInputIdx;
      printf(">>");
      scanf("%d", &userInputIdx);
      if (userInputIdx == 0)
      {
        exit(1);
      }

      // 유저입력과 같은 번째의 노드인경우 삭제 갈기기
      iidx = 1;
      currCandiLog = candidateLoglist;
      while (true)
      {
        if (currCandiLog == NULL)
        {
          break;
        }
        if (iidx == userInputIdx)
        {
          // 삭제하기
          if (remove(currCandiLog->cur->backuplog) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", currCandiLog->cur->backuplog);
            return 1;
          }
          else
          {
            char logStr[PATHMAX * 2] = "";
            char *backupTime = splitBackupDate(currCandiLog->cur->backuplog);
            printf("\"%s\" removed by \"%s\"\n", currCandiLog->cur->backuplog, path);
            strcat(logStr, backupTime);
            strcat(logStr, " : ");
            strcat(logStr, "\"");
            strcat(logStr, currCandiLog->cur->backuplog);
            strcat(logStr, "\"");
            strcat(logStr, " removed by ");
            strcat(logStr, "\"");
            strcat(logStr, path);
            strcat(logStr, "\"");
            strcat(logStr, "\n");
            // 로그파일에 작성
            if (write(fd2, logStr, strlen(logStr)) == -1)
            {
              fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
              return 1;
            }
          }
          // 메타데이터도 지워주기
          if ((clearMetaData(currCandiLog->cur->backuplog)) < 0)
          {
            fprintf(stderr, "Error : clean meta data error\n");
          }

          // 현재 백업디렉토리가 빈폴더라면 삭제하기
          char backupLog[PATHMAX];
          strcpy(backupLog, currCandiLog->cur->backuplog);
          char backupTimeTemp[PATHMAX];
          strcpy(backupTimeTemp, currCandiLog->cur->backuplog);
          char *backuptime = strstr(backupTimeTemp, backupPATH);
          if (backuptime != NULL)
          {
            strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
          }
          // 백업시간만 잘라내기
          char *realBackupTime = strtok(backuptime, "/");

          // currCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
          // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
          char backupTmpPath[PATHMAX] = "";
          strcat(backupTmpPath, currCandiLog->cur->backuplog);
          char *subDirPath;
          char tmpBackDir[PATHMAX] = "";
          strcat(tmpBackDir, backupPATH);
          strcat(tmpBackDir, "/");
          strcat(tmpBackDir, realBackupTime);
          // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

          queue Queue2;
          initQueue(&Queue2);
          enqueue(&Queue2, tmpBackDir);

          while (1)
          {
            if (isEmpty(&Queue2))
            {
              break;
            }
            char *nodePath = dequeue(&Queue2);

            struct stat statbuf;
            if (access(nodePath, F_OK) == 0)
            {
              if (lstat(nodePath, &statbuf) < 0)
              {
                fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
                return 1;
              }
              // 디렉토리인 애들 중
              // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
              if (S_ISDIR(statbuf.st_mode))
              {

                int subCnt;
                struct dirent **subNameList;
                char prevPath[PATHMAX] = "";

                if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                {
                  fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                  return 1;
                }

                if (subCnt == 2)
                {
                  if (remove(nodePath) < 0)
                  {
                    fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                    return 1;
                  }
                }
                else if (subCnt >= 3)
                {

                  for (int i = 0; i < subCnt; i++)
                  {
                    if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                      continue;

                    struct stat substatbuf;
                    if (lstat(nodePath, &substatbuf) < 0)
                    {
                      fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                      return 1;
                    }
                    if (S_ISDIR(substatbuf.st_mode))
                    {
                      strcpy(prevPath, nodePath);

                      strcat(nodePath, "/");
                      strcat(nodePath, subNameList[i]->d_name);

                      // 만약 새로운경로가 디렉이면 큐에 넣고
                      if (lstat(nodePath, &substatbuf) < 0)
                      {
                        fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                        return 1;
                      }
                      if (S_ISDIR(substatbuf.st_mode))
                      {
                        enqueue(&Queue2, nodePath);
                        // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                        if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                        {
                          fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                          return 1;
                        }
                        if (subCnt == 2)
                        {
                          enqueue(&Queue2, prevPath);
                        }
                      }
                    }
                  }
                }
              }
            }
          }

          // 백업폴더도 지워주기
          int cnt;
          struct dirent **nameList;

          if (access(tmpBackDir, F_OK) == 0)
          {
            if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
              return 1;
            }
            if (cnt == 2)
            {
              if (remove(tmpBackDir) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
                return 1;
              }
            }
          }
        }
        currCandiLog = currCandiLog->next;
        iidx++;
      }
    }
  }

  return 0;
}

// 전역 removeCandidateList

logList *removeCandidateLoglist = NULL;
// 2) RemoveFileByDir에서 전역 removeCandidateLoglist 만들어서 해당 로그를 저장한다.
int RemoveFileByDir(char *path, char *backupPath, command_parameter *parameter)
{

  char backupTemp[PATHMAX] = "";
  char backupDirTmp[PATHMAX] = "";
  char backupDateTmp[PATHMAX] = "";
  char pathTemp[PATHMAX] = "";
  logList *candidateNode = (logList *)malloc(sizeof(logList));
  // backupPath에서 backupPATH제외하고 백업폴더 제외하고 남는 부분을 path뒤에 strcat해준다.
  strcpy(backupTemp, backupPath);
  strcpy(backupDateTmp, backupPath);
  strcpy(pathTemp, path);

  char *ptrBack = strstr(backupTemp, backupPATH);
  if (ptrBack != NULL)
  {
    // backupPATH 문자열의 길이만큼을 더해, 그 뒤의 문자열을 가리키게
    char *startCopyPosition = ptrBack + strlen(backupPATH);
    strcpy(backupDirTmp, startCopyPosition);
  }

  char *backupDate = splitBackupDate(backupDateTmp);

  char *ptrBack2 = strstr(backupTemp, backupDate);
  if (ptrBack2 != NULL)
  {
    // backupPATH 문자열의 길이만큼을 더해, 그 뒤의 문자열을 가리키게
    char *startCopyPosition2 = ptrBack2 + strlen(backupDate);
    strcpy(backupDirTmp, startCopyPosition2);
  }

  strcat(pathTemp, backupDirTmp);

  // 노드 안의 로그 생성
  logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
  memset(candiNodeElement, 0, sizeof(candiNodeElement));
  strcpy(candiNodeElement->backuplog, backupPath);
  strcpy(candiNodeElement->log, pathTemp);
  candidateNode->cur = candiNodeElement;
  candidateNode->prev = NULL;
  candidateNode->next = NULL;

  // 로그 리스트에 로그 노드 추가
  logList *curr;
  if (removeCandidateLoglist == NULL)
  {
    // removeCandidateLoglist가 비어 있는 경우
    removeCandidateLoglist = candidateNode;
  }
  else
  {
    curr = removeCandidateLoglist;
    while (curr->next != NULL)
    {
      curr = curr->next;
    }
    curr->next = candidateNode;
    candidateNode->prev = curr;
  }

  return 0;
}

int RemoveDir(char *path, command_parameter *parameter)
{

  struct stat statbuf;
  char originPath[PATHMAX];
  int fd, cnt, candidateIdx, fd2, subCnt;
  int removeCnt = 0;
  int candidateCnt = 0;
  struct dirent **namelist, **subNameList;
  char originHash[PATHMAX];
  char *tmpPath;
  char *originFilePath;

  // d옵션인경우
  // 0) 전역 백업 링크드 리스트를 돈다
  // 1) 유저가 입력한 디렉토리 절대경로와 문자열의 맨앞에서부터 유저가 작성한 절대경로의 문자열 길이 만큼 같은지 판단하고
  // 2) 해당 날짜에 해당하는 백업 폴더로 가서
  // 2) Regfile인애들만 remove

  if (parameter->commandopt == OPT_D)
  {
    char pathTemp[PATHMAX] = "";
    char tmpBackupDirPath[PATHMAX] = "";
    strcpy(pathTemp, path);

    backupList *currBackupLog = mainBackupList;
    logList *candidateLoglist = NULL;
    char *backupDate;

    // 0) 전역 백업 링크드 리스트를 돈다
    while (true)
    {
      if (currBackupLog == NULL)
      {
        break;
      }

      // 1) 유저가 입력한 디렉토리 절대경로와 로그에서 마지막 파일명만 제거한 문자열이 같으면
      char *extractLogPath = extractPath(currBackupLog->cur->log);
      // 2) 해당 날짜에 해당하는 백업 폴더로 가서

      if (!strncmp(extractLogPath, path, strlen(path)))
      {

        backupDate = splitBackupDate(currBackupLog->cur->backuplog);

        char backupDirPath[PATHMAX] = "";
        strcat(backupDirPath, backupPATH);
        strcat(backupDirPath, "/");
        strcat(backupDirPath, backupDate);

        // 매개변수로 받아온 path에서 최상위 루트디렉토리만 삭제해서 strcat하고 큐에 넣어준다.
        // 매개변수로 받아온 실행경로 만큼 잘라내기
        char *ptrDir = strstr(pathTemp, exePATH);
        if (ptrDir != NULL)
        {
          strcpy(tmpBackupDirPath, ptrDir + 1 + strlen(exePATH));
        }
        char slashRootDir[PATHMAX] = "";
        strcpy(slashRootDir, tmpBackupDirPath);
        // 백업디렉토리중 가장 상위 디렉토리 잘라내기
        char *slashPosition = strchr(slashRootDir, '/');

        if (slashPosition != NULL)
        {
          memmove(slashRootDir, slashPosition + 1, strlen(slashPosition));
        }

        if (strcmp(tmpBackupDirPath, slashRootDir))
        {
          strcat(backupDirPath, "/");
          strcat(backupDirPath, slashRootDir);
        }

        if ((cnt = scandir(backupDirPath, &namelist, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", backupDirPath);
          return 1;
        }

        for (int i = 0; i < cnt; i++)
        {

          if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
            continue;
          char subTemp[PATHMAX] = "";
          strcat(subTemp, backupDirPath);
          strcat(subTemp, "/");
          strcat(subTemp, namelist[i]->d_name);

          if (lstat(subTemp, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", subTemp);
            return 1;
          }
          // 2) Regfile인애들만 remove
          if (!S_ISDIR(statbuf.st_mode) && !strcmp(subTemp, currBackupLog->cur->backuplog))
          {

            logList *candidateNode = (logList *)malloc(sizeof(logList));
            // 노드 안의 로그 생성
            logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
            memset(candiNodeElement, 0, sizeof(candiNodeElement));
            strcpy(candiNodeElement->backuplog, currBackupLog->cur->backuplog);
            strcpy(candiNodeElement->log, currBackupLog->cur->log);
            candidateNode->cur = candiNodeElement;
            candidateNode->prev = NULL;
            candidateNode->next = NULL;

            // 로그 리스트에 로그 노드 추가
            logList *curr;
            if (candidateLoglist == NULL)
            {
              // mainlogList가 비어 있는 경우
              candidateLoglist = candidateNode;
            }
            else
            {
              curr = candidateLoglist;
              while (curr->next != NULL)
              {
                curr = curr->next;
              }
              curr->next = candidateNode;
              candidateNode->prev = curr;
            }

            break;
          }
        }
      }

      currBackupLog = currBackupLog->next;
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd2 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }

    // 3)삭제할 애들이 모여있는 리스트

    logList *currCandiLog = candidateLoglist;
    bool exitFlag = false;
    while (true)
    {
      if (currCandiLog == NULL || currCandiLog->cur->log == NULL)
      {
        break;
      }
      // removeCandidatLoglist의 최신상태를 반영할 수 있게 (지우면 세그폴트 뜸!!!!)
      currCandiLog = candidateLoglist;

      // 반복문 안에서 똑같은 원본 파일이 있는지 확인하는 리스트를 만든다.
      logList *subCandiList = NULL;
      logList *subPos = candidateLoglist;
      while (true)
      {
        if (subPos == NULL)
        {
          break;
        }
        if (!strcmp(subPos->cur->log, currCandiLog->cur->log))
        {

          logList *candidateNode = (logList *)malloc(sizeof(logList));
          // 노드 안의 로그 생성
          logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
          memset(candiNodeElement, 0, sizeof(candiNodeElement));
          strcpy(candiNodeElement->backuplog, subPos->cur->backuplog);
          strcpy(candiNodeElement->log, subPos->cur->log);
          candidateNode->cur = candiNodeElement;
          candidateNode->prev = NULL;
          candidateNode->next = NULL;

          // 로그 리스트에 로그 노드 추가
          logList *curr;
          if (subCandiList == NULL)
          {
            // mainlogList가 비어 있는 경우
            subCandiList = candidateNode;
          }
          else
          {
            curr = subCandiList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = candidateNode;
            candidateNode->prev = curr;
          }
        }
        subPos = subPos->next;
      }

      // 서브 리스트 속에 원소가 몇개인지 센다
      logList *subCandiLog = subCandiList;
      int subPosCnt = 0;
      while (true)
      {
        if (subCandiLog == NULL)
        {
          break;
        }

        subPosCnt++;
        subCandiLog = subCandiLog->next;
      }

      subCandiLog = subCandiList;

      if (subPosCnt == 1)
      {
        if (remove(currCandiLog->cur->backuplog) < 0)
        {
          fprintf(stderr, "ERROR : remove error %s\n", subCandiLog->cur->backuplog);
          return 1;
        }
        else
        {

          char logStr[PATHMAX * 2] = "";
          char *logTime = getDate();
          printf("\"%s\" removed by \"%s\"\n", subCandiLog->cur->backuplog, path);
          strcat(logStr, logTime);
          strcat(logStr, " : ");
          strcat(logStr, "\"");
          strcat(logStr, subCandiLog->cur->backuplog);
          strcat(logStr, "\"");
          strcat(logStr, " removed by ");
          strcat(logStr, "\"");
          strcat(logStr, path);
          strcat(logStr, "\"");
          strcat(logStr, "\n");

          if (write(fd2, logStr, strlen(logStr)) == -1)
          {
            fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
            return 1;
          }
          // 메타데이터도 지워주기
          if ((clearMetaData(subCandiLog->cur->backuplog)) < 0)
          {
            fprintf(stderr, "Error : clean meta data error\n");
          }
        }

        // 현재 백업디렉토리가 빈폴더라면 삭제하기

        char backupLog[PATHMAX];
        strcpy(backupLog, subCandiLog->cur->backuplog);
        char backupTimeTemp[PATHMAX];
        strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
        char *backuptime = strstr(backupTimeTemp, backupPATH);
        if (backuptime != NULL)
        {
          strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
        }
        // 백업시간만 잘라내기
        char *realBackupTime = strtok(backuptime, "/");

        // subCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
        // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
        char backupTmpPath[PATHMAX] = "";
        strcat(backupTmpPath, subCandiLog->cur->backuplog);
        char *subDirPath;
        char tmpBackDir[PATHMAX] = "";
        strcat(tmpBackDir, backupPATH);
        strcat(tmpBackDir, "/");
        strcat(tmpBackDir, realBackupTime);
        // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

        queue Queue2;
        initQueue(&Queue2);
        enqueue(&Queue2, tmpBackDir);

        while (1)
        {
          if (isEmpty(&Queue2))
          {
            break;
          }
          char *nodePath = dequeue(&Queue2);

          struct stat statbuf;
          if (access(nodePath, F_OK) == 0)
          {
            if (lstat(nodePath, &statbuf) < 0)
            {
              fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
              return 1;
            }
            // 디렉토리인 애들 중
            // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
            if (S_ISDIR(statbuf.st_mode))
            {

              int subCnt;
              struct dirent **subNameList;
              char prevPath[PATHMAX] = "";

              if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
              {
                fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                return 1;
              }

              if (subCnt == 2)
              {
                if (remove(nodePath) < 0)
                {
                  fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                  return 1;
                }
              }
              else if (subCnt >= 3)
              {

                for (int i = 0; i < subCnt; i++)
                {
                  if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                    continue;

                  struct stat substatbuf;
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    strcpy(prevPath, nodePath);

                    strcat(nodePath, "/");
                    strcat(nodePath, subNameList[i]->d_name);

                    // 만약 새로운경로가 디렉이면 큐에 넣고
                    if (lstat(nodePath, &substatbuf) < 0)
                    {
                      fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                      return 1;
                    }
                    if (S_ISDIR(substatbuf.st_mode))
                    {
                      enqueue(&Queue2, nodePath);
                      // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                      if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                      {
                        fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                        return 1;
                      }
                      if (subCnt == 2)
                      {
                        enqueue(&Queue2, prevPath);
                      }
                    }
                  }
                }
              }
            }
          }
        }

        // 백업폴더도 지워주기
        int cnt;
        struct dirent **nameList;

        if (access(tmpBackDir, F_OK) == 0)
        {
          if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
          {
            fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
            return 1;
          }
          if (cnt == 2)
          {
            if (remove(tmpBackDir) < 0)
            {
              fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
              return 1;
            }
          }
        }
        // candiLogList에서도 삭제 반영
        removeBackuplog(&candidateLoglist, subCandiLog->cur->backuplog);
        removeLogAndUpdateList(&candidateLoglist, subCandiLog->cur->log);
      }
      else if (subPosCnt > 1)
      {
        // 주의! 서브 리스트 안에서 삭제를 해야함!
        printf("remove files of %s\n", subCandiLog->cur->log);
        printf("0. exit\n");
        int iidx = 1;
        while (true)
        {

          if (subCandiLog == NULL)
          {
            break;
          }

          char backupLog[PATHMAX] = "";
          strcpy(backupLog, subCandiLog->cur->backuplog);
          char backupTimeTemp[PATHMAX] = "";
          strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
          char *backuptime = strstr(backupTimeTemp, backupPATH);
          if (backuptime != NULL)
          {
            strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
          }
          // 백업시간만 잘라내기
          char *realBackupTime = strtok(backuptime, "/");
          // 파일 크기 알아내기
          struct stat file_stat;
          if (stat(subCandiLog->cur->backuplog, &file_stat) == -1)
          {
            fprintf(stderr, "ERROR: stat error\n");
            return 1;
          }
          printf("%d. %s     %lldbytes\n", iidx, realBackupTime, (long long)file_stat.st_size);

          subCandiLog = subCandiLog->next;
          iidx++;
        }

        // 유저의 입력받기
        int userInputIdx;
        printf(">>");
        scanf("%d", &userInputIdx);
        if (userInputIdx == 0)
        {
          exit(1);
        }

        // 유저입력과 같은 번째의 노드인경우 삭제 갈기기
        iidx = 1;
        // 서브 리스트 안에서 삭제 해야하기 때문에 subCandiList를 가리키는 포인터
        subCandiLog = subCandiList;
        while (1)
        {
          if (subCandiLog == NULL)
          {
            break;
          }

          if (iidx == userInputIdx)
          {

            // 삭제하기
            if (remove(subCandiLog->cur->backuplog) < 0)
            {
              fprintf(stderr, "ERROR : remove error %s\n", subCandiLog->cur->backuplog);
              return 1;
            }
            else
            {

              char logStr[PATHMAX * 2] = "";
              char *logTime = getDate();
              printf("\"%s\" removed by \"%s\"\n", subCandiLog->cur->backuplog, path);
              strcat(logStr, logTime);
              strcat(logStr, " : ");
              strcat(logStr, "\"");
              strcat(logStr, subCandiLog->cur->backuplog);
              strcat(logStr, "\"");
              strcat(logStr, " removed by ");
              strcat(logStr, "\"");
              strcat(logStr, path);
              strcat(logStr, "\"");
              strcat(logStr, "\n");
              // 로그파일에 작성
              if (write(fd2, logStr, strlen(logStr)) == -1)
              {
                fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
                return 1;
              }
            }
            // 메타데이터도 지워주기
            if ((clearMetaData(subCandiLog->cur->backuplog)) < 0)
            {
              fprintf(stderr, "Error : clean meta data error\n");
            }

            // 현재 백업디렉토리가 빈폴더라면 삭제하기
            char backupLog[PATHMAX];
            strcpy(backupLog, subCandiLog->cur->backuplog);
            char backupTimeTemp[PATHMAX];
            strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
            char *backuptime = strstr(backupTimeTemp, backupPATH);
            if (backuptime != NULL)
            {
              strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
            }
            // 백업시간만 잘라내기
            char *realBackupTime = strtok(backuptime, "/");

            // subCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
            // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
            char backupTmpPath[PATHMAX] = "";
            strcat(backupTmpPath, subCandiLog->cur->backuplog);
            char *subDirPath;
            char tmpBackDir[PATHMAX] = "";
            strcat(tmpBackDir, backupPATH);
            strcat(tmpBackDir, "/");
            strcat(tmpBackDir, realBackupTime);
            // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

            queue Queue2;
            initQueue(&Queue2);
            enqueue(&Queue2, tmpBackDir);

            while (1)
            {
              if (isEmpty(&Queue2))
              {
                break;
              }
              char *nodePath = dequeue(&Queue2);

              struct stat statbuf;
              if (access(nodePath, F_OK) == 0)
              {
                if (lstat(nodePath, &statbuf) < 0)
                {
                  fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
                  return 1;
                }
                // 디렉토리인 애들 중
                // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
                if (S_ISDIR(statbuf.st_mode))
                {

                  int subCnt;
                  struct dirent **subNameList;
                  char prevPath[PATHMAX] = "";

                  if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                  {
                    fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                    return 1;
                  }

                  if (subCnt == 2)
                  {
                    if (remove(nodePath) < 0)
                    {
                      fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                      return 1;
                    }
                  }
                  else if (subCnt >= 3)
                  {

                    for (int i = 0; i < subCnt; i++)
                    {
                      if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                        continue;

                      struct stat substatbuf;
                      if (lstat(nodePath, &substatbuf) < 0)
                      {
                        fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                        return 1;
                      }
                      if (S_ISDIR(substatbuf.st_mode))
                      {
                        strcpy(prevPath, nodePath);

                        strcat(nodePath, "/");
                        strcat(nodePath, subNameList[i]->d_name);

                        // 만약 새로운경로가 디렉이면 큐에 넣고
                        if (lstat(nodePath, &substatbuf) < 0)
                        {
                          fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                          return 1;
                        }
                        if (S_ISDIR(substatbuf.st_mode))
                        {
                          enqueue(&Queue2, nodePath);
                          // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                          if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                          {
                            fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                            return 1;
                          }
                          if (subCnt == 2)
                          {
                            enqueue(&Queue2, prevPath);
                          }
                        }
                      }
                    }
                  }
                }
              }
            }

            // 백업폴더도 지워주기
            int cnt;
            struct dirent **nameList;

            if (access(tmpBackDir, F_OK) == 0)
            {
              if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
              {
                fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
                return 1;
              }
              if (cnt == 2)
              {
                if (remove(tmpBackDir) < 0)
                {
                  fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
                  return 1;
                }
              }
            }
            // 백업로그 먼저 삭제

            removeBackuplog(&candidateLoglist, subCandiLog->cur->backuplog);
            // candidateLoglist에도 반영
            removeLogAndUpdateList(&candidateLoglist, subCandiLog->cur->log);
            break;
          }
          else
          {
            subCandiLog = subCandiLog->next;
            iidx++;
          }
        }
      }

      if (currCandiLog == NULL)
      {
        break;
      }
      else
      {
        currCandiLog = currCandiLog->next;
      }
    }
  }

  // r옵션인경우
  // 0) 전역 백업 링크드 리스트를 돈다
  // 1) 유저가 입력한 디렉토리 절대경로와 백업경로가 문자열의 맨앞에서부터 유저가 작성한 절대경로의 문자열 길이 만큼 같은지 판단하고
  // 2) 맞다면 거기서 backupDate가져와서 유저가 입력한 경로에서 실행디렉 잘라내고 backupPATH/백업날짜/실행디렉 잘라낸 유저가 입력한 경로 -> 이렇게 새로운 백업경로 생성
  // 3) 새롭게 만든 백업경로에서 namelist strcat시키면서 디렉이면 경로자체를 큐에넣고 파일이면 안넣는다.

  else if (parameter->commandopt == OPT_R)
  {

    char pathTemp[PATHMAX] = "";
    char tmpBackupDirPath[PATHMAX] = "";
    backupList *currBackupLog = mainBackupList;
    logList *candidateLoglist = NULL;
    // 큐에 넣을 애들 중 중복검사
    logList *enqueueLogList = NULL;
    char *backupDate;
    queue Queue;
    initQueue(&Queue);
    strcpy(pathTemp, path);

    // 1) 전역 백업 링크드 리스트를 돈다
    while (true)
    {
      if (currBackupLog == NULL)
      {
        break;
      }

      // 1) 유저가 입력한 디렉토리 절대경로와 로그에서 마지막 파일명만 제거한 문자열이 같으면
      char *extractLogPath = extractPath(currBackupLog->cur->log);
      // 2) 해당 날짜에 해당하는 백업 폴더로 가서
      if (!strncmp(extractLogPath, path, strlen(path)))
      {

        backupDate = splitBackupDate(currBackupLog->cur->backuplog);

        char backupDirPath[PATHMAX] = "";
        strcat(backupDirPath, backupPATH);
        strcat(backupDirPath, "/");
        strcat(backupDirPath, backupDate);

        // 매개변수로 받아온 path에서 최상위 루트디렉토리만 삭제해서 strcat하고 큐에 넣어준다.
        // 매개변수로 받아온 실행경로 만큼 잘라내기
        char *ptrDir = strstr(pathTemp, exePATH);
        if (ptrDir != NULL)
        {
          strcpy(tmpBackupDirPath, ptrDir + 1 + strlen(exePATH));
        }
        char slashRootDir[PATHMAX] = "";
        strcpy(slashRootDir, tmpBackupDirPath);
        // 백업디렉토리중 가장 상위 디렉토리 잘라내기
        char *slashPosition = strchr(slashRootDir, '/');

        if (slashPosition != NULL)
        {
          memmove(slashRootDir, slashPosition + 1, strlen(slashPosition));
        }

        if (strcmp(tmpBackupDirPath, slashRootDir))
        {
          strcat(backupDirPath, "/");
          strcat(backupDirPath, slashRootDir);
        }

        logList *enqueueLog = enqueueLogList;
        bool enqueueFlag = true;
        while (true)
        {
          if (enqueueLog == NULL)
          {

            break;
          }
          if (!strcmp(enqueueLog->cur->backuplog, backupDirPath))
          {
            enqueueFlag = false;
          }
          enqueueLog = enqueueLog->next;
        }

        if (enqueueFlag)
        {
          logList *candidateNode = (logList *)malloc(sizeof(logList));
          // 노드 안의 로그 생성
          logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
          memset(candiNodeElement, 0, sizeof(candiNodeElement));
          strcpy(candiNodeElement->backuplog, backupDirPath);
          strcpy(candiNodeElement->log, currBackupLog->cur->log);
          candidateNode->cur = candiNodeElement;
          candidateNode->prev = NULL;
          candidateNode->next = NULL;

          // 로그 리스트에 로그 노드 추가
          logList *curr;
          if (enqueueLogList == NULL)
          {
            // removeCandidateLoglist가 비어 있는 경우
            enqueueLogList = candidateNode;
          }
          else
          {
            curr = enqueueLogList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = candidateNode;
            candidateNode->prev = curr;
          }
        }
      }

      currBackupLog = currBackupLog->next;
    }

    // 3-1 )중복 제거해서 큐에 넣는다.
    logList *enqueueLog = enqueueLogList;
    while (true)
    {
      if (enqueueLog == NULL)
      {
        break;
      }
      enqueue(&Queue, enqueueLog->cur->backuplog);
      enqueueLog = enqueueLog->next;
    }

    // 3-2) 큐가 다 비어있을때까지 반복

    while (1)
    {
      if (isEmpty(&Queue))
      {
        break;
      }

      char *nodePath = dequeue(&Queue);
      struct stat statbuf;
      if (lstat(nodePath, &statbuf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", nodePath);
        return 1;
      }
      // 파일이라면 그대로 백업을 시켜본다
      if (!S_ISDIR(statbuf.st_mode))
      {
        RemoveFileByDir(path, nodePath, parameter);
      }
      else
      {
        // 디렉토리라면 현재 디렉토리 하위의 파일이나 폴더를 탐색 후 path부분만 지우고 다시 큐에 넣는다.
        subCnt = 0;

        if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
          return 1;
        }
        for (int i = 0; i < subCnt; i++)
        {
          if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
            continue;
          struct stat statbuf;

          // 새로운 경로를 만들기
          char newPath[PATHMAX] = "";
          char tmpDirPath[PATHMAX] = "";
          strcpy(tmpDirPath, nodePath);

          // 서브디렉토리가 있는경우 만들어주기
          if (lstat(tmpDirPath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", tmpDirPath);
            return 1;
          }

          // 새로 경로 만들기
          char checkTmpPath[PATHMAX] = "";
          strcat(checkTmpPath, nodePath);
          strcat(checkTmpPath, "/");
          strcat(checkTmpPath, subNameList[i]->d_name);

          if (lstat(checkTmpPath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", checkTmpPath);
            return 1;
          }

          // bfs유망성 판단해주기
          // 파일이면 -> 바로 파일 백업 함수로 가게
          if (S_ISREG(statbuf.st_mode))
          {
            RemoveFileByDir(path, checkTmpPath, parameter);
          }
          // 디렉토리일때만 큐에 넣어준다.
          if (S_ISDIR(statbuf.st_mode))
          {
            enqueue(&Queue, checkTmpPath);
          }
        }
      }
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd2 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }

    // 4)  RemoveFileDir에서 bfs하면서 전역 링크드 리스트에 저장시켜놓은 애들을 다시 가져와서 진짜 삭제하기!
    logList *currRemoveLog = removeCandidateLoglist;
    while (true)
    {
      if (currRemoveLog == NULL || currRemoveLog->cur->log == NULL)
      {
        break;
      }
      // removeCandidatLoglist의 최신상태를 반영할 수 있게 (지우면 세그폴트 뜸!!!!)
      currRemoveLog = removeCandidateLoglist;
      // 반복문 안에서 똑같은 원본 파일이 있는지 확인하는 리스트를 만든다.
      logList *subCandiList = NULL;
      logList *subPos = removeCandidateLoglist;
      while (true)
      {
        if (subPos == NULL)
        {
          break;
        }

        if (!strcmp(subPos->cur->log, currRemoveLog->cur->log))
        {

          logList *candidateNode = (logList *)malloc(sizeof(logList));
          // 노드 안의 로그 생성
          logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
          memset(candiNodeElement, 0, sizeof(candiNodeElement));
          strcpy(candiNodeElement->backuplog, subPos->cur->backuplog);
          strcpy(candiNodeElement->log, subPos->cur->log);
          candidateNode->cur = candiNodeElement;
          candidateNode->prev = NULL;
          candidateNode->next = NULL;

          // 로그 리스트에 로그 노드 추가
          logList *curr;
          if (subCandiList == NULL)
          {
            // mainlogList가 비어 있는 경우
            subCandiList = candidateNode;
          }
          else
          {
            curr = subCandiList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = candidateNode;
            candidateNode->prev = curr;
          }
        }
        subPos = subPos->next;
      }

      // 서브 리스트 속에 원소가 몇개인지 센다
      logList *subCandiLog = subCandiList;
      int subPosCnt = 0;
      while (true)
      {
        if (subCandiLog == NULL)
        {
          break;
        }

        subPosCnt++;
        subCandiLog = subCandiLog->next;
      }

      subCandiLog = subCandiList;

      if (subPosCnt == 1)
      {
        if (remove(currRemoveLog->cur->backuplog) < 0)
        {
          fprintf(stderr, "ERROR : remove error %s\n", subCandiLog->cur->backuplog);
          return 1;
        }
        else
        {

          char logStr[PATHMAX * 2] = "";
          char *logTime = getDate();
          printf("\"%s\" removed by \"%s\"\n", subCandiLog->cur->backuplog, path);
          strcat(logStr, logTime);
          strcat(logStr, " : ");
          strcat(logStr, "\"");
          strcat(logStr, subCandiLog->cur->backuplog);
          strcat(logStr, "\"");
          strcat(logStr, " removed by ");
          strcat(logStr, "\"");
          strcat(logStr, path);
          strcat(logStr, "\"");
          strcat(logStr, "\n");

          if (write(fd2, logStr, strlen(logStr)) == -1)
          {
            fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
            return 1;
          }
          // 메타데이터도 지워주기
          if ((clearMetaData(subCandiLog->cur->backuplog)) < 0)
          {
            fprintf(stderr, "Error : clean meta data error\n");
          }
        }

        // 현재 백업디렉토리가 빈폴더라면 삭제하기
        char backupLog[PATHMAX];
        strcpy(backupLog, subCandiLog->cur->backuplog);
        char backupTimeTemp[PATHMAX];
        strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
        char *backuptime = strstr(backupTimeTemp, backupPATH);
        if (backuptime != NULL)
        {
          strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
        }
        // 백업시간만 잘라내기
        char *realBackupTime = strtok(backuptime, "/");

        // subCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
        // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
        char backupTmpPath[PATHMAX] = "";
        strcat(backupTmpPath, subCandiLog->cur->backuplog);
        char *subDirPath;
        char tmpBackDir[PATHMAX] = "";
        strcat(tmpBackDir, backupPATH);
        strcat(tmpBackDir, "/");
        strcat(tmpBackDir, realBackupTime);
        // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

        queue Queue2;
        initQueue(&Queue2);
        enqueue(&Queue2, tmpBackDir);

        while (1)
        {
          if (isEmpty(&Queue2))
          {
            break;
          }
          char *nodePath = dequeue(&Queue2);

          struct stat statbuf;
          if (access(nodePath, F_OK) == 0)
          {
            if (lstat(nodePath, &statbuf) < 0)
            {
              fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
              return 1;
            }
            // 디렉토리인 애들 중
            // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
            if (S_ISDIR(statbuf.st_mode))
            {

              int subCnt;
              struct dirent **subNameList;
              char prevPath[PATHMAX] = "";

              if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
              {
                fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                return 1;
              }

              if (subCnt == 2)
              {
                if (remove(nodePath) < 0)
                {
                  fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                  return 1;
                }
              }
              else if (subCnt >= 3)
              {

                for (int i = 0; i < subCnt; i++)
                {
                  if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                    continue;

                  struct stat substatbuf;
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    strcpy(prevPath, nodePath);

                    strcat(nodePath, "/");
                    strcat(nodePath, subNameList[i]->d_name);

                    // 만약 새로운경로가 디렉이면 큐에 넣고
                    if (lstat(nodePath, &substatbuf) < 0)
                    {
                      fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                      return 1;
                    }
                    if (S_ISDIR(substatbuf.st_mode))
                    {
                      enqueue(&Queue2, nodePath);
                      // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                      if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                      {
                        fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                        return 1;
                      }
                      if (subCnt == 2)
                      {
                        enqueue(&Queue2, prevPath);
                      }
                    }
                  }
                }
              }
            }
          }
        }

        // 백업폴더도 지워주기
        int cnt;
        struct dirent **nameList;

        if (access(tmpBackDir, F_OK) == 0)
        {
          if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
          {
            fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
            return 1;
          }
          if (cnt == 2)
          {
            if (remove(tmpBackDir) < 0)
            {
              fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
              return 1;
            }
          }
        }

        // 백업로그를 먼저 삭제한다 (삭제한거 다시 접근하지 않게)
        removeBackuplog(&removeCandidateLoglist, subCandiLog->cur->backuplog);
        // removecandiLogList에서도 삭제 반영
        removeLogAndUpdateList(&removeCandidateLoglist, subCandiLog->cur->log);
      }
      else if (subPosCnt > 1)
      {
        // 주의! 서브 리스트 안에서 삭제를 해야함!
        printf("remove files of %s\n", subCandiLog->cur->log);
        printf("0. exit\n");
        int iidx = 1;
        while (true)
        {

          if (subCandiLog == NULL)
          {
            break;
          }

          char backupLog[PATHMAX] = "";
          strcpy(backupLog, subCandiLog->cur->backuplog);
          char backupTimeTemp[PATHMAX] = "";
          strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
          char *backuptime = strstr(backupTimeTemp, backupPATH);
          if (backuptime != NULL)
          {
            strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
          }
          // 백업시간만 잘라내기
          char *realBackupTime = strtok(backuptime, "/");
          // 파일 크기 알아내기
          struct stat file_stat;
          if (stat(subCandiLog->cur->backuplog, &file_stat) == -1)
          {
            fprintf(stderr, "ERROR: stat error\n");
            return 1;
          }
          printf("%d. %s     %lldbytes\n", iidx, realBackupTime, (long long)file_stat.st_size);

          subCandiLog = subCandiLog->next;
          iidx++;
        }

        // 유저의 입력받기
        int userInputIdx;
        printf(">>");
        scanf("%d", &userInputIdx);
        if (userInputIdx == 0)
        {
          exit(1);
        }

        // 유저입력과 같은 번째의 노드인경우 삭제 갈기기
        iidx = 1;
        // 서브 리스트 안에서 삭제 해야하기 때문에 subCandiList를 가리키는 포인터
        subCandiLog = subCandiList;
        while (1)
        {
          if (subCandiLog == NULL)
          {
            break;
          }

          if (iidx == userInputIdx)
          {

            // 삭제하기
            if (remove(subCandiLog->cur->backuplog) < 0)
            {
              fprintf(stderr, "ERROR : remove error %s\n", subCandiLog->cur->backuplog);
              return 1;
            }
            else
            {

              char logStr[PATHMAX * 2] = "";
              char *logTime = getDate();
              printf("\"%s\" removed by \"%s\"\n", subCandiLog->cur->backuplog, path);
              strcat(logStr, logTime);
              strcat(logStr, " : ");
              strcat(logStr, "\"");
              strcat(logStr, subCandiLog->cur->backuplog);
              strcat(logStr, "\"");
              strcat(logStr, " removed by ");
              strcat(logStr, "\"");
              strcat(logStr, path);
              strcat(logStr, "\"");
              strcat(logStr, "\n");
              // 로그파일에 작성
              if (write(fd2, logStr, strlen(logStr)) == -1)
              {
                fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
                return 1;
              }
            }
            // 메타데이터도 지워주기
            if ((clearMetaData(subCandiLog->cur->backuplog)) < 0)
            {
              fprintf(stderr, "Error : clean meta data error\n");
            }

            // 현재 백업디렉토리가 빈폴더라면 삭제하기

            char backupLog[PATHMAX];
            strcpy(backupLog, subCandiLog->cur->backuplog);
            char backupTimeTemp[PATHMAX];
            strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
            char *backuptime = strstr(backupTimeTemp, backupPATH);
            if (backuptime != NULL)
            {
              strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
            }
            // 백업시간만 잘라내기
            char *realBackupTime = strtok(backuptime, "/");

            // subCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
            // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
            char backupTmpPath[PATHMAX] = "";
            strcat(backupTmpPath, subCandiLog->cur->backuplog);
            char *subDirPath;
            char tmpBackDir[PATHMAX] = "";
            strcat(tmpBackDir, backupPATH);
            strcat(tmpBackDir, "/");
            strcat(tmpBackDir, realBackupTime);
            // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

            queue Queue2;
            initQueue(&Queue2);
            enqueue(&Queue2, tmpBackDir);

            while (1)
            {
              if (isEmpty(&Queue2))
              {
                break;
              }
              char *nodePath = dequeue(&Queue2);

              struct stat statbuf;
              if (access(nodePath, F_OK) == 0)
              {
                if (lstat(nodePath, &statbuf) < 0)
                {
                  fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
                  return 1;
                }
                // 디렉토리인 애들 중
                // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
                if (S_ISDIR(statbuf.st_mode))
                {

                  int subCnt;
                  struct dirent **subNameList;
                  char prevPath[PATHMAX] = "";

                  if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                  {
                    fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                    return 1;
                  }

                  if (subCnt == 2)
                  {
                    if (remove(nodePath) < 0)
                    {
                      fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                      return 1;
                    }
                  }
                  else if (subCnt >= 3)
                  {

                    for (int i = 0; i < subCnt; i++)
                    {
                      if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                        continue;

                      struct stat substatbuf;
                      if (lstat(nodePath, &substatbuf) < 0)
                      {
                        fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                        return 1;
                      }
                      if (S_ISDIR(substatbuf.st_mode))
                      {
                        strcpy(prevPath, nodePath);

                        strcat(nodePath, "/");
                        strcat(nodePath, subNameList[i]->d_name);

                        // 만약 새로운경로가 디렉이면 큐에 넣고
                        if (lstat(nodePath, &substatbuf) < 0)
                        {
                          fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                          return 1;
                        }
                        if (S_ISDIR(substatbuf.st_mode))
                        {
                          enqueue(&Queue2, nodePath);
                          // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                          if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                          {
                            fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                            return 1;
                          }
                          if (subCnt == 2)
                          {
                            enqueue(&Queue2, prevPath);
                          }
                        }
                      }
                    }
                  }
                }
              }
            }

            // 백업폴더도 지워주기
            int cnt;
            struct dirent **nameList;

            if (access(tmpBackDir, F_OK) == 0)
            {
              if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
              {
                fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
                return 1;
              }
              if (cnt == 2)
              {
                if (remove(tmpBackDir) < 0)
                {
                  fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
                  return 1;
                }
              }
            }

            // 백업로그를 먼저 삭제한다 (삭제한거 다시 접근하지 않게)
            removeBackuplog(&removeCandidateLoglist, subCandiLog->cur->backuplog);
            // removeCandidateLoglist 에서도 삭제 반영
            removeLogAndUpdateList(&removeCandidateLoglist, subCandiLog->cur->log);
            break;
          }
          else
          {
            subCandiLog = subCandiLog->next;
            iidx++;
          }
        }
      }

      if (currRemoveLog != NULL)
      {
        currRemoveLog = currRemoveLog->next;
      }
      else
      {
        break;
      }
    }
  }

  // -d 옵션과 -a옵션
  else if (parameter->commandopt == (OPT_D | OPT_A))
  {

    char pathTemp[PATHMAX] = "";
    char tmpBackupDirPath[PATHMAX] = "";
    strcpy(pathTemp, path);

    backupList *currBackupLog = mainBackupList;
    logList *candidateLoglist = NULL;
    char *backupDate;

    // 0) 전역 백업 링크드 리스트를 돈다
    while (true)
    {
      if (currBackupLog == NULL)
      {
        break;
      }

      // 1) 유저가 입력한 디렉토리 절대경로와 로그에서 마지막 파일명만 제거한 문자열이 같으면
      char *extractLogPath = extractPath(currBackupLog->cur->log);
      // 2) 해당 날짜에 해당하는 백업 폴더로 가서

      if (!strncmp(extractLogPath, path, strlen(path)))
      {

        backupDate = splitBackupDate(currBackupLog->cur->backuplog);

        char backupDirPath[PATHMAX] = "";
        strcat(backupDirPath, backupPATH);
        strcat(backupDirPath, "/");
        strcat(backupDirPath, backupDate);

        // 매개변수로 받아온 path에서 최상위 루트디렉토리만 삭제해서 strcat하고 큐에 넣어준다.
        // 매개변수로 받아온 실행경로 만큼 잘라내기
        char *ptrDir = strstr(pathTemp, exePATH);
        if (ptrDir != NULL)
        {
          strcpy(tmpBackupDirPath, ptrDir + 1 + strlen(exePATH));
        }
        char slashRootDir[PATHMAX] = "";
        strcpy(slashRootDir, tmpBackupDirPath);
        // 백업디렉토리중 가장 상위 디렉토리 잘라내기
        char *slashPosition = strchr(slashRootDir, '/');

        if (slashPosition != NULL)
        {
          memmove(slashRootDir, slashPosition + 1, strlen(slashPosition));
        }

        if (strcmp(tmpBackupDirPath, slashRootDir))
        {
          strcat(backupDirPath, "/");
          strcat(backupDirPath, slashRootDir);
        }

        if ((cnt = scandir(backupDirPath, &namelist, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", backupDirPath);
          return 1;
        }

        for (int i = 0; i < cnt; i++)
        {

          if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
            continue;
          char subTemp[PATHMAX] = "";
          strcat(subTemp, backupDirPath);
          strcat(subTemp, "/");
          strcat(subTemp, namelist[i]->d_name);

          if (lstat(subTemp, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", subTemp);
            return 1;
          }
          // 2) Regfile인애들만 remove
          if (!S_ISDIR(statbuf.st_mode) && !strcmp(subTemp, currBackupLog->cur->backuplog))
          {

            logList *candidateNode = (logList *)malloc(sizeof(logList));
            // 노드 안의 로그 생성
            logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
            memset(candiNodeElement, 0, sizeof(candiNodeElement));
            strcpy(candiNodeElement->backuplog, currBackupLog->cur->backuplog);
            strcpy(candiNodeElement->log, currBackupLog->cur->log);
            candidateNode->cur = candiNodeElement;
            candidateNode->prev = NULL;
            candidateNode->next = NULL;

            // 로그 리스트에 로그 노드 추가
            logList *curr;
            if (candidateLoglist == NULL)
            {
              // mainlogList가 비어 있는 경우
              candidateLoglist = candidateNode;
            }
            else
            {
              curr = candidateLoglist;
              while (curr->next != NULL)
              {
                curr = curr->next;
              }
              curr->next = candidateNode;
              candidateNode->prev = curr;
            }

            break;
          }
        }
      }

      currBackupLog = currBackupLog->next;
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd2 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }

    // 3)삭제할 애들이 모여있는 리스트

    logList *currCandiLog = candidateLoglist;
    while (true)
    {
      if (currCandiLog == NULL)
      {
        break;
      }

      if (remove(currCandiLog->cur->backuplog) < 0)
      {
        fprintf(stderr, "ERROR : remove error %s\n", currCandiLog->cur->backuplog);
        return 1;
      }
      else
      {

        char logStr[PATHMAX * 2] = "";
        char *logTime = getDate();
        printf("\"%s\" removed by \"%s\"\n", currCandiLog->cur->backuplog, path);
        strcat(logStr, logTime);
        strcat(logStr, " : ");
        strcat(logStr, "\"");
        strcat(logStr, currCandiLog->cur->backuplog);
        strcat(logStr, "\"");
        strcat(logStr, " removed by ");
        strcat(logStr, "\"");
        strcat(logStr, path);
        strcat(logStr, "\"");
        strcat(logStr, "\n");

        if (write(fd2, logStr, strlen(logStr)) == -1)
        {
          fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
          return 1;
        }
        // 메타데이터도 지워주기
        if ((clearMetaData(currCandiLog->cur->backuplog)) < 0)
        {
          fprintf(stderr, "Error : clean meta data error\n");
        }
      }

      // 현재 백업디렉토리가 빈폴더라면 삭제하기
      char backupLog[PATHMAX];
      strcpy(backupLog, currCandiLog->cur->backuplog);
      char backupTimeTemp[PATHMAX];
      strcpy(backupTimeTemp, currCandiLog->cur->backuplog);
      char *backuptime = strstr(backupTimeTemp, backupPATH);
      if (backuptime != NULL)
      {
        strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
      }
      // 백업시간만 잘라내기
      char *realBackupTime = strtok(backuptime, "/");

      // currCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
      // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
      char backupTmpPath[PATHMAX] = "";
      strcat(backupTmpPath, currCandiLog->cur->backuplog);
      char *subDirPath;
      char tmpBackDir[PATHMAX] = "";
      strcat(tmpBackDir, backupPATH);
      strcat(tmpBackDir, "/");
      strcat(tmpBackDir, realBackupTime);
      // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

      queue Queue2;
      initQueue(&Queue2);
      enqueue(&Queue2, tmpBackDir);

      while (1)
      {
        if (isEmpty(&Queue2))
        {
          break;
        }
        char *nodePath = dequeue(&Queue2);

        struct stat statbuf;
        if (access(nodePath, F_OK) == 0)
        {
          if (lstat(nodePath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
            return 1;
          }
          // 디렉토리인 애들 중
          // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
          if (S_ISDIR(statbuf.st_mode))
          {

            int subCnt;
            struct dirent **subNameList;
            char prevPath[PATHMAX] = "";

            if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
              return 1;
            }

            if (subCnt == 2)
            {
              if (remove(nodePath) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                return 1;
              }
            }
            else if (subCnt >= 3)
            {

              for (int i = 0; i < subCnt; i++)
              {
                if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                  continue;

                struct stat substatbuf;
                if (lstat(nodePath, &substatbuf) < 0)
                {
                  fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                  return 1;
                }
                if (S_ISDIR(substatbuf.st_mode))
                {
                  strcpy(prevPath, nodePath);

                  strcat(nodePath, "/");
                  strcat(nodePath, subNameList[i]->d_name);

                  // 만약 새로운경로가 디렉이면 큐에 넣고
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    enqueue(&Queue2, nodePath);
                    // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                    if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                    {
                      fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                      return 1;
                    }
                    if (subCnt == 2)
                    {
                      enqueue(&Queue2, prevPath);
                    }
                  }
                }
              }
            }
          }
        }
      }

      // 백업폴더도 지워주기
      int cnt;
      struct dirent **nameList;

      if (access(tmpBackDir, F_OK) == 0)
      {
        if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
          return 1;
        }
        if (cnt == 2)
        {
          if (remove(tmpBackDir) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
            return 1;
          }
        }
      }

      // candiLogList에서도 삭제 반영
      removeBackuplog(&candidateLoglist, currCandiLog->cur->backuplog);

      if (currCandiLog == NULL)
      {
        break;
      }
      else
      {
        currCandiLog = currCandiLog->next;
      }
    }
  }
  else if (parameter->commandopt == (OPT_R | OPT_A))
  {

    char pathTemp[PATHMAX] = "";
    char tmpBackupDirPath[PATHMAX] = "";
    backupList *currBackupLog = mainBackupList;
    logList *candidateLoglist = NULL;
    // 큐에 넣을 애들 중 중복검사
    logList *enqueueLogList = NULL;
    char *backupDate;
    queue Queue;
    initQueue(&Queue);
    strcpy(pathTemp, path);

    // 1) 전역 백업 링크드 리스트를 돈다
    while (true)
    {
      if (currBackupLog == NULL)
      {
        break;
      }

      // 1) 유저가 입력한 디렉토리 절대경로와 로그에서 마지막 파일명만 제거한 문자열이 같으면
      char *extractLogPath = extractPath(currBackupLog->cur->log);
      // 2) 해당 날짜에 해당하는 백업 폴더로 가서
      if (!strncmp(extractLogPath, path, strlen(path)))
      {

        backupDate = splitBackupDate(currBackupLog->cur->backuplog);

        char backupDirPath[PATHMAX] = "";
        strcat(backupDirPath, backupPATH);
        strcat(backupDirPath, "/");
        strcat(backupDirPath, backupDate);

        // 매개변수로 받아온 path에서 최상위 루트디렉토리만 삭제해서 strcat하고 큐에 넣어준다.
        // 매개변수로 받아온 실행경로 만큼 잘라내기
        char *ptrDir = strstr(pathTemp, exePATH);
        if (ptrDir != NULL)
        {
          strcpy(tmpBackupDirPath, ptrDir + 1 + strlen(exePATH));
        }
        char slashRootDir[PATHMAX] = "";
        strcpy(slashRootDir, tmpBackupDirPath);
        // 백업디렉토리중 가장 상위 디렉토리 잘라내기
        char *slashPosition = strchr(slashRootDir, '/');

        if (slashPosition != NULL)
        {
          memmove(slashRootDir, slashPosition + 1, strlen(slashPosition));
        }

        if (strcmp(tmpBackupDirPath, slashRootDir))
        {
          strcat(backupDirPath, "/");
          strcat(backupDirPath, slashRootDir);
        }

        logList *enqueueLog = enqueueLogList;
        bool enqueueFlag = true;
        while (true)
        {
          if (enqueueLog == NULL)
          {

            break;
          }
          if (!strcmp(enqueueLog->cur->backuplog, backupDirPath))
          {
            enqueueFlag = false;
          }
          enqueueLog = enqueueLog->next;
        }

        if (enqueueFlag)
        {
          logList *candidateNode = (logList *)malloc(sizeof(logList));
          // 노드 안의 로그 생성
          logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
          memset(candiNodeElement, 0, sizeof(candiNodeElement));
          strcpy(candiNodeElement->backuplog, backupDirPath);
          strcpy(candiNodeElement->log, currBackupLog->cur->log);
          candidateNode->cur = candiNodeElement;
          candidateNode->prev = NULL;
          candidateNode->next = NULL;

          // 로그 리스트에 로그 노드 추가
          logList *curr;
          if (enqueueLogList == NULL)
          {
            // removeCandidateLoglist가 비어 있는 경우
            enqueueLogList = candidateNode;
          }
          else
          {
            curr = enqueueLogList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = candidateNode;
            candidateNode->prev = curr;
          }
        }
      }

      currBackupLog = currBackupLog->next;
    }

    // 3-1 )중복 제거해서 큐에 넣는다.
    logList *enqueueLog = enqueueLogList;
    while (true)
    {
      if (enqueueLog == NULL)
      {
        break;
      }
      enqueue(&Queue, enqueueLog->cur->backuplog);
      enqueueLog = enqueueLog->next;
    }

    // 3-2) 큐가 다 비어있을때까지 반복

    while (1)
    {
      if (isEmpty(&Queue))
      {
        break;
      }

      char *nodePath = dequeue(&Queue);

      struct stat statbuf;
      if (lstat(nodePath, &statbuf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", nodePath);
        return 1;
      }
      // 파일이라면 그대로 백업을 시켜본다
      if (!S_ISDIR(statbuf.st_mode))
      {
        RemoveFileByDir(path, nodePath, parameter);
      }
      else
      {
        // 디렉토리라면 현재 디렉토리 하위의 파일이나 폴더를 탐색 후 path부분만 지우고 다시 큐에 넣는다.
        subCnt = 0;

        if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
          return 1;
        }
        for (int i = 0; i < subCnt; i++)
        {
          if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
            continue;
          struct stat statbuf;

          // 새로운 경로를 만들기
          char newPath[PATHMAX] = "";
          char tmpDirPath[PATHMAX] = "";
          strcpy(tmpDirPath, nodePath);

          // 서브디렉토리가 있는경우 만들어주기
          if (lstat(tmpDirPath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", tmpDirPath);
            return 1;
          }

          // 새로 경로 만들기
          char checkTmpPath[PATHMAX] = "";
          strcat(checkTmpPath, nodePath);
          strcat(checkTmpPath, "/");
          strcat(checkTmpPath, subNameList[i]->d_name);

          if (lstat(checkTmpPath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", checkTmpPath);
            return 1;
          }

          // bfs유망성 판단해주기
          // 파일이면 -> 바로 파일 백업 함수로 가게
          if (S_ISREG(statbuf.st_mode))
          {
            RemoveFileByDir(path, checkTmpPath, parameter);
          }
          // 디렉토리일때만 큐에 넣어준다.
          if (S_ISDIR(statbuf.st_mode))
          {
            enqueue(&Queue, checkTmpPath);
          }
        }
      }
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd2 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }

    // 4)  RemoveFileDir에서 bfs하면서 전역 링크드 리스트에 저장시켜놓은 애들을 다시 가져와서 진짜 삭제하기!
    logList *currRemoveLog = removeCandidateLoglist;
    while (true)
    {
      if (currRemoveLog == NULL || currRemoveLog->cur->log == NULL)
      {
        break;
      }
      // removeCandidatLoglist의 최신상태를 반영할 수 있게 (지우면 세그폴트 뜸!!!!)
      currRemoveLog = removeCandidateLoglist;

      if (remove(currRemoveLog->cur->backuplog) < 0)
      {
        fprintf(stderr, "ERROR : remove error %s\n", currRemoveLog->cur->backuplog);
        return 1;
      }

      char logStr[PATHMAX * 2] = "";
      char *logTime = getDate();
      printf("\"%s\" removed by \"%s\"\n", currRemoveLog->cur->backuplog, path);
      strcat(logStr, logTime);
      strcat(logStr, " : ");
      strcat(logStr, "\"");
      strcat(logStr, currRemoveLog->cur->backuplog);
      strcat(logStr, "\"");
      strcat(logStr, " removed by ");
      strcat(logStr, "\"");
      strcat(logStr, path);
      strcat(logStr, "\"");
      strcat(logStr, "\n");

      if (write(fd2, logStr, strlen(logStr)) == -1)
      {
        fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
        return 1;
      }
      // 메타데이터도 지워주기
      if ((clearMetaData(currRemoveLog->cur->backuplog)) < 0)
      {
        fprintf(stderr, "Error : clean meta data error\n");
      }

      // 현재 백업디렉토리가 빈폴더라면 삭제하기
      char backupLog[PATHMAX];
      strcpy(backupLog, currRemoveLog->cur->backuplog);
      char backupTimeTemp[PATHMAX];
      strcpy(backupTimeTemp, currRemoveLog->cur->backuplog);
      char *backuptime = strstr(backupTimeTemp, backupPATH);
      if (backuptime != NULL)
      {
        strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
      }
      // 백업시간만 잘라내기
      char *realBackupTime = strtok(backuptime, "/");

      // currRemoveLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
      // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
      char backupTmpPath[PATHMAX] = "";
      strcat(backupTmpPath, currRemoveLog->cur->backuplog);
      char *subDirPath;
      char tmpBackDir[PATHMAX] = "";
      strcat(tmpBackDir, backupPATH);
      strcat(tmpBackDir, "/");
      strcat(tmpBackDir, realBackupTime);
      // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

      queue Queue2;
      initQueue(&Queue2);
      enqueue(&Queue2, tmpBackDir);

      while (1)
      {
        if (isEmpty(&Queue2))
        {
          break;
        }
        char *nodePath = dequeue(&Queue2);

        struct stat statbuf;
        if (access(nodePath, F_OK) == 0)
        {
          if (lstat(nodePath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
            return 1;
          }
          // 디렉토리인 애들 중
          // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
          if (S_ISDIR(statbuf.st_mode))
          {

            int subCnt;
            struct dirent **subNameList;
            char prevPath[PATHMAX] = "";

            if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
              return 1;
            }

            if (subCnt == 2)
            {
              if (remove(nodePath) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                return 1;
              }
            }
            else if (subCnt >= 3)
            {

              for (int i = 0; i < subCnt; i++)
              {
                if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                  continue;

                struct stat substatbuf;
                if (lstat(nodePath, &substatbuf) < 0)
                {
                  fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                  return 1;
                }
                if (S_ISDIR(substatbuf.st_mode))
                {
                  strcpy(prevPath, nodePath);

                  strcat(nodePath, "/");
                  strcat(nodePath, subNameList[i]->d_name);

                  // 만약 새로운경로가 디렉이면 큐에 넣고
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    enqueue(&Queue2, nodePath);
                    // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                    if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                    {
                      fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                      return 1;
                    }
                    if (subCnt == 2)
                    {
                      enqueue(&Queue2, prevPath);
                    }
                  }
                }
              }
            }
          }
        }
      }

      // 백업폴더도 지워주기
      int cnt;
      struct dirent **nameList;

      if (access(tmpBackDir, F_OK) == 0)
      {
        if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
          return 1;
        }
        if (cnt == 2)
        {
          if (remove(tmpBackDir) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
            return 1;
          }
        }
      }

      removeBackuplog(&removeCandidateLoglist, currRemoveLog->cur->backuplog);
      if (currRemoveLog == NULL)
      {
        break;
      }
      else
      {
        currRemoveLog = currRemoveLog->next;
      }
    }
  }

  return 0;
}

int RemoveCommand(command_parameter *parameter)
{
  struct stat statbuf;
  char originPath[PATHMAX];
  int fd, cnt, candidateIdx, candidateCnt, fd2;
  struct dirent **namelist, **subNameList;
  logList *candidateLoglist = NULL;
  char originHash[PATHMAX];
  char *tmpPath;

  strcpy(originPath, parameter->filename);
  // lstat을 사용해 디렉토리 속성을 가져옴
  if (lstat(originPath, &statbuf) < 0)
  {
    fprintf(stderr, "ERROR: lstat error for %s %s\n", originPath, parameter->filename);
    return 1;
  }

  // 디렉토리인데 옵션으로 -d 또는 -r로 지정되지 않았다면 에러 메시지를 출력하고 -1을 반환
  if (S_ISDIR(statbuf.st_mode) && !((parameter->commandopt & OPT_D) || (parameter->commandopt & OPT_R) || (parameter->commandopt & OPT_Y)))
  {
    fprintf(stderr, "ERROR: %s is a directory file\n", originPath);
    return -1;
  }

  // 파일인데 옵션으로 -d 또는 -r로 지정되지 않았다면 에러 메시지를 출력하고 -1을 반환
  if (S_ISREG(statbuf.st_mode) && ((parameter->commandopt & OPT_D) || (parameter->commandopt & OPT_R) || (parameter->commandopt & OPT_Y)))
  {
    fprintf(stderr, "ERROR: %s is a directory not a regular file \n", originPath);
    return -1;
  }
  if ((cnt = scandir(backupPATH, &namelist, NULL, alphasort)) == -1)
  {
    fprintf(stderr, "ERROR: scandir error for %s\n", backupPATH);
    return 1;
  }
  if (S_ISDIR(statbuf.st_mode))
  {
    RemoveDir(originPath, parameter);
  }
  else
  {
    RemoveFile(originPath, parameter);
  }

  return 0;
}

int RecoverDir(char *originPath, char *tmpPath, command_parameter *parameter)
{

  char *buf = (char *)malloc(sizeof(char) * PATHMAX);
  struct stat statbuf;
  int fd, cnt, candidateIdx, fd2, subCnt, fdBack, fdOrigin, length;
  int removeCnt = 0;
  int candidateCnt = 0;
  struct dirent **namelist, **subNameList;
  char originHash[PATHMAX];
  char *originFilePath;

  if (parameter->commandopt == OPT_D)
  {
    char pathTemp[PATHMAX] = "";
    char tmpBackupDirPath[PATHMAX] = "";
    strcpy(pathTemp, originPath);

    backupList *currBackupLog = mainBackupList;
    logList *candidateLoglist = NULL;
    char *backupDate;

    // 0) 전역 백업 링크드 리스트를 돈다
    while (true)
    {
      if (currBackupLog == NULL)
      {
        break;
      }

      // 1) 유저가 입력한 디렉토리 절대경로와 로그에서 마지막 파일명만 제거한 문자열이 같으면
      char *extractLogPath = extractPath(currBackupLog->cur->log);
      // 2) 해당 날짜에 해당하는 백업 폴더로 가서
      if (!strcmp(extractLogPath, originPath))
      {
        backupDate = splitBackupDate(currBackupLog->cur->backuplog);

        char backupDirPath[PATHMAX] = "";
        strcat(backupDirPath, backupPATH);
        strcat(backupDirPath, "/");
        strcat(backupDirPath, backupDate);
        // 매개변수로 받아온 path에서 최상위 루트디렉토리만 삭제해서 strcat하고 큐에 넣어준다.
        // 매개변수로 받아온 실행경로 만큼 잘라내기
        char *ptrDir = strstr(pathTemp, exePATH);
        if (ptrDir != NULL)
        {
          strcpy(tmpBackupDirPath, ptrDir + 1 + strlen(exePATH));
        }
        char slashRootDir[PATHMAX] = "";
        strcpy(slashRootDir, tmpBackupDirPath);
        // 백업디렉토리중 가장 상위 디렉토리 잘라내기
        char *slashPosition = strchr(slashRootDir, '/');

        if (slashPosition != NULL)
        {
          memmove(slashRootDir, slashPosition + 1, strlen(slashPosition));
        }

        if (strcmp(tmpBackupDirPath, slashRootDir))
        {
          strcat(backupDirPath, "/");
          strcat(backupDirPath, slashRootDir);
        }

        if ((cnt = scandir(backupDirPath, &namelist, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", backupDirPath);
          return 1;
        }

        for (int i = 0; i < cnt; i++)
        {

          if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
            continue;
          char subTemp[PATHMAX] = "";
          strcat(subTemp, backupDirPath);
          strcat(subTemp, "/");
          strcat(subTemp, namelist[i]->d_name);

          if (lstat(subTemp, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", subTemp);
            return 1;
          }
          // 2) Regfile인애들만 recover
          if (!S_ISDIR(statbuf.st_mode) && !strcmp(subTemp, currBackupLog->cur->backuplog))
          {

            logList *candidateNode = (logList *)malloc(sizeof(logList));
            // 노드 안의 로그 생성
            logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
            memset(candiNodeElement, 0, sizeof(candiNodeElement));
            strcpy(candiNodeElement->backuplog, currBackupLog->cur->backuplog);
            strcpy(candiNodeElement->log, currBackupLog->cur->log);
            candidateNode->cur = candiNodeElement;
            candidateNode->prev = NULL;
            candidateNode->next = NULL;

            // 로그 리스트에 로그 노드 추가
            logList *curr;
            if (candidateLoglist == NULL)
            {
              // mainlogList가 비어 있는 경우
              candidateLoglist = candidateNode;
            }
            else
            {
              curr = candidateLoglist;
              while (curr->next != NULL)
              {
                curr = curr->next;
              }
              curr->next = candidateNode;
              candidateNode->prev = curr;
            }

            break;
          }
        }
      }

      currBackupLog = currBackupLog->next;
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd2 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }

    // 3)복구할 애들이 모여있는 리스트
    // 복구 했을 때  currCandiLog에 삭제된 상황 반영이 되어있어야함!
    // mainbackuplist에도 삭제된 상황이 반영되어야 -> refresh메서드 쓰거나 삭제 메서드 쓸것
    logList *currCandiLog = candidateLoglist;
    bool exitFlag = false;
    while (true)
    {
      if (currCandiLog == NULL || currCandiLog->cur->log == NULL)
      {
        break;
      }

      // 반복문 안에서 똑같은 원본 파일이 있는지 확인하는 리스트를 만든다.
      logList *subCandiList = NULL;
      logList *subPos = candidateLoglist;
      while (true)
      {
        if (subPos == NULL)
        {
          break;
        }
        if (!strcmp(subPos->cur->log, currCandiLog->cur->log))
        {

          logList *candidateNode = (logList *)malloc(sizeof(logList));
          // 노드 안의 로그 생성
          logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
          memset(candiNodeElement, 0, sizeof(candiNodeElement));
          strcpy(candiNodeElement->backuplog, subPos->cur->backuplog);
          strcpy(candiNodeElement->log, currCandiLog->cur->log);
          candidateNode->cur = candiNodeElement;
          candidateNode->prev = NULL;
          candidateNode->next = NULL;

          // 로그 리스트에 로그 노드 추가
          logList *curr;
          if (subCandiList == NULL)
          {
            // mainlogList가 비어 있는 경우
            subCandiList = candidateNode;
          }
          else
          {
            curr = subCandiList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = candidateNode;
            candidateNode->prev = curr;
          }
        }
        subPos = subPos->next;
      }

      // 서브 리스트 속에 원소가 몇개인지 센다
      logList *subCandiLog = subCandiList;
      int subPosCnt = 0;
      while (true)
      {
        if (subCandiLog == NULL)
        {
          break;
        }

        subPosCnt++;
        subCandiLog = subCandiLog->next;
      }

      subCandiLog = subCandiList;

      if (subPosCnt == 1)
      {
        // 원본폴더로 파일 디스크립터 열기 이미 데이터가 있다면 덮어쓸 수 있게 다 밀어버리기
        if ((fdOrigin = open(currCandiLog->cur->log, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
        {
          fprintf(stderr, "ERROR : open error %s\n", currCandiLog->cur->log);
          return 1;
        }
        if ((fdBack = open(currCandiLog->cur->backuplog, O_RDONLY, 0644)) < 0)
        {
          fprintf(stderr, "ERROR : open error %s\n", currCandiLog->cur->backuplog);
          return 1;
        }
        // backup파일에서 읽은 데이터를 원본경로에 쓴다.
        while ((length = read(fdBack, buf, BUFFER_SIZE)) > 0)
        {
          write(fdOrigin, buf, length);
        }
        // 백업 파일 삭제하기
        if (remove(currCandiLog->cur->backuplog) < 0)
        {
          fprintf(stderr, "ERROR : remove error %s\n", subCandiLog->cur->backuplog);
          return 1;
        }
        else
        {

          char logStr[PATHMAX * 2] = "";
          char *logTime = getDate();
          printf("\"%s\" recovered to \"%s\"\n", subCandiLog->cur->backuplog, subCandiLog->cur->log);
          strcat(logStr, logTime);
          strcat(logStr, " : ");
          strcat(logStr, "\"");
          strcat(logStr, subCandiLog->cur->backuplog);
          strcat(logStr, "\"");
          strcat(logStr, " recovered to ");
          strcat(logStr, "\"");
          strcat(logStr, subCandiLog->cur->log);
          strcat(logStr, "\"");
          strcat(logStr, "\n");

          if (write(fd2, logStr, strlen(logStr)) == -1)
          {
            fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
            return 1;
          }
          // 메타데이터도 지워주기
          if ((clearMetaData(subCandiLog->cur->backuplog)) < 0)
          {
            fprintf(stderr, "Error : clean meta data error\n");
          }
        }

        // 현재 백업디렉토리가 빈폴더라면 삭제하기
        char backupLog[PATHMAX];
        strcpy(backupLog, subCandiLog->cur->backuplog);
        char backupTimeTemp[PATHMAX];
        strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
        char *backuptime = strstr(backupTimeTemp, backupPATH);
        if (backuptime != NULL)
        {
          strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
        }
        // 백업시간만 잘라내기
        char *realBackupTime = strtok(backuptime, "/");

        // subCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
        // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
        char backupTmpPath[PATHMAX] = "";
        strcat(backupTmpPath, subCandiLog->cur->backuplog);
        char *subDirPath;
        char tmpBackDir[PATHMAX] = "";
        strcat(tmpBackDir, backupPATH);
        strcat(tmpBackDir, "/");
        strcat(tmpBackDir, realBackupTime);
        // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

        queue Queue2;
        initQueue(&Queue2);
        enqueue(&Queue2, tmpBackDir);

        while (1)
        {
          if (isEmpty(&Queue2))
          {
            break;
          }
          char *nodePath = dequeue(&Queue2);

          struct stat statbuf;
          if (access(nodePath, F_OK) == 0)
          {
            if (lstat(nodePath, &statbuf) < 0)
            {
              fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
              return 1;
            }
            // 디렉토리인 애들 중
            // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
            if (S_ISDIR(statbuf.st_mode))
            {

              int subCnt;
              struct dirent **subNameList;
              char prevPath[PATHMAX] = "";

              if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
              {
                fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                return 1;
              }

              if (subCnt == 2)
              {
                if (remove(nodePath) < 0)
                {
                  fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                  return 1;
                }
              }
              else if (subCnt >= 3)
              {

                for (int i = 0; i < subCnt; i++)
                {
                  if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                    continue;

                  struct stat substatbuf;
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    strcpy(prevPath, nodePath);

                    strcat(nodePath, "/");
                    strcat(nodePath, subNameList[i]->d_name);

                    // 만약 새로운경로가 디렉이면 큐에 넣고
                    if (lstat(nodePath, &substatbuf) < 0)
                    {
                      fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                      return 1;
                    }
                    if (S_ISDIR(substatbuf.st_mode))
                    {
                      enqueue(&Queue2, nodePath);
                      // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                      if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                      {
                        fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                        return 1;
                      }
                      if (subCnt == 2)
                      {
                        enqueue(&Queue2, prevPath);
                      }
                    }
                  }
                }
              }
            }
          }
        }

        // 백업폴더도 지워주기
        int cnt;
        struct dirent **nameList;

        //@todo

        if (access(tmpBackDir, F_OK) == 0)
        {
          if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
          {
            fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
            return 1;
          }
          if (cnt == 2)
          {
            if (remove(tmpBackDir) < 0)
            {
              fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
              return 1;
            }
          }
        }

        // 백업로그 먼저 삭제 (이미 삭제한거 접근하지 않게)
        removeBackuplog(&candidateLoglist, subCandiLog->cur->backuplog);
        // candiLogList에서도 삭제 반영
        removeLogAndUpdateList(&candidateLoglist, subCandiLog->cur->log);
      }
      else if (subPosCnt > 1)
      {
        // 주의! 서브 리스트 안에서 삭제를 해야함!
        printf("backup files %s\n", subCandiLog->cur->log);
        printf("0. exit\n");
        int iidx = 1;
        while (true)
        {

          if (subCandiLog == NULL)
          {
            break;
          }

          char backupLog[PATHMAX] = "";
          strcpy(backupLog, subCandiLog->cur->backuplog);
          char backupTimeTemp[PATHMAX] = "";
          strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
          char *backuptime = strstr(backupTimeTemp, backupPATH);
          if (backuptime != NULL)
          {
            strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
          }
          // 백업시간만 잘라내기
          char *realBackupTime = strtok(backuptime, "/");
          // 파일 크기 알아내기
          struct stat file_stat;
          if (stat(subCandiLog->cur->backuplog, &file_stat) == -1)
          {
            fprintf(stderr, "ERROR: stat error\n");
            return 1;
          }
          printf("%d. %s     %lldbytes\n", iidx, realBackupTime, (long long)file_stat.st_size);

          subCandiLog = subCandiLog->next;
          iidx++;
        }

        // 유저의 입력받기
        int userInputIdx;
        printf(">>");
        scanf("%d", &userInputIdx);
        if (userInputIdx == 0)
        {
          exit(1);
        }

        // 유저입력과 같은 번째의 노드인경우 삭제 갈기기
        iidx = 1;
        // 서브 리스트 안에서 삭제 해야하기 때문에 subCandiList를 가리키는 포인터
        subCandiLog = subCandiList;
        while (1)
        {
          if (subCandiLog == NULL)
          {
            break;
          }
          if (iidx == userInputIdx)
          {
            // 원본폴더로 파일 디스크립터 열기 이미 데이터가 있다면 덮어쓸 수 있게 다 밀어버리기
            if ((fdOrigin = open(currCandiLog->cur->log, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
            {
              fprintf(stderr, "ERROR : open error %s\n", currCandiLog->cur->log);
              return 1;
            }
            if ((fdBack = open(currCandiLog->cur->backuplog, O_RDONLY, 0644)) < 0)
            {
              fprintf(stderr, "ERROR : open error %s\n", currCandiLog->cur->backuplog);
              return 1;
            }
            // backup파일에서 읽은 데이터를 원본경로에 쓴다.
            while ((length = read(fdBack, buf, BUFFER_SIZE)) > 0)
            {
              write(fdOrigin, buf, length);
            }
            // 삭제하기
            if (remove(subCandiLog->cur->backuplog) < 0)
            {
              fprintf(stderr, "ERROR!!!! : remove error %s\n", subCandiLog->cur->backuplog);
              return 1;
            }
            else
            {

              char logStr[PATHMAX * 2] = "";
              char *logTime = getDate();
              printf("\"%s\" recovered to \"%s\"\n", subCandiLog->cur->backuplog, subCandiLog->cur->log);
              strcat(logStr, logTime);
              strcat(logStr, " : ");
              strcat(logStr, "\"");
              strcat(logStr, subCandiLog->cur->backuplog);
              strcat(logStr, "\"");
              strcat(logStr, " recovered to ");
              strcat(logStr, "\"");
              strcat(logStr, subCandiLog->cur->log);
              strcat(logStr, "\"");
              strcat(logStr, "\n");
              // 로그파일에 작성
              if (write(fd2, logStr, strlen(logStr)) == -1)
              {
                fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
                return 1;
              }
            }
            // 메타데이터도 지워주기
            if ((clearMetaData(subCandiLog->cur->backuplog)) < 0)
            {
              fprintf(stderr, "Error : clean meta data error\n");
            }
            // 현재 백업디렉토리가 빈폴더라면 삭제하기

            char backupLog[PATHMAX];
            strcpy(backupLog, subCandiLog->cur->backuplog);
            char backupTimeTemp[PATHMAX];
            strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
            char *backuptime = strstr(backupTimeTemp, backupPATH);
            if (backuptime != NULL)
            {
              strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
            }
            // 백업시간만 잘라내기
            char *realBackupTime = strtok(backuptime, "/");

            // subCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
            // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
            char backupTmpPath[PATHMAX] = "";
            strcat(backupTmpPath, subCandiLog->cur->backuplog);
            char *subDirPath;
            char tmpBackDir[PATHMAX] = "";
            strcat(tmpBackDir, backupPATH);
            strcat(tmpBackDir, "/");
            strcat(tmpBackDir, realBackupTime);
            // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

            queue Queue2;
            initQueue(&Queue2);
            enqueue(&Queue2, tmpBackDir);

            while (1)
            {
              if (isEmpty(&Queue2))
              {
                break;
              }
              char *nodePath = dequeue(&Queue2);

              struct stat statbuf;
              if (access(nodePath, F_OK) == 0)
              {
                if (lstat(nodePath, &statbuf) < 0)
                {
                  fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
                  return 1;
                }
                // 디렉토리인 애들 중
                // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
                if (S_ISDIR(statbuf.st_mode))
                {

                  int subCnt;
                  struct dirent **subNameList;
                  char prevPath[PATHMAX] = "";

                  if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                  {
                    fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                    return 1;
                  }

                  if (subCnt == 2)
                  {
                    if (remove(nodePath) < 0)
                    {
                      fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                      return 1;
                    }
                  }
                  else if (subCnt >= 3)
                  {

                    for (int i = 0; i < subCnt; i++)
                    {
                      if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                        continue;

                      struct stat substatbuf;
                      if (lstat(nodePath, &substatbuf) < 0)
                      {
                        fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                        return 1;
                      }
                      if (S_ISDIR(substatbuf.st_mode))
                      {
                        strcpy(prevPath, nodePath);

                        strcat(nodePath, "/");
                        strcat(nodePath, subNameList[i]->d_name);

                        // 만약 새로운경로가 디렉이면 큐에 넣고
                        if (lstat(nodePath, &substatbuf) < 0)
                        {
                          fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                          return 1;
                        }
                        if (S_ISDIR(substatbuf.st_mode))
                        {
                          enqueue(&Queue2, nodePath);
                          // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                          if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                          {
                            fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                            return 1;
                          }
                          if (subCnt == 2)
                          {
                            enqueue(&Queue2, prevPath);
                          }
                        }
                      }
                    }
                  }
                }
              }
            }

            // 백업폴더도 지워주기
            int cnt;
            struct dirent **nameList;

            if (access(tmpBackDir, F_OK) == 0)
            {
              if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
              {
                fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
                return 1;
              }
              if (cnt == 2)
              {
                if (remove(tmpBackDir) < 0)
                {
                  fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
                  return 1;
                }
              }
            }
            // 백업로그 먼저 삭제 (이미 삭제한거 접근하지 않게)
            removeBackuplog(&candidateLoglist, subCandiLog->cur->backuplog);
            // candiLogList에서도 삭제 반영
            removeLogAndUpdateList(&candidateLoglist, subCandiLog->cur->log);
            break;
          }
          else
          {
            subCandiLog = subCandiLog->next;
            iidx++;
          }
        }
      }

      if (currCandiLog == NULL || currCandiLog->next == NULL)
      {
        break;
      }
      currCandiLog = currCandiLog->next;
    }
  }
  else if (parameter->commandopt == OPT_R)
  {

    char pathTemp[PATHMAX] = "";
    char tmpBackupDirPath[PATHMAX] = "";
    backupList *currBackupLog = mainBackupList;
    logList *candidateLoglist = NULL;
    // 큐에 넣을 애들 중 중복검사
    logList *enqueueLogList = NULL;
    char *backupDate;
    queue Queue;
    initQueue(&Queue);
    strcpy(pathTemp, originPath);

    // 1) 전역 백업 링크드 리스트를 돈다
    while (true)
    {
      if (currBackupLog == NULL)
      {
        break;
      }

      // 1) 유저가 입력한 디렉토리 절대경로와 로그에서 마지막 파일명만 제거한 문자열이 같으면
      char *extractLogPath = extractPath(currBackupLog->cur->log);
      // 2) 해당 날짜에 해당하는 백업 폴더로 가서
      if (!strncmp(extractLogPath, originPath, strlen(originPath)))
      {

        backupDate = splitBackupDate(currBackupLog->cur->backuplog);

        char backupDirPath[PATHMAX] = "";
        strcat(backupDirPath, backupPATH);
        strcat(backupDirPath, "/");
        strcat(backupDirPath, backupDate);

        // 매개변수로 받아온 path에서 최상위 루트디렉토리만 삭제해서 strcat하고 큐에 넣어준다.
        // 매개변수로 받아온 실행경로 만큼 잘라내기
        char *ptrDir = strstr(pathTemp, exePATH);
        if (ptrDir != NULL)
        {
          strcpy(tmpBackupDirPath, ptrDir + 1 + strlen(exePATH));
        }
        char slashRootDir[PATHMAX] = "";
        strcpy(slashRootDir, tmpBackupDirPath);
        // 백업디렉토리중 가장 상위 디렉토리 잘라내기
        char *slashPosition = strchr(slashRootDir, '/');

        if (slashPosition != NULL)
        {
          memmove(slashRootDir, slashPosition + 1, strlen(slashPosition));
        }

        if (strcmp(tmpBackupDirPath, slashRootDir))
        {
          strcat(backupDirPath, "/");
          strcat(backupDirPath, slashRootDir);
        }

        logList *enqueueLog = enqueueLogList;
        bool enqueueFlag = true;
        while (true)
        {
          if (enqueueLog == NULL)
          {

            break;
          }
          if (!strcmp(enqueueLog->cur->backuplog, backupDirPath))
          {
            enqueueFlag = false;
          }
          enqueueLog = enqueueLog->next;
        }

        if (enqueueFlag)
        {
          logList *candidateNode = (logList *)malloc(sizeof(logList));
          // 노드 안의 로그 생성
          logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
          memset(candiNodeElement, 0, sizeof(candiNodeElement));
          strcpy(candiNodeElement->backuplog, backupDirPath);
          strcpy(candiNodeElement->log, currBackupLog->cur->log);
          candidateNode->cur = candiNodeElement;
          candidateNode->prev = NULL;
          candidateNode->next = NULL;

          // 로그 리스트에 로그 노드 추가
          logList *curr;
          if (enqueueLogList == NULL)
          {
            // removeCandidateLoglist가 비어 있는 경우
            enqueueLogList = candidateNode;
          }
          else
          {
            curr = enqueueLogList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = candidateNode;
            candidateNode->prev = curr;
          }
        }
      }

      currBackupLog = currBackupLog->next;
    }

    // 3-1 )중복 제거해서 큐에 넣는다.
    logList *enqueueLog = enqueueLogList;
    while (true)
    {
      if (enqueueLog == NULL)
      {
        break;
      }
      enqueue(&Queue, enqueueLog->cur->backuplog);
      enqueueLog = enqueueLog->next;
    }

    // 3-2) 큐가 다 비어있을때까지 반복

    while (1)
    {
      if (isEmpty(&Queue))
      {
        break;
      }

      char *nodePath = dequeue(&Queue);
      struct stat statbuf;
      if (lstat(nodePath, &statbuf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", nodePath);
        return 1;
      }
      // 파일이라면 그대로 백업을 시켜본다
      if (!S_ISDIR(statbuf.st_mode))
      {
        RemoveFileByDir(originPath, nodePath, parameter);
      }
      else
      {
        // 디렉토리라면 현재 디렉토리 하위의 파일이나 폴더를 탐색 후 path부분만 지우고 다시 큐에 넣는다.
        subCnt = 0;

        if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
          return 1;
        }
        for (int i = 0; i < subCnt; i++)
        {
          if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
            continue;
          struct stat statbuf;

          // 새로운 경로를 만들기
          char newPath[PATHMAX] = "";
          char tmpDirPath[PATHMAX] = "";
          strcpy(tmpDirPath, nodePath);

          // 서브디렉토리가 있는경우 만들어주기
          if (lstat(tmpDirPath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", tmpDirPath);
            return 1;
          }

          // 새로 경로 만들기
          char checkTmpPath[PATHMAX] = "";
          strcat(checkTmpPath, nodePath);
          strcat(checkTmpPath, "/");
          strcat(checkTmpPath, subNameList[i]->d_name);

          if (lstat(checkTmpPath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", checkTmpPath);
            return 1;
          }

          // bfs유망성 판단해주기
          // 파일이면 -> 바로 파일 백업 함수로 가게
          if (S_ISREG(statbuf.st_mode))
          {
            RemoveFileByDir(originPath, checkTmpPath, parameter);
          }
          // 디렉토리일때만 큐에 넣어준다.
          if (S_ISDIR(statbuf.st_mode))
          {
            enqueue(&Queue, checkTmpPath);
          }
        }
      }
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd2 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }

    // 4)  RemoveFileDir에서 bfs하면서 전역 링크드 리스트에 저장시켜놓은 애들을 다시 가져와서 진짜 삭제하기!
    logList *currRemoveLog = removeCandidateLoglist;
    while (true)
    {
      // removeCandidatLoglist의 최신상태를 반영할 수 있게
      currRemoveLog = removeCandidateLoglist;

      if (currRemoveLog == NULL || currRemoveLog->cur->log == NULL)
      {
        break;
      }

      // 반복문 안에서 똑같은 원본 파일이 있는지 확인하는 리스트를 만든다.
      logList *subCandiList = NULL;
      logList *subPos = removeCandidateLoglist;
      while (true)
      {
        if (subPos == NULL)
        {
          break;
        }

        if (!strcmp(subPos->cur->log, currRemoveLog->cur->log))
        {

          logList *candidateNode = (logList *)malloc(sizeof(logList));
          // 노드 안의 로그 생성
          logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
          memset(candiNodeElement, 0, sizeof(candiNodeElement));
          strcpy(candiNodeElement->backuplog, subPos->cur->backuplog);
          strcpy(candiNodeElement->log, subPos->cur->log);
          candidateNode->cur = candiNodeElement;
          candidateNode->prev = NULL;
          candidateNode->next = NULL;

          // 로그 리스트에 로그 노드 추가
          logList *curr;
          if (subCandiList == NULL)
          {
            // mainlogList가 비어 있는 경우
            subCandiList = candidateNode;
          }
          else
          {
            curr = subCandiList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = candidateNode;
            candidateNode->prev = curr;
          }
        }
        subPos = subPos->next;
      }

      // 서브 리스트 속에 원소가 몇개인지 센다
      logList *subCandiLog = subCandiList;
      int subPosCnt = 0;
      while (true)
      {
        if (subCandiLog == NULL)
        {
          break;
        }

        subPosCnt++;
        subCandiLog = subCandiLog->next;
      }

      subCandiLog = subCandiList;

      if (subPosCnt == 1)
      {
        // 복원시키기
        // 원본폴더로 파일 디스크립터 열기 이미 데이터가 있다면 덮어쓸 수 있게 다 밀어버리기
        if ((fdOrigin = open(currRemoveLog->cur->backuplog, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
        {
          fprintf(stderr, "ERROR : open error %s\n", currRemoveLog->cur->backuplog);
          return 1;
        }
        if ((fdBack = open(currRemoveLog->cur->backuplog, O_RDONLY, 0644)) < 0)
        {
          fprintf(stderr, "ERROR : open error %s\n", currRemoveLog->cur->backuplog);
          return 1;
        }
        // backup파일에서 읽은 데이터를 원본경로에 쓴다.
        while ((length = read(fdBack, buf, BUFFER_SIZE)) > 0)
        {
          write(fdOrigin, buf, length);
        }
        // 백업 파일 삭제하기

        if (remove(currRemoveLog->cur->backuplog) < 0)
        {
          fprintf(stderr, "ERROR : remove error %s\n", subCandiLog->cur->backuplog);
          return 1;
        }

        char logStr[PATHMAX * 2] = "";
        char *logTime = getDate();
        printf("\"%s\" recovered to \"%s\"\n", subCandiLog->cur->backuplog, subCandiLog->cur->log);
        strcat(logStr, logTime);
        strcat(logStr, " : ");
        strcat(logStr, "\"");
        strcat(logStr, subCandiLog->cur->backuplog);
        strcat(logStr, "\"");
        strcat(logStr, " recovered to ");
        strcat(logStr, "\"");
        strcat(logStr, subCandiLog->cur->log);
        strcat(logStr, "\"");
        strcat(logStr, "\n");

        if (write(fd2, logStr, strlen(logStr)) == -1)
        {
          fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
          return 1;
        }
        // 메타데이터도 지워주기
        if ((clearMetaData(subCandiLog->cur->backuplog)) < 0)
        {
          fprintf(stderr, "Error : clean meta data error\n");
        }

        // 현재 백업디렉토리가 빈폴더라면 삭제하기
        char backupLog[PATHMAX];
        strcpy(backupLog, subCandiLog->cur->backuplog);
        char backupTimeTemp[PATHMAX];
        strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
        char *backuptime = strstr(backupTimeTemp, backupPATH);
        if (backuptime != NULL)
        {
          strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
        }
        // 백업시간만 잘라내기
        char *realBackupTime = strtok(backuptime, "/");

        // subCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
        // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
        char backupTmpPath[PATHMAX] = "";
        strcat(backupTmpPath, subCandiLog->cur->backuplog);
        char *subDirPath;
        char tmpBackDir[PATHMAX] = "";
        strcat(tmpBackDir, backupPATH);
        strcat(tmpBackDir, "/");
        strcat(tmpBackDir, realBackupTime);
        // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

        queue Queue2;
        initQueue(&Queue2);
        enqueue(&Queue2, tmpBackDir);

        while (1)
        {
          if (isEmpty(&Queue2))
          {
            break;
          }
          char *nodePath = dequeue(&Queue2);

          struct stat statbuf;
          if (access(nodePath, F_OK) == 0)
          {
            if (lstat(nodePath, &statbuf) < 0)
            {
              fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
              return 1;
            }
            // 디렉토리인 애들 중
            // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
            if (S_ISDIR(statbuf.st_mode))
            {

              int subCnt;
              struct dirent **subNameList;
              char prevPath[PATHMAX] = "";

              if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
              {
                fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                return 1;
              }

              if (subCnt == 2)
              {
                if (remove(nodePath) < 0)
                {
                  fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                  return 1;
                }
              }
              else if (subCnt >= 3)
              {

                for (int i = 0; i < subCnt; i++)
                {
                  if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                    continue;

                  struct stat substatbuf;
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    strcpy(prevPath, nodePath);

                    strcat(nodePath, "/");
                    strcat(nodePath, subNameList[i]->d_name);

                    // 만약 새로운경로가 디렉이면 큐에 넣고
                    if (lstat(nodePath, &substatbuf) < 0)
                    {
                      fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                      return 1;
                    }
                    if (S_ISDIR(substatbuf.st_mode))
                    {
                      enqueue(&Queue2, nodePath);
                      // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                      if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                      {
                        fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                        return 1;
                      }
                      if (subCnt == 2)
                      {
                        enqueue(&Queue2, prevPath);
                      }
                    }
                  }
                }
              }
            }
          }
        }

        // 백업폴더도 지워주기
        int cnt;
        struct dirent **nameList;

        if (access(tmpBackDir, F_OK) == 0)
        {
          if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
          {
            fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
            return 1;
          }
          if (cnt == 2)
          {
            if (remove(tmpBackDir) < 0)
            {
              fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
              return 1;
            }
          }
        }
        // 백업로그를 먼저 삭제한다 (삭제한거 다시 접근하지 않게)
        removeBackuplog(&removeCandidateLoglist, subCandiLog->cur->backuplog);
        // removecandiLogList에서도 삭제 반영
        removeLogAndUpdateList(&removeCandidateLoglist, subCandiLog->cur->log);
      }
      else if (subPosCnt > 1)
      {
        // 주의! 서브 리스트 안에서 삭제를 해야함!
        printf("backup files of %s\n", subCandiLog->cur->log);
        printf("0. exit\n");
        int iidx = 1;
        while (true)
        {

          if (subCandiLog == NULL)
          {
            break;
          }

          char backupLog[PATHMAX] = "";
          strcpy(backupLog, subCandiLog->cur->backuplog);
          char backupTimeTemp[PATHMAX] = "";
          strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
          char *backuptime = strstr(backupTimeTemp, backupPATH);
          if (backuptime != NULL)
          {
            strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
          }
          // 백업시간만 잘라내기
          char *realBackupTime = strtok(backuptime, "/");
          // 파일 크기 알아내기
          struct stat file_stat;
          if (stat(subCandiLog->cur->backuplog, &file_stat) == -1)
          {
            fprintf(stderr, "ERROR: stat error\n");
            return 1;
          }
          printf("%d. %s     %lldbytes\n", iidx, realBackupTime, (long long)file_stat.st_size);

          subCandiLog = subCandiLog->next;
          iidx++;
        }

        // 유저의 입력받기
        int userInputIdx;
        printf(">>");
        scanf("%d", &userInputIdx);
        if (userInputIdx == 0)
        {
          exit(1);
        }

        // 유저입력과 같은 번째의 노드인경우 삭제 갈기기
        iidx = 1;
        // 서브 리스트 안에서 삭제 해야하기 때문에 subCandiList를 가리키는 포인터
        subCandiLog = subCandiList;
        while (1)
        {
          if (subCandiLog == NULL)
          {
            break;
          }

          if (iidx == userInputIdx)
          {
            // 복원시키기
            // 원본폴더로 파일 디스크립터 열기 이미 데이터가 있다면 덮어쓸 수 있게 다 밀어버리기
            if ((fdOrigin = open(subCandiLog->cur->backuplog, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
            {
              fprintf(stderr, "ERROR : open error %s\n", subCandiLog->cur->backuplog);
              return 1;
            }
            if ((fdBack = open(subCandiLog->cur->backuplog, O_RDONLY, 0644)) < 0)
            {
              fprintf(stderr, "ERROR : open error %s\n", subCandiLog->cur->backuplog);
              return 1;
            }
            // backup파일에서 읽은 데이터를 원본경로에 쓴다.
            while ((length = read(fdBack, buf, BUFFER_SIZE)) > 0)
            {
              write(fdOrigin, buf, length);
            }
            // 백업 파일 삭제하기

            // 삭제하기
            if (remove(subCandiLog->cur->backuplog) < 0)
            {
              fprintf(stderr, "ERROR : remove error %s\n", subCandiLog->cur->backuplog);
              return 1;
            }

            char logStr[PATHMAX * 2] = "";
            char *logTime = getDate();
            printf("\"%s\" recovered to \"%s\"\n", subCandiLog->cur->backuplog, subCandiLog->cur->log);
            strcat(logStr, logTime);
            strcat(logStr, " : ");
            strcat(logStr, "\"");
            strcat(logStr, subCandiLog->cur->backuplog);
            strcat(logStr, "\"");
            strcat(logStr, " recovered to ");
            strcat(logStr, "\"");
            strcat(logStr, subCandiLog->cur->log);
            strcat(logStr, "\"");
            strcat(logStr, "\n");
            // 로그파일에 작성
            if (write(fd2, logStr, strlen(logStr)) == -1)
            {
              fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
              return 1;
            }

            // 메타데이터도 지워주기
            if ((clearMetaData(subCandiLog->cur->backuplog)) < 0)
            {
              fprintf(stderr, "Error : clean meta data error\n");
            }

            // 현재 백업디렉토리가 빈폴더라면 삭제하기
            char backupLog[PATHMAX];
            strcpy(backupLog, subCandiLog->cur->backuplog);
            char backupTimeTemp[PATHMAX];
            strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
            char *backuptime = strstr(backupTimeTemp, backupPATH);
            if (backuptime != NULL)
            {
              strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
            }
            // 백업시간만 잘라내기
            char *realBackupTime = strtok(backuptime, "/");

            // subCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
            // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
            char backupTmpPath[PATHMAX] = "";
            strcat(backupTmpPath, subCandiLog->cur->backuplog);
            char *subDirPath;
            char tmpBackDir[PATHMAX] = "";
            strcat(tmpBackDir, backupPATH);
            strcat(tmpBackDir, "/");
            strcat(tmpBackDir, realBackupTime);
            // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

            queue Queue2;
            initQueue(&Queue2);
            enqueue(&Queue2, tmpBackDir);

            while (1)
            {
              if (isEmpty(&Queue2))
              {
                break;
              }
              char *nodePath = dequeue(&Queue2);

              struct stat statbuf;
              if (access(nodePath, F_OK) == 0)
              {
                if (lstat(nodePath, &statbuf) < 0)
                {
                  fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
                  return 1;
                }
                // 디렉토리인 애들 중
                // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
                if (S_ISDIR(statbuf.st_mode))
                {

                  int subCnt;
                  struct dirent **subNameList;
                  char prevPath[PATHMAX] = "";

                  if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                  {
                    fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                    return 1;
                  }

                  if (subCnt == 2)
                  {
                    if (remove(nodePath) < 0)
                    {
                      fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                      return 1;
                    }
                  }
                  else if (subCnt >= 3)
                  {

                    for (int i = 0; i < subCnt; i++)
                    {
                      if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                        continue;

                      struct stat substatbuf;
                      if (lstat(nodePath, &substatbuf) < 0)
                      {
                        fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                        return 1;
                      }
                      if (S_ISDIR(substatbuf.st_mode))
                      {
                        strcpy(prevPath, nodePath);

                        strcat(nodePath, "/");
                        strcat(nodePath, subNameList[i]->d_name);

                        // 만약 새로운경로가 디렉이면 큐에 넣고
                        if (lstat(nodePath, &substatbuf) < 0)
                        {
                          fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                          return 1;
                        }
                        if (S_ISDIR(substatbuf.st_mode))
                        {
                          enqueue(&Queue2, nodePath);
                          // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                          if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                          {
                            fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                            return 1;
                          }
                          if (subCnt == 2)
                          {
                            enqueue(&Queue2, prevPath);
                          }
                        }
                      }
                    }
                  }
                }
              }
            }

            // 백업폴더도 지워주기
            int cnt;
            struct dirent **nameList;

            if (access(tmpBackDir, F_OK) == 0)
            {
              if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
              {
                fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
                return 1;
              }
              if (cnt == 2)
              {
                if (remove(tmpBackDir) < 0)
                {
                  fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
                  return 1;
                }
              }
            }

            // 백업로그를 먼저 삭제한다 (삭제한거 다시 접근하지 않게)
            removeBackuplog(&removeCandidateLoglist, subCandiLog->cur->backuplog);
            // removeCandidateLoglist 에서도 삭제 반영
            removeLogAndUpdateList(&removeCandidateLoglist, subCandiLog->cur->log);
            break;
          }
          else
          {
            subCandiLog = subCandiLog->next;
            iidx++;
          }
        }
      }
      if (currRemoveLog == NULL)
      {
        break;
      }
      else
      {
        currRemoveLog = currRemoveLog->next;
      }
    }
  }
  else if (parameter->commandopt == (OPT_D | OPT_L))
  {

    backupList *currBackupLog = mainBackupList;
    logList *candidateLoglist = NULL;
    char *backupDate;
    char pathTemp[PATHMAX] = "";
    char tmpBackupDirPath[PATHMAX] = "";
    strcpy(pathTemp, originPath);

    // 0) 전역 백업 링크드 리스트를 돈다
    while (true)
    {
      if (currBackupLog == NULL)
      {
        break;
      }

      // 1) 유저가 입력한 디렉토리 절대경로와 로그에서 마지막 파일명만 제거한 문자열이 같으면
      char *extractLogPath = extractPath(currBackupLog->cur->log);
      // 2) 해당 날짜에 해당하는 백업 폴더로 가서
      if (!strcmp(extractLogPath, originPath))
      {
        backupDate = splitBackupDate(currBackupLog->cur->backuplog);

        char backupDirPath[PATHMAX] = "";
        strcat(backupDirPath, backupPATH);
        strcat(backupDirPath, "/");
        strcat(backupDirPath, backupDate);

        // 매개변수로 받아온 path에서 최상위 루트디렉토리만 삭제해서 strcat하고 큐에 넣어준다.
        // 매개변수로 받아온 실행경로 만큼 잘라내기
        char *ptrDir = strstr(pathTemp, exePATH);
        if (ptrDir != NULL)
        {
          strcpy(tmpBackupDirPath, ptrDir + 1 + strlen(exePATH));
        }
        char slashRootDir[PATHMAX] = "";
        strcpy(slashRootDir, tmpBackupDirPath);
        // 백업디렉토리중 가장 상위 디렉토리 잘라내기
        char *slashPosition = strchr(slashRootDir, '/');

        if (slashPosition != NULL)
        {
          memmove(slashRootDir, slashPosition + 1, strlen(slashPosition));
        }

        if (strcmp(tmpBackupDirPath, slashRootDir))
        {
          strcat(backupDirPath, "/");
          strcat(backupDirPath, slashRootDir);
        }

        if ((cnt = scandir(backupDirPath, &namelist, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", backupDirPath);
          return 1;
        }

        for (int i = 0; i < cnt; i++)
        {

          if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
            continue;
          char subTemp[PATHMAX] = "";
          strcat(subTemp, backupDirPath);
          strcat(subTemp, "/");
          strcat(subTemp, namelist[i]->d_name);

          if (lstat(subTemp, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", subTemp);
            return 1;
          }

          // 2) Regfile인애들만 recover
          if (!S_ISDIR(statbuf.st_mode) && !strcmp(subTemp, currBackupLog->cur->backuplog))
          {

            logList *candidateNode = (logList *)malloc(sizeof(logList));
            // 노드 안의 로그 생성
            logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
            memset(candiNodeElement, 0, sizeof(candiNodeElement));
            strcpy(candiNodeElement->backuplog, currBackupLog->cur->backuplog);
            strcpy(candiNodeElement->log, currBackupLog->cur->log);
            candidateNode->cur = candiNodeElement;
            candidateNode->prev = NULL;
            candidateNode->next = NULL;

            // 로그 리스트에 로그 노드 추가
            logList *curr;
            if (candidateLoglist == NULL)
            {
              // mainlogList가 비어 있는 경우
              candidateLoglist = candidateNode;
            }
            else
            {
              curr = candidateLoglist;
              while (curr->next != NULL)
              {
                curr = curr->next;
              }
              curr->next = candidateNode;
              candidateNode->prev = curr;
            }

            break;
          }
        }
      }

      currBackupLog = currBackupLog->next;
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd2 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }

    // 3)복구할 애들이 모여있는 리스트
    // 복구 했을 때  currCandiLog에 삭제된 상황 반영이 되어있어야함!
    // mainbackuplist에도 삭제된 상황이 반영되어야 -> refresh메서드 쓰거나 삭제 메서드 쓸것
    logList *currCandiLog = candidateLoglist;

    while (true)
    {
      if (currCandiLog == NULL)
      {
        break;
      }

      // 반복문 안에서 똑같은 원본 파일이 있는지 확인하는 리스트를 만든다.
      logList *subCandiList = NULL;
      logList *subPos = candidateLoglist;
      while (true)
      {
        if (subPos == NULL)
        {
          break;
        }
        if (!strcmp(subPos->cur->log, currCandiLog->cur->log))
        {

          logList *candidateNode = (logList *)malloc(sizeof(logList));
          // 노드 안의 로그 생성
          logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
          memset(candiNodeElement, 0, sizeof(candiNodeElement));
          strcpy(candiNodeElement->backuplog, subPos->cur->backuplog);
          strcpy(candiNodeElement->log, currCandiLog->cur->log);
          candidateNode->cur = candiNodeElement;
          candidateNode->prev = NULL;
          candidateNode->next = NULL;

          // 로그 리스트에 로그 노드 추가
          logList *curr;
          if (subCandiList == NULL)
          {
            // mainlogList가 비어 있는 경우
            subCandiList = candidateNode;
          }
          else
          {
            curr = subCandiList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = candidateNode;
            candidateNode->prev = curr;
          }
        }
        subPos = subPos->next;
      }

      // 서브 리스트 끝까지 간다.
      logList *subCandiLog = subCandiList;

      while (true)
      {
        if (subCandiLog->next == NULL)
        {
          break;
        }

        subCandiLog = subCandiLog->next;
      }

      // 원본폴더로 파일 디스크립터 열기 이미 데이터가 있다면 덮어쓸 수 있게 다 밀어버리기
      if ((fdOrigin = open(subCandiLog->cur->log, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
      {
        fprintf(stderr, "ERROR : open error %s\n", subCandiLog->cur->log);
        return 1;
      }
      if ((fdBack = open(subCandiLog->cur->backuplog, O_RDONLY, 0644)) < 0)
      {
        fprintf(stderr, "ERROR : open error %s\n", subCandiLog->cur->backuplog);
        return 1;
      }
      // backup파일에서 읽은 데이터를 원본경로에 쓴다.
      while ((length = read(fdBack, buf, BUFFER_SIZE)) > 0)
      {
        write(fdOrigin, buf, length);
      }
      // 백업 파일 삭제하기
      if (remove(subCandiLog->cur->backuplog) < 0)
      {
        fprintf(stderr, "ERROR : remove error %s\n", subCandiLog->cur->backuplog);
        return 1;
      }
      else
      {

        char logStr[PATHMAX * 2] = "";
        char *logTime = getDate();
        printf("\"%s\" recovered to \"%s\"\n", subCandiLog->cur->backuplog, subCandiLog->cur->log);
        strcat(logStr, logTime);
        strcat(logStr, " : ");
        strcat(logStr, "\"");
        strcat(logStr, subCandiLog->cur->backuplog);
        strcat(logStr, "\"");
        strcat(logStr, " recovered to ");
        strcat(logStr, "\"");
        strcat(logStr, subCandiLog->cur->log);
        strcat(logStr, "\"");
        strcat(logStr, "\n");

        if (write(fd2, logStr, strlen(logStr)) == -1)
        {
          fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
          return 1;
        }
        // 메타데이터도 지워주기
        if ((clearMetaData(subCandiLog->cur->backuplog)) < 0)
        {
          fprintf(stderr, "Error : clean meta data error\n");
        }
      }

      // 현재 백업디렉토리가 빈폴더라면 삭제하기
      char backupLog[PATHMAX];
      strcpy(backupLog, subCandiLog->cur->backuplog);
      char backupTimeTemp[PATHMAX];
      strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
      char *backuptime = strstr(backupTimeTemp, backupPATH);
      if (backuptime != NULL)
      {
        strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
      }
      // 백업시간만 잘라내기
      char *realBackupTime = strtok(backuptime, "/");

      // subCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
      // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
      char backupTmpPath[PATHMAX] = "";
      strcat(backupTmpPath, subCandiLog->cur->backuplog);
      char *subDirPath;
      char tmpBackDir[PATHMAX] = "";
      strcat(tmpBackDir, backupPATH);
      strcat(tmpBackDir, "/");
      strcat(tmpBackDir, realBackupTime);
      // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

      queue Queue2;
      initQueue(&Queue2);
      enqueue(&Queue2, tmpBackDir);

      while (1)
      {
        if (isEmpty(&Queue2))
        {
          break;
        }
        char *nodePath = dequeue(&Queue2);

        struct stat statbuf;
        if (access(nodePath, F_OK) == 0)
        {
          if (lstat(nodePath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
            return 1;
          }
          // 디렉토리인 애들 중
          // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
          if (S_ISDIR(statbuf.st_mode))
          {

            int subCnt;
            struct dirent **subNameList;
            char prevPath[PATHMAX] = "";

            if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
              return 1;
            }

            if (subCnt == 2)
            {
              if (remove(nodePath) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                return 1;
              }
            }
            else if (subCnt >= 3)
            {

              for (int i = 0; i < subCnt; i++)
              {
                if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                  continue;

                struct stat substatbuf;
                if (lstat(nodePath, &substatbuf) < 0)
                {
                  fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                  return 1;
                }
                if (S_ISDIR(substatbuf.st_mode))
                {
                  strcpy(prevPath, nodePath);

                  strcat(nodePath, "/");
                  strcat(nodePath, subNameList[i]->d_name);

                  // 만약 새로운경로가 디렉이면 큐에 넣고
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    enqueue(&Queue2, nodePath);
                    // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                    if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                    {
                      fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                      return 1;
                    }
                    if (subCnt == 2)
                    {
                      enqueue(&Queue2, prevPath);
                    }
                  }
                }
              }
            }
          }
        }
      }

      // 백업폴더도 지워주기
      int cnt;
      struct dirent **nameList;

      if (access(tmpBackDir, F_OK) == 0)
      {
        if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
          return 1;
        }
        if (cnt == 2)
        {
          if (remove(tmpBackDir) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
            return 1;
          }
        }
      }

      // 백업로그 먼저 삭제 (이미 삭제한거 접근하지 않게)
      removeBackuplog(&candidateLoglist, subCandiLog->cur->backuplog);
      // candiLogList에서도 삭제 반영
      removeLogAndUpdateList(&candidateLoglist, subCandiLog->cur->log);

      if (currCandiLog == NULL)
      {
        break;
      }
      else
      {
        currCandiLog = currCandiLog->next;
      }
    }
  }
  else if (parameter->commandopt == (OPT_R | OPT_L))
  {
    char pathTemp[PATHMAX] = "";
    char tmpBackupDirPath[PATHMAX] = "";
    backupList *currBackupLog = mainBackupList;
    logList *candidateLoglist = NULL;
    // 큐에 넣을 애들 중 중복검사
    logList *enqueueLogList = NULL;
    char *backupDate;
    queue Queue;
    initQueue(&Queue);
    strcpy(pathTemp, originPath);

    // 1) 전역 백업 링크드 리스트를 돈다
    while (true)
    {
      if (currBackupLog == NULL)
      {
        break;
      }

      // 1) 유저가 입력한 디렉토리 절대경로와 로그에서 마지막 파일명만 제거한 문자열이 같으면
      char *extractLogPath = extractPath(currBackupLog->cur->log);
      // 2) 해당 날짜에 해당하는 백업 폴더로 가서
      if (!strncmp(extractLogPath, originPath, strlen(originPath)))
      {

        backupDate = splitBackupDate(currBackupLog->cur->backuplog);

        char backupDirPath[PATHMAX] = "";
        strcat(backupDirPath, backupPATH);
        strcat(backupDirPath, "/");
        strcat(backupDirPath, backupDate);

        // 매개변수로 받아온 path에서 최상위 루트디렉토리만 삭제해서 strcat하고 큐에 넣어준다.
        // 매개변수로 받아온 실행경로 만큼 잘라내기
        char *ptrDir = strstr(pathTemp, exePATH);
        if (ptrDir != NULL)
        {
          strcpy(tmpBackupDirPath, ptrDir + 1 + strlen(exePATH));
        }
        char slashRootDir[PATHMAX] = "";
        strcpy(slashRootDir, tmpBackupDirPath);
        // 백업디렉토리중 가장 상위 디렉토리 잘라내기
        char *slashPosition = strchr(slashRootDir, '/');

        if (slashPosition != NULL)
        {
          memmove(slashRootDir, slashPosition + 1, strlen(slashPosition));
        }

        if (strcmp(tmpBackupDirPath, slashRootDir))
        {
          strcat(backupDirPath, "/");
          strcat(backupDirPath, slashRootDir);
        }

        logList *enqueueLog = enqueueLogList;
        bool enqueueFlag = true;
        while (true)
        {
          if (enqueueLog == NULL)
          {

            break;
          }
          if (!strcmp(enqueueLog->cur->backuplog, backupDirPath))
          {
            enqueueFlag = false;
          }
          enqueueLog = enqueueLog->next;
        }

        if (enqueueFlag)
        {
          logList *candidateNode = (logList *)malloc(sizeof(logList));
          // 노드 안의 로그 생성
          logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
          memset(candiNodeElement, 0, sizeof(candiNodeElement));
          strcpy(candiNodeElement->backuplog, backupDirPath);
          strcpy(candiNodeElement->log, currBackupLog->cur->log);
          candidateNode->cur = candiNodeElement;
          candidateNode->prev = NULL;
          candidateNode->next = NULL;

          // 로그 리스트에 로그 노드 추가
          logList *curr;
          if (enqueueLogList == NULL)
          {
            // removeCandidateLoglist가 비어 있는 경우
            enqueueLogList = candidateNode;
          }
          else
          {
            curr = enqueueLogList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = candidateNode;
            candidateNode->prev = curr;
          }
        }
      }

      currBackupLog = currBackupLog->next;
    }

    // 3-1 )중복 제거해서 큐에 넣는다.
    logList *enqueueLog = enqueueLogList;
    while (true)
    {
      if (enqueueLog == NULL)
      {
        break;
      }
      enqueue(&Queue, enqueueLog->cur->backuplog);
      enqueueLog = enqueueLog->next;
    }

    // 3-2) 큐가 다 비어있을때까지 반복

    while (1)
    {
      if (isEmpty(&Queue))
      {
        break;
      }

      char *nodePath = dequeue(&Queue);
      struct stat statbuf;
      if (lstat(nodePath, &statbuf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", nodePath);
        return 1;
      }
      // 파일이라면 그대로 백업을 시켜본다
      if (!S_ISDIR(statbuf.st_mode))
      {
        RemoveFileByDir(originPath, nodePath, parameter);
      }
      else
      {
        // 디렉토리라면 현재 디렉토리 하위의 파일이나 폴더를 탐색 후 path부분만 지우고 다시 큐에 넣는다.
        subCnt = 0;

        if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
          return 1;
        }
        for (int i = 0; i < subCnt; i++)
        {
          if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
            continue;
          struct stat statbuf;

          // 새로운 경로를 만들기
          char newPath[PATHMAX] = "";
          char tmpDirPath[PATHMAX] = "";
          strcpy(tmpDirPath, nodePath);

          // 서브디렉토리가 있는경우 만들어주기
          if (lstat(tmpDirPath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", tmpDirPath);
            return 1;
          }

          // 새로 경로 만들기
          char checkTmpPath[PATHMAX] = "";
          strcat(checkTmpPath, nodePath);
          strcat(checkTmpPath, "/");
          strcat(checkTmpPath, subNameList[i]->d_name);

          if (lstat(checkTmpPath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", checkTmpPath);
            return 1;
          }

          // bfs유망성 판단해주기
          // 파일이면 -> 바로 파일 백업 함수로 가게
          if (S_ISREG(statbuf.st_mode))
          {
            RemoveFileByDir(originPath, checkTmpPath, parameter);
          }
          // 디렉토리일때만 큐에 넣어준다.
          if (S_ISDIR(statbuf.st_mode))
          {
            enqueue(&Queue, checkTmpPath);
          }
        }
      }
    }

    // ssubackLog에 작성할 파일디스크립터 생성
    if ((fd2 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
    {
      fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
      return 1;
    }

    // 4)  RemoveFileDir에서 bfs하면서 전역 링크드 리스트에 저장시켜놓은 애들을 다시 가져와서 진짜 삭제하기!
    logList *currRemoveLog = removeCandidateLoglist;
    while (true)
    {
      // removeCandidatLoglist의 최신상태를 반영할 수 있게
      currRemoveLog = removeCandidateLoglist;

      if (currRemoveLog == NULL || currRemoveLog->cur->log == NULL)
      {
        break;
      }

      // 반복문 안에서 똑같은 원본 파일이 있는지 확인하는 리스트를 만든다.
      logList *subCandiList = NULL;
      logList *subPos = removeCandidateLoglist;
      while (true)
      {
        if (subPos == NULL)
        {
          break;
        }

        if (!strcmp(subPos->cur->log, currRemoveLog->cur->log))
        {

          logList *candidateNode = (logList *)malloc(sizeof(logList));
          // 노드 안의 로그 생성
          logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
          memset(candiNodeElement, 0, sizeof(candiNodeElement));
          strcpy(candiNodeElement->backuplog, subPos->cur->backuplog);
          strcpy(candiNodeElement->log, subPos->cur->log);
          candidateNode->cur = candiNodeElement;
          candidateNode->prev = NULL;
          candidateNode->next = NULL;

          // 로그 리스트에 로그 노드 추가
          logList *curr;
          if (subCandiList == NULL)
          {
            // mainlogList가 비어 있는 경우
            subCandiList = candidateNode;
          }
          else
          {
            curr = subCandiList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = candidateNode;
            candidateNode->prev = curr;
          }
        }
        subPos = subPos->next;
      }

      // 서브 리스트  끝까지 간다.
      logList *subCandiLog = subCandiList;

      while (true)
      {
        if (subCandiLog->next == NULL)
        {
          break;
        }
        subCandiLog = subCandiLog->next;
      }

      // 복원시키기
      // 원본폴더로 파일 디스크립터 열기 이미 데이터가 있다면 덮어쓸 수 있게 다 밀어버리기
      if ((fdOrigin = open(subCandiLog->cur->backuplog, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
      {
        fprintf(stderr, "ERROR : open error %s\n", subCandiLog->cur->backuplog);
        return 1;
      }
      if ((fdBack = open(subCandiLog->cur->backuplog, O_RDONLY, 0644)) < 0)
      {
        fprintf(stderr, "ERROR : open error %s\n", subCandiLog->cur->backuplog);
        return 1;
      }
      // backup파일에서 읽은 데이터를 원본경로에 쓴다.
      while ((length = read(fdBack, buf, BUFFER_SIZE)) > 0)
      {
        write(fdOrigin, buf, length);
      }
      // 백업 파일 삭제하기

      if (remove(subCandiLog->cur->backuplog) < 0)
      {
        fprintf(stderr, "ERROR : remove error %s\n", subCandiLog->cur->backuplog);
        return 1;
      }

      char logStr[PATHMAX * 2] = "";
      char *logTime = getDate();
      printf("\"%s\" recovered to \"%s\"\n", subCandiLog->cur->backuplog, subCandiLog->cur->log);
      strcat(logStr, logTime);
      strcat(logStr, " : ");
      strcat(logStr, "\"");
      strcat(logStr, subCandiLog->cur->backuplog);
      strcat(logStr, "\"");
      strcat(logStr, " recovered to ");
      strcat(logStr, "\"");
      strcat(logStr, subCandiLog->cur->log);
      strcat(logStr, "\"");
      strcat(logStr, "\n");

      if (write(fd2, logStr, strlen(logStr)) == -1)
      {
        fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
        return 1;
      }
      // 메타데이터도 지워주기
      if ((clearMetaData(subCandiLog->cur->backuplog)) < 0)
      {
        fprintf(stderr, "Error : clean meta data error\n");
      }

      // 현재 백업디렉토리가 빈폴더라면 삭제하기
      char backupLog[PATHMAX];
      strcpy(backupLog, subCandiLog->cur->backuplog);
      char backupTimeTemp[PATHMAX];
      strcpy(backupTimeTemp, subCandiLog->cur->backuplog);
      char *backuptime = strstr(backupTimeTemp, backupPATH);
      if (backuptime != NULL)
      {
        strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
      }
      // 백업시간만 잘라내기
      char *realBackupTime = strtok(backuptime, "/");

      // subCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
      // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
      char backupTmpPath[PATHMAX] = "";
      strcat(backupTmpPath, subCandiLog->cur->backuplog);
      char *subDirPath;
      char tmpBackDir[PATHMAX] = "";
      strcat(tmpBackDir, backupPATH);
      strcat(tmpBackDir, "/");
      strcat(tmpBackDir, realBackupTime);
      // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

      queue Queue2;
      initQueue(&Queue2);
      enqueue(&Queue2, tmpBackDir);

      while (1)
      {
        if (isEmpty(&Queue2))
        {
          break;
        }
        char *nodePath = dequeue(&Queue2);

        struct stat statbuf;
        if (access(nodePath, F_OK) == 0)
        {
          if (lstat(nodePath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
            return 1;
          }
          // 디렉토리인 애들 중
          // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
          if (S_ISDIR(statbuf.st_mode))
          {

            int subCnt;
            struct dirent **subNameList;
            char prevPath[PATHMAX] = "";

            if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
              return 1;
            }

            if (subCnt == 2)
            {
              if (remove(nodePath) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                return 1;
              }
            }
            else if (subCnt >= 3)
            {

              for (int i = 0; i < subCnt; i++)
              {
                if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                  continue;

                struct stat substatbuf;
                if (lstat(nodePath, &substatbuf) < 0)
                {
                  fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                  return 1;
                }
                if (S_ISDIR(substatbuf.st_mode))
                {
                  strcpy(prevPath, nodePath);

                  strcat(nodePath, "/");
                  strcat(nodePath, subNameList[i]->d_name);

                  // 만약 새로운경로가 디렉이면 큐에 넣고
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    enqueue(&Queue2, nodePath);
                    // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                    if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                    {
                      fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                      return 1;
                    }
                    if (subCnt == 2)
                    {
                      enqueue(&Queue2, prevPath);
                    }
                  }
                }
              }
            }
          }
        }
      }

      // 백업폴더도 지워주기
      int cnt;
      struct dirent **nameList;

      if (access(tmpBackDir, F_OK) == 0)
      {
        if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
          return 1;
        }
        if (cnt == 2)
        {
          if (remove(tmpBackDir) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
            return 1;
          }
        }
      }
      // 백업로그를 먼저 삭제한다 (삭제한거 다시 접근하지 않게)
      removeBackuplog(&removeCandidateLoglist, subCandiLog->cur->backuplog);
      // removecandiLogList에서도 삭제 반영
      removeLogAndUpdateList(&removeCandidateLoglist, subCandiLog->cur->log);
      if (currRemoveLog == NULL)
      {
        break;
      }
      else
      {
        currRemoveLog = currRemoveLog->next;
      }
    }
  }
  return 0;
}

int RecoverFile(char *originPath, char *tmpPath, command_parameter *parameter)
{

  int fd1, fd2, fd3, len, candidateCnt;
  char *buf = (char *)malloc(sizeof(char) * PATHMAX);
  logList *candidateLoglist = NULL;
  backupList *currBackupLog = mainBackupList;

  if ((fd1 = open(ssubakLogPATH, O_WRONLY | O_APPEND, 777)) < 0)
  {
    fprintf(stderr, "ERROR: open error for %s\n", ssubakLogPATH);
    return 1;
  }

  if ((fd2 = open(originPath, O_WRONLY | O_CREAT | O_TRUNC, 777)) < 0)
  {
    fprintf(stderr, "ERROR: open error for %s\n", originPath);
    return 1;
  }

  if (parameter->commandopt == OPT_L)
  {
    //  1) 전역 백업 링크드리스트 돌면서 originPath에 해당하는 백업파일들을 candidateList에 담는다.
    while (true)
    {

      if (currBackupLog == NULL)
      {
        break;
      }
      // originPath와 동일한 경로는 candidateList애 담아주기
      if (!strcmp(originPath, currBackupLog->cur->log))
      {
        logList *candidateNode = (logList *)malloc(sizeof(logList));
        // 노드 안의 로그 생성
        logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
        memset(candiNodeElement, 0, sizeof(candiNodeElement));
        strcpy(candiNodeElement->backuplog, currBackupLog->cur->backuplog);
        strcpy(candiNodeElement->log, currBackupLog->cur->log);
        candidateNode->cur = candiNodeElement;
        candidateNode->prev = NULL;
        candidateNode->next = NULL;

        // 로그 리스트에 로그 노드 추가
        logList *curr;
        if (candidateLoglist == NULL)
        {
          // mainlogList가 비어 있는 경우
          candidateLoglist = candidateNode;
          candidateCnt++;
        }
        else
        {
          curr = candidateLoglist;
          while (curr->next != NULL)
          {
            curr = curr->next;
            candidateCnt++;
          }
          curr->next = candidateNode;
          candidateNode->prev = curr;
          candidateCnt++;
        }
      }

      currBackupLog = currBackupLog->next;
    }

    logList *currCandiLog = candidateLoglist;

    // 2) candidateList 크기가 한개일경우
    if (candidateCnt == 1)
    {
      struct stat statbuf;
      // 주어진 파일의 속성을 확인
      if (lstat(currCandiLog->cur->backuplog, &statbuf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", currCandiLog->cur->backuplog);
        return 1;
      }
      // 2-1) originPath에 백업파일을 그대로 복사한다. (originPath에 파일이 있다면 덮어쓰기)
      if ((fd3 = open(currCandiLog->cur->backuplog, O_WRONLY | O_CREAT | O_TRUNC, 777)) < 0)
      {
        fprintf(stderr, "ERROR: open error for %s\n", originPath);
        return 1;
      }
      while ((len = read(fd3, buf, statbuf.st_size)) > 0)
      {
        write(fd2, buf, len);
      }
      // 2-2) 백업파일을 삭제한다.
      if (remove(currCandiLog->cur->backuplog) < 0)
      {
        fprintf(stderr, "ERROR : remove error %s\n", currCandiLog->cur->backuplog);
        return 1;
      }
      // 2-3) 복구 성공시 메세지 출력
      char logStr[PATHMAX * 2] = "";
      char *backupTime = splitBackupDate(currCandiLog->cur->backuplog);

      printf("\"%s\"recovered to \"%s\"\n", currCandiLog->cur->backuplog, originPath);
      strcat(logStr, backupTime);
      strcat(logStr, " : ");
      strcat(logStr, "\"");
      strcat(logStr, currCandiLog->cur->backuplog);
      strcat(logStr, "\"");
      strcat(logStr, " recovered to ");
      strcat(logStr, "\"");
      strcat(logStr, originPath);
      strcat(logStr, "\"");
      strcat(logStr, "\n");
      // 메타데이터도 지워주기
      if ((clearMetaData(currCandiLog->cur->backuplog)) < 0)
      {
        fprintf(stderr, "Error : clean meta data error\n");
      }

      // 2-4) 복구 성공시 로그 append
      if (write(fd1, logStr, strlen(logStr)) == -1)
      {
        fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
        return 1;
      }

      // 2-5) 현재 백업디렉토리가 빈폴더라면 삭제하기

      char backupLog[PATHMAX];
      strcpy(backupLog, currCandiLog->cur->backuplog);
      char backupTimeTemp[PATHMAX];
      strcpy(backupTimeTemp, currCandiLog->cur->backuplog);
      char *backuptime = strstr(backupTimeTemp, backupPATH);
      if (backuptime != NULL)
      {
        strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
      }
      // 백업시간만 잘라내기
      char *realBackupTime = strtok(backuptime, "/");

      // currCandiLogg->cur->backuplog에서 backupPATH/realBackupTime 제거하고
      // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
      char backupTmpPath[PATHMAX] = "";
      strcat(backupTmpPath, currCandiLog->cur->backuplog);
      char *subDirPath;
      char tmpBackDir[PATHMAX] = "";
      strcat(tmpBackDir, backupPATH);
      strcat(tmpBackDir, "/");
      strcat(tmpBackDir, realBackupTime);
      // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

      queue Queue2;
      initQueue(&Queue2);
      enqueue(&Queue2, tmpBackDir);

      while (1)
      {
        if (isEmpty(&Queue2))
        {
          break;
        }
        char *nodePath = dequeue(&Queue2);

        struct stat statbuf;
        if (access(nodePath, F_OK) == 0)
        {
          if (lstat(nodePath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
            return 1;
          }
          // 디렉토리인 애들 중
          // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
          if (S_ISDIR(statbuf.st_mode))
          {

            int subCnt;
            struct dirent **subNameList;
            char prevPath[PATHMAX] = "";

            if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
              return 1;
            }

            if (subCnt == 2)
            {
              if (remove(nodePath) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                return 1;
              }
            }
            else if (subCnt >= 3)
            {

              for (int i = 0; i < subCnt; i++)
              {
                if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                  continue;

                struct stat substatbuf;
                if (lstat(nodePath, &substatbuf) < 0)
                {
                  fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                  return 1;
                }
                if (S_ISDIR(substatbuf.st_mode))
                {
                  strcpy(prevPath, nodePath);

                  strcat(nodePath, "/");
                  strcat(nodePath, subNameList[i]->d_name);

                  // 만약 새로운경로가 디렉이면 큐에 넣고
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    enqueue(&Queue2, nodePath);
                    // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                    if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                    {
                      fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                      return 1;
                    }
                    if (subCnt == 2)
                    {
                      enqueue(&Queue2, prevPath);
                    }
                  }
                }
              }
            }
          }
        }
      }

      // 백업폴더도 지워주기
      int cnt;
      struct dirent **nameList;

      if (access(tmpBackDir, F_OK) == 0)
      {
        if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
          return 1;
        }
        if (cnt == 2)
        {
          if (remove(tmpBackDir) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
            return 1;
          }
        }
      }
    }
    else if (candidateCnt > 1)
    {
      // 3) candidateList 크기가 두개 이상일 경우

      // 3-1) 링크드리스트 끝으로 가기
      while (true)
      {
        if (currCandiLog->next == NULL)
        {
          break;
        }

        currCandiLog = currCandiLog->next;
      }
      struct stat statbuf;
      // 주어진 파일의 속성을 확인
      if (lstat(currCandiLog->cur->backuplog, &statbuf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", currCandiLog->cur->backuplog);
        return 1;
      }
      // 2-1) originPath에 백업파일을 그대로 복사한다. (originPath에 파일이 있다면 덮어쓰기)
      if ((fd3 = open(currCandiLog->cur->backuplog, O_WRONLY | O_CREAT | O_TRUNC, 777)) < 0)
      {
        fprintf(stderr, "ERROR: open error for %s\n", originPath);
        return 1;
      }
      while ((len = read(fd3, buf, statbuf.st_size)) > 0)
      {
        write(fd2, buf, len);
      }
      // 2-2) 백업파일을 삭제한다.
      if (remove(currCandiLog->cur->backuplog) < 0)
      {
        fprintf(stderr, "ERROR : remove error %s\n", currCandiLog->cur->backuplog);
        return 1;
      }
      // 2-3) 복구 성공시 메세지 출력
      char logStr[PATHMAX * 2] = "";
      char *backupTime = splitBackupDate(currCandiLog->cur->backuplog);

      printf("\"%s\"recovered to \"%s\"\n", currCandiLog->cur->backuplog, originPath);
      strcat(logStr, backupTime);
      strcat(logStr, " : ");
      strcat(logStr, "\"");
      strcat(logStr, currCandiLog->cur->backuplog);
      strcat(logStr, "\"");
      strcat(logStr, " recovered to ");
      strcat(logStr, "\"");
      strcat(logStr, originPath);
      strcat(logStr, "\"");
      strcat(logStr, "\n");

      // 메타데이터도 지워주기
      if ((clearMetaData(currCandiLog->cur->backuplog)) < 0)
      {
        fprintf(stderr, "Error : clean meta data error\n");
      }

      // 2-4) 복구 성공시 로그 append
      if (write(fd1, logStr, strlen(logStr)) == -1)
      {
        fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
        return 1;
      }
      // 2-5) 현재 백업디렉토리가 빈폴더라면 삭제하기
      char backupLog[PATHMAX];
      strcpy(backupLog, currCandiLog->cur->backuplog);
      char backupTimeTemp[PATHMAX];
      strcpy(backupTimeTemp, currCandiLog->cur->backuplog);
      char *backuptime = strstr(backupTimeTemp, backupPATH);
      if (backuptime != NULL)
      {
        strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
      }
      // 백업시간만 잘라내기
      char *realBackupTime = strtok(backuptime, "/");

      // currCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
      // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
      char backupTmpPath[PATHMAX] = "";
      strcat(backupTmpPath, currCandiLog->cur->backuplog);
      char *subDirPath;
      char tmpBackDir[PATHMAX] = "";
      strcat(tmpBackDir, backupPATH);
      strcat(tmpBackDir, "/");
      strcat(tmpBackDir, realBackupTime);
      // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

      queue Queue2;
      initQueue(&Queue2);
      enqueue(&Queue2, tmpBackDir);

      while (1)
      {
        if (isEmpty(&Queue2))
        {
          break;
        }
        char *nodePath = dequeue(&Queue2);

        struct stat statbuf;
        if (access(nodePath, F_OK) == 0)
        {
          if (lstat(nodePath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
            return 1;
          }
          // 디렉토리인 애들 중
          // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
          if (S_ISDIR(statbuf.st_mode))
          {

            int subCnt;
            struct dirent **subNameList;
            char prevPath[PATHMAX] = "";

            if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
              return 1;
            }

            if (subCnt == 2)
            {
              if (remove(nodePath) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                return 1;
              }
            }
            else if (subCnt >= 3)
            {

              for (int i = 0; i < subCnt; i++)
              {
                if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                  continue;

                struct stat substatbuf;
                if (lstat(nodePath, &substatbuf) < 0)
                {
                  fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                  return 1;
                }
                if (S_ISDIR(substatbuf.st_mode))
                {
                  strcpy(prevPath, nodePath);

                  strcat(nodePath, "/");
                  strcat(nodePath, subNameList[i]->d_name);

                  // 만약 새로운경로가 디렉이면 큐에 넣고
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    enqueue(&Queue2, nodePath);
                    // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                    if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                    {
                      fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                      return 1;
                    }
                    if (subCnt == 2)
                    {
                      enqueue(&Queue2, prevPath);
                    }
                  }
                }
              }
            }
          }
        }
      }

      // 백업폴더도 지워주기
      int cnt;
      struct dirent **nameList;

      if (access(tmpBackDir, F_OK) == 0)
      {
        if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
          return 1;
        }
        if (cnt == 2)
        {
          if (remove(tmpBackDir) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
            return 1;
          }
        }
      }
    }
  }
  else
  {
    //  1) 전역 백업 링크드리스트 돌면서 originPath에 해당하는 백업파일들을 candidateList에 담는다.
    while (true)
    {

      if (currBackupLog == NULL)
      {
        break;
      }
      // originPath와 동일한 경로는 candidateList애 담아주기
      if (!strcmp(originPath, currBackupLog->cur->log))
      {
        logList *candidateNode = (logList *)malloc(sizeof(logList));
        // 노드 안의 로그 생성
        logElement *candiNodeElement = (logElement *)malloc(sizeof(logElement));
        memset(candiNodeElement, 0, sizeof(candiNodeElement));
        strcpy(candiNodeElement->backuplog, currBackupLog->cur->backuplog);
        strcpy(candiNodeElement->log, currBackupLog->cur->log);
        candidateNode->cur = candiNodeElement;
        candidateNode->prev = NULL;
        candidateNode->next = NULL;

        // 로그 리스트에 로그 노드 추가
        logList *curr;
        if (candidateLoglist == NULL)
        {
          // mainlogList가 비어 있는 경우
          candidateLoglist = candidateNode;
          candidateCnt++;
        }
        else
        {
          curr = candidateLoglist;
          while (curr->next != NULL)
          {
            curr = curr->next;
            candidateCnt++;
          }
          curr->next = candidateNode;
          candidateNode->prev = curr;
          candidateCnt++;
        }
      }

      currBackupLog = currBackupLog->next;
    }

    logList *currCandiLog = candidateLoglist;

    // 2) candidateList 크기가 한개일경우
    if (candidateCnt == 1)
    {
      struct stat statbuf;
      // 주어진 파일의 속성을 확인
      if (lstat(currCandiLog->cur->backuplog, &statbuf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", currCandiLog->cur->backuplog);
        return 1;
      }
      // 2-1) originPath에 백업파일을 그대로 복사한다. (originPath에 파일이 있다면 덮어쓰기)
      if ((fd3 = open(currCandiLog->cur->backuplog, O_WRONLY | O_CREAT | O_TRUNC, 777)) < 0)
      {
        fprintf(stderr, "ERROR: open error for %s\n", originPath);
        return 1;
      }
      while ((len = read(fd3, buf, statbuf.st_size)) > 0)
      {
        write(fd2, buf, len);
      }
      // 2-2) 백업파일을 삭제한다.
      if (remove(currCandiLog->cur->backuplog) < 0)
      {
        fprintf(stderr, "ERROR : remove error %s\n", currCandiLog->cur->backuplog);
        return 1;
      }
      // 2-3) 복구 성공시 메세지 출력
      char logStr[PATHMAX * 2] = "";
      char *backupTime = splitBackupDate(currCandiLog->cur->backuplog);

      printf("\"%s\"recovered to \"%s\"\n", currCandiLog->cur->backuplog, originPath);
      strcat(logStr, backupTime);
      strcat(logStr, " : ");
      strcat(logStr, "\"");
      strcat(logStr, currCandiLog->cur->backuplog);
      strcat(logStr, "\"");
      strcat(logStr, " recovered to ");
      strcat(logStr, "\"");
      strcat(logStr, originPath);
      strcat(logStr, "\"");
      strcat(logStr, "\n");
      // 메타데이터도 지워주기
      if ((clearMetaData(currCandiLog->cur->backuplog)) < 0)
      {
        fprintf(stderr, "Error : clean meta data error\n");
      }

      // 2-4) 복구 성공시 로그 append
      if (write(fd1, logStr, strlen(logStr)) == -1)
      {
        fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
        return 1;
      }

      // 2-5) 현재 백업디렉토리가 빈폴더라면 삭제하기

      char backupLog[PATHMAX];
      strcpy(backupLog, currCandiLog->cur->backuplog);
      char backupTimeTemp[PATHMAX];
      strcpy(backupTimeTemp, currCandiLog->cur->backuplog);
      char *backuptime = strstr(backupTimeTemp, backupPATH);
      if (backuptime != NULL)
      {
        strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
      }
      // 백업시간만 잘라내기
      char *realBackupTime = strtok(backuptime, "/");

      // currCandiLogg->cur->backuplog에서 backupPATH/realBackupTime 제거하고
      // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
      char backupTmpPath[PATHMAX] = "";
      strcat(backupTmpPath, currCandiLog->cur->backuplog);
      char *subDirPath;
      char tmpBackDir[PATHMAX] = "";
      strcat(tmpBackDir, backupPATH);
      strcat(tmpBackDir, "/");
      strcat(tmpBackDir, realBackupTime);
      // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

      queue Queue2;
      initQueue(&Queue2);
      enqueue(&Queue2, tmpBackDir);

      while (1)
      {
        if (isEmpty(&Queue2))
        {
          break;
        }
        char *nodePath = dequeue(&Queue2);

        struct stat statbuf;
        if (access(nodePath, F_OK) == 0)
        {
          if (lstat(nodePath, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
            return 1;
          }
          // 디렉토리인 애들 중
          // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
          if (S_ISDIR(statbuf.st_mode))
          {

            int subCnt;
            struct dirent **subNameList;
            char prevPath[PATHMAX] = "";

            if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
              return 1;
            }

            if (subCnt == 2)
            {
              if (remove(nodePath) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                return 1;
              }
            }
            else if (subCnt >= 3)
            {

              for (int i = 0; i < subCnt; i++)
              {
                if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                  continue;

                struct stat substatbuf;
                if (lstat(nodePath, &substatbuf) < 0)
                {
                  fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                  return 1;
                }
                if (S_ISDIR(substatbuf.st_mode))
                {
                  strcpy(prevPath, nodePath);

                  strcat(nodePath, "/");
                  strcat(nodePath, subNameList[i]->d_name);

                  // 만약 새로운경로가 디렉이면 큐에 넣고
                  if (lstat(nodePath, &substatbuf) < 0)
                  {
                    fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                    return 1;
                  }
                  if (S_ISDIR(substatbuf.st_mode))
                  {
                    enqueue(&Queue2, nodePath);
                    // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                    if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                    {
                      fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                      return 1;
                    }
                    if (subCnt == 2)
                    {
                      enqueue(&Queue2, prevPath);
                    }
                  }
                }
              }
            }
          }
        }
      }

      // 백업폴더도 지워주기
      int cnt;
      struct dirent **nameList;

      if (access(tmpBackDir, F_OK) == 0)
      {
        if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
        {
          fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
          return 1;
        }
        if (cnt == 2)
        {
          if (remove(tmpBackDir) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
            return 1;
          }
        }
      }
    }
    else if (candidateCnt > 1)
    {
      // 3) candidateList 크기가 두개 이상일 경우
      printf("backup files of %s\n", originPath);
      printf("0. exit\n");
      int iidx = 1;
      // 3-1) 후보인 로그 리스트로 뽑아내기
      while (true)
      {
        if (currCandiLog == NULL)
        {
          break;
        }
        char backupLog[PATHMAX] = "";
        strcpy(backupLog, currCandiLog->cur->backuplog);
        char backupTimeTemp[PATHMAX] = "";
        strcpy(backupTimeTemp, currCandiLog->cur->backuplog);
        char *backuptime = strstr(backupTimeTemp, backupPATH);
        if (backuptime != NULL)
        {
          strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
        }
        // 백업시간만 잘라내기
        char *realBackupTime = strtok(backuptime, "/");
        // 파일 크기 알아내기
        struct stat file_stat;
        if (stat(currCandiLog->cur->backuplog, &file_stat) == -1)
        {
          fprintf(stderr, "ERROR: stat error\n");
          return 1;
        }
        printf("%d. %s     %lldbytes\n", iidx, realBackupTime, (long long)file_stat.st_size);

        currCandiLog = currCandiLog->next;
        iidx++;
      }

      // 3-2) 유저의 입력받기
      int userInputIdx;
      printf(">>");
      scanf("%d", &userInputIdx);
      if (userInputIdx == 0)
      {
        exit(1);
      }

      // 3-3) 유저입력과 같은 번째의 노드인경우 복구 갈기기
      iidx = 1;
      currCandiLog = candidateLoglist;
      while (true)
      {
        if (currCandiLog == NULL)
        {
          break;
        }
        if (iidx == userInputIdx)
        {

          struct stat statbuf;
          // 주어진 파일의 속성을 확인
          if (lstat(currCandiLog->cur->backuplog, &statbuf) < 0)
          {
            fprintf(stderr, "ERROR: lstat error for %s\n", currCandiLog->cur->backuplog);
            return 1;
          }
          // 2-1) originPath에 백업파일을 그대로 복사한다. (originPath에 파일이 있다면 덮어쓰기)
          if ((fd3 = open(currCandiLog->cur->backuplog, O_WRONLY | O_CREAT | O_TRUNC, 777)) < 0)
          {
            fprintf(stderr, "ERROR: open error for %s\n", originPath);
            return 1;
          }
          while ((len = read(fd3, buf, statbuf.st_size)) > 0)
          {
            write(fd2, buf, len);
          }
          // 2-2) 백업파일을 삭제한다.
          if (remove(currCandiLog->cur->backuplog) < 0)
          {
            fprintf(stderr, "ERROR : remove error %s\n", currCandiLog->cur->backuplog);
            return 1;
          }
          // 2-3) 복구 성공시 메세지 출력
          char logStr[PATHMAX * 2] = "";
          char *backupTime = splitBackupDate(currCandiLog->cur->backuplog);

          printf("\"%s\"recovered to \"%s\"\n", currCandiLog->cur->backuplog, originPath);
          strcat(logStr, backupTime);
          strcat(logStr, " : ");
          strcat(logStr, "\"");
          strcat(logStr, currCandiLog->cur->backuplog);
          strcat(logStr, "\"");
          strcat(logStr, " recovered to ");
          strcat(logStr, "\"");
          strcat(logStr, originPath);
          strcat(logStr, "\"");
          strcat(logStr, "\n");

          // 메타데이터도 지워주기
          if ((clearMetaData(currCandiLog->cur->backuplog)) < 0)
          {
            fprintf(stderr, "Error : clean meta data error\n");
          }

          // 2-4) 복구 성공시 로그 append
          if (write(fd1, logStr, strlen(logStr)) == -1)
          {
            fprintf(stderr, "ERROR: write error for %s\n", ssubakLogPATH);
            return 1;
          }
          // 2-5) 현재 백업디렉토리가 빈폴더라면 삭제하기
          char backupLog[PATHMAX];
          strcpy(backupLog, currCandiLog->cur->backuplog);
          char backupTimeTemp[PATHMAX];
          strcpy(backupTimeTemp, currCandiLog->cur->backuplog);
          char *backuptime = strstr(backupTimeTemp, backupPATH);
          if (backuptime != NULL)
          {
            strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
          }
          // 백업시간만 잘라내기
          char *realBackupTime = strtok(backuptime, "/");

          // currCandiLog->cur->backuplog에서 backupPATH/realBackupTime 제거하고
          // 맨뒤에 파일명 제거한 부분이 서브디렉토리가 될것
          char backupTmpPath[PATHMAX] = "";
          strcat(backupTmpPath, currCandiLog->cur->backuplog);
          char *subDirPath;
          char tmpBackDir[PATHMAX] = "";
          strcat(tmpBackDir, backupPATH);
          strcat(tmpBackDir, "/");
          strcat(tmpBackDir, realBackupTime);
          // 현재 백업 경로를 bfs로 돌면서 subcnt==2이면 그 폴더 삭제

          queue Queue2;
          initQueue(&Queue2);
          enqueue(&Queue2, tmpBackDir);

          while (1)
          {
            if (isEmpty(&Queue2))
            {
              break;
            }
            char *nodePath = dequeue(&Queue2);

            struct stat statbuf;
            if (access(nodePath, F_OK) == 0)
            {
              if (lstat(nodePath, &statbuf) < 0)
              {
                fprintf(stderr, "ERROR2: lstat error for %s\n", nodePath);
                return 1;
              }
              // 디렉토리인 애들 중
              // subCnt가 2가 아니면 자기자신도 큐에 넣고 (나중에 돌아오면서 삭제위함) 서브디렉토리도 찾아서 걔를 큐에 넣는다.
              if (S_ISDIR(statbuf.st_mode))
              {

                int subCnt;
                struct dirent **subNameList;
                char prevPath[PATHMAX] = "";

                if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                {
                  fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                  return 1;
                }

                if (subCnt == 2)
                {
                  if (remove(nodePath) < 0)
                  {
                    fprintf(stderr, "ERROR : remove error %s\n", nodePath);
                    return 1;
                  }
                }
                else if (subCnt >= 3)
                {

                  for (int i = 0; i < subCnt; i++)
                  {
                    if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, ".."))
                      continue;

                    struct stat substatbuf;
                    if (lstat(nodePath, &substatbuf) < 0)
                    {
                      fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                      return 1;
                    }
                    if (S_ISDIR(substatbuf.st_mode))
                    {
                      strcpy(prevPath, nodePath);

                      strcat(nodePath, "/");
                      strcat(nodePath, subNameList[i]->d_name);

                      // 만약 새로운경로가 디렉이면 큐에 넣고
                      if (lstat(nodePath, &substatbuf) < 0)
                      {
                        fprintf(stderr, "ERROR3: lstat error for %s\n", nodePath);
                        return 1;
                      }
                      if (S_ISDIR(substatbuf.st_mode))
                      {
                        enqueue(&Queue2, nodePath);
                        // 빈 디렉이라면 이전 새로운 경로를 한번 더 넣어준다.
                        if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
                        {
                          fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
                          return 1;
                        }
                        if (subCnt == 2)
                        {
                          enqueue(&Queue2, prevPath);
                        }
                      }
                    }
                  }
                }
              }
            }
          }

          // 백업폴더도 지워주기
          int cnt;
          struct dirent **nameList;

          if (access(tmpBackDir, F_OK) == 0)
          {
            if ((cnt = scandir(tmpBackDir, &nameList, NULL, alphasort)) == -1)
            {
              fprintf(stderr, "ERROR: scandir error for %s\n", tmpBackDir);
              return 1;
            }
            if (cnt == 2)
            {
              if (remove(tmpBackDir) < 0)
              {
                fprintf(stderr, "ERROR : remove error %s\n", tmpBackDir);
                return 1;
              }
            }
          }
        }
        currCandiLog = currCandiLog->next;
        iidx++;
      }
    }
  }
  return 0;
}

int RecoverCommand(command_parameter *parameter)
{

  struct stat statbuf, tmpbuf;
  char originPath[PATHMAX];
  int fd, cnt, candidateIdx, candidateCnt;
  int originFileSize, tmpFileSize;
  struct dirent **namelist, **subNameList;
  logList *candidateLoglist = NULL;
  char filehash[PATHMAX] = "";
  char tmphash[PATHMAX] = "";
  char tmppath[PATHMAX] = "";

  strcpy(originPath, parameter->filename);
  strcpy(tmppath, parameter->tmpname);
  // 인자로 받아온 경로
  strcpy(tmppath, parameter->tmpname);

  // lstat을 사용해 디렉토리 속성을 가져옴
  if (lstat(originPath, &statbuf) < 0)
  {
    fprintf(stderr, "ERROR: lstat error for %s \n", originPath);
    return 1;
  }

  originFileSize = statbuf.st_size;

  // 디렉토리인데 옵션으로 -d 또는 -r로 지정되지 않았다면 에러 메시지를 출력하고 -1을 반환
  if (S_ISDIR(statbuf.st_mode) && !((parameter->commandopt & OPT_D) || (parameter->commandopt & OPT_R)))
  {
    fprintf(stderr, "ERROR: %s is a directory \n", originPath);
    return -1;
  }

  // 파일인데 옵션으로 -d 또는 -r로 지정되었다면 에러 메시지를 출력하고 -1을 반환
  if (S_ISREG(statbuf.st_mode) && ((parameter->commandopt & OPT_D) || (parameter->commandopt & OPT_R)))
  {
    fprintf(stderr, "ERROR!!: %s is a file  \n", originPath);
    return -1;
  }
  if ((cnt = scandir(backupPATH, &namelist, NULL, alphasort)) == -1)
  {
    fprintf(stderr, "ERROR: scandir error for %s\n", backupPATH);
    return 1;
  }
  if (parameter->commandopt & OPT_N)
  {

    if (lstat(tmppath, &tmpbuf) < 0)
    {
      fprintf(stderr, "ERROR: lstat error for %s \n", tmppath);
      return 1;
    }
    // 인자로 받아온 경로
    if (lstat(tmppath, &tmpbuf) < 0)
    {
      fprintf(stderr, "ERROR: lstat error for %s\n", tmppath);
      return 1;
    }
    // n 뒤에 경로 입력하지 않았다면 비정상 종료
    if ((originPath == NULL) && ((parameter->commandopt & OPT_N)))
    {
      fprintf(stderr, "ERROR: did not write a path %s \n", originPath);
      return -1;
    }
    // n 뒤에 경로가 올바르지 않는 경우
    if ((lstat(tmppath, &tmpbuf) < 0) && ((parameter->commandopt & OPT_N)))
    {
      fprintf(stderr, "ERROR: invalid file path for %s\n", parameter->filename);
      return -1;
    }
    // 경로가 홈 디렉토리를 벗어나는 경우, 백업 디렉토리에 백업되려고 할때
    if (strncmp(tmppath, homePATH, strlen(homePATH)) || !strncmp(tmppath, backupPATH, strlen(backupPATH)) || !strcmp(tmppath, homePATH))
    {
      fprintf(stderr, "ERROR: filename %s is out of home path \n", parameter->filename);

      return -1;
    }
    // d, r을 사용안했는데 n뒤에 경로가 존재하고 디렉인경우s
    if (S_ISDIR(tmpbuf.st_mode) && !((parameter->commandopt & OPT_D) || (parameter->commandopt & OPT_R)) && (parameter->commandopt & OPT_N) && parameter->tmpname)
    {
      fprintf(stderr, "ERROR: %s is a directory \n", originPath);
      return -1;
    }

    // d,r을 사용했는데 n뒤에 경로 존재하고 파일인경우
    if (S_ISREG(tmpbuf.st_mode) && ((parameter->commandopt & OPT_D) || (parameter->commandopt & OPT_R)) && (parameter->commandopt & OPT_N) && parameter->tmpname)
    {
      fprintf(stderr, "ERROR: %s is a file\n", originPath);
      return -1;
    }
  }

  if (S_ISDIR(statbuf.st_mode))
  {
    RecoverDir(originPath, tmppath, parameter);
  }
  else
  {
    //[예외처리] 인자로 입력받은 경로에 대한 백업 파일이 원본 파일과 내용이 같을 경우 복원을 진행하지 않고, (“원본 경로” not changed with “백업 경로”)를 표준 출력(5점)
    // 1) 링크드 리스트를 돌면서 해당 경로에 대한 백업파일을 리턴받는다
    // 2) 원본 파일과 백업파일의 해시값을 비교한다.
    char *searchBackupPath = findBackupFileByOriginPath(originPath);
    if (searchBackupPath == NULL)
    {
      fprintf(stderr, "ERROR : file does not exist in the backup list %s\n", originPath);
    }

    struct stat searchBackupBuf;
    if (lstat(searchBackupPath, &tmpbuf) < 0)
    {
      fprintf(stderr, "ERROR: lstat error for %s\n", tmppath);
      return 1;
    }
    tmpFileSize = searchBackupBuf.st_size;

    ConvertHash(originPath, filehash);
    ConvertHash(searchBackupPath, tmphash);
    if (!strcmp(filehash, tmphash) && (originFileSize == tmpFileSize))
    {
      printf("%s not changed with %s\n", originPath, searchBackupPath);
      exit(1);
    }
    RecoverFile(originPath, tmppath, parameter);
  }

  return 0;
}

void SystemExec(char **arglist)
{
  pid_t pid;
  char whichPath[PATHMAX];

  sprintf(whichPath, "/usr/bin/%s", arglist[0]);

  if ((pid = fork()) < 0)
  {
    fprintf(stderr, "ERROR: fork error\n");
    exit(1);
  }
  else if (pid == 0)
  {
    execv(whichPath, arglist);
    exit(0);
  }
  else
  {
    pid = wait(NULL);
  }
}

void ListParameterInit(list_parameter *parameter)
{
  parameter->command = (char *)malloc(sizeof(char) * PATHMAX);
  parameter->index = (char *)malloc(sizeof(char) * PATHMAX);
  parameter->tmpname = (char *)malloc(sizeof(char) * PATHMAX);
  // 명령어의 옵션 나타내는 플래그
  parameter->commandopt = 0;
}

void ListPrompt(int listSize)
{

  char input[STRMAX];
  int argcnt = 0;
  char **arglist = NULL;
  int command;
  int argc = 0;
  char *argvStr = NULL;
  list_parameter ListParameter = {(char *)0, (char *)0, (char *)0, 0};

  while (1)
  {
    int lastind;
    printf(">>");
    fgets(input, sizeof(input), stdin);

    if ((arglist = GetSubstring(input, &argcnt, " ")) == NULL)
    {
      continue;
    }
    if (argcnt == 0)
    {
      continue;
    }
    // 커멘드에 불필요한 공백이 생길 경우 제거
    for (int i = 0; i < argcnt; i++)
    {
      char *arg = arglist[i];
      arg = trim_left(arg);
      arg = trim_right(arg);
      strcpy(arglist[i], arg);
    }

    if (!strcmp(arglist[0], commandData[5]))
    {
      command = CMD_REM;
    }
    else if (!strcmp(arglist[0], commandData[6]))
    {
      command = CMD_REC;
    }
    else if (!strcmp(arglist[0], commandData[7]))
    {
      command = CMD_SYS;
    }
    else if (!strcmp(arglist[0], commandData[8]))
    {
      command = CMD_SYS;
    }
    else if (!strcmp(arglist[0], commandData[9]))
    {
      command = CMD_EXIT;
      printf("Program exit(0)\n");
      exit(0);
    }
    else
    {
      command = NOT_CMD;
    }

    // [예외처리] 잘못된 명령어 입력시
    if (command == NOT_CMD)
    {
      fprintf(stderr, "ERROR : invalid command\n");
      exit(1);
    }

    // [예외처리] 잘못된 인덱스 입력시
    if (command & (CMD_REM | CMD_REC))
    {
      ListParameter.command = arglist[0];
      ListParameter.index = arglist[1];
      int atoiIdx = atoi(ListParameter.index);
      if (atoiIdx < 0 || atoiIdx > listSize)
      {
        fprintf(stderr, "ERROR: invalid index number\n");
        exit(1);
      }
    }
    else if (command & (CMD_SYS))
    {
      SystemExec(arglist);
    }
    // 옵션처리
    lastind = 2;
  }
}

int ListCommand(command_parameter *parameter)
{

  struct stat statbuf, tmpbuf;
  char originPath[PATHMAX];
  int listSize = 0;
  int fd;

  strcpy(originPath, parameter->filename);
  if (lstat(originPath, &statbuf) < 0)
  {
    fprintf(stderr, "ERROR: lstat error for %s\n", originPath);
    return 1;
  }

  // 인자로 받아온 filename에 대한 리스트 출력
  // 1) 파일일 경우
  if (S_ISREG(statbuf.st_mode))
  {
    // originPath에 해당하는 백업 경로 가져오기
    char *backupPath = findBackupFileByOriginPath(originPath);
    char *backupDate = splitBackupDate(backupPath);
    // 백업파일의 사이즈 알아내기
    off_t filesize;
    fd = open(backupPath, O_RDONLY);
    filesize = lseek(fd, 0, SEEK_END);
    char *convertBackupFileSize = addCommas(filesize);
    printf("%d. %s        %s %sbytes\n", listSize, originPath, backupDate, convertBackupFileSize);

    ListPrompt(listSize);
  }
  // 2) 디렉토리일 경우
  if (S_ISDIR(statbuf.st_mode))
  {
  }

  return 0;
}

int HelpCommand(char *parameter)
{
  help(parameter);
  return 0;
}

// 파라미터 초기화 해주는 함수
void ParameterInit(command_parameter *parameter)
{
  parameter->command = (char *)malloc(sizeof(char) * PATHMAX);
  parameter->filename = (char *)malloc(sizeof(char) * PATHMAX);
  parameter->tmpname = (char *)malloc(sizeof(char) * PATHMAX);
  // 명령어의 옵션 나타내는 플래그
  parameter->commandopt = 0;
}

int ParameterProcessing(int argcnt, char **arglist, int command, command_parameter *parameter)
{
  struct stat buf; // 파일의 메타 데이터 저장함
  optind = 0;
  opterr = 0;
  int lastind;
  int option;
  int optcnt = 0;

  switch (command)
  {
  // 파일백업 명령어
  case CMD_BACKUP:
  {

    // 경로를 입력하지 않은 경우
    if (argcnt < 3)
    {
      fprintf(stderr, "Usage : backup <PATH> [OPTION] \n");
      return -1;
    }
    char *resolved = realpath(arglist[2], parameter->filename);
    // 경로가 올바르지 않은경우
    if (resolved == NULL)
    {
      fprintf(stderr, "ERROR: %s is invalid filepath\n", parameter->filename);
      return -1;
    }
    // 경로가 길이 제한을 넘는 경우
    if (strlen(resolved) > PATHMAX)
    {
      fprintf(stderr, "ERROR : %s exceeds 4096 bytes \n", parameter->filename);
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
    // 경로가 홈 디렉토리를 벗어나는 경우, 백업 디렉토리에 백업되려고 할때
    if (strncmp(parameter->filename, homePATH, strlen(homePATH)) || !strncmp(parameter->filename, backupPATH, strlen(backupPATH)) || !strcmp(parameter->filename, homePATH))
    {
      fprintf(stderr, "ERROR: filename %s can't be backuped %s \n", parameter->filename, backupPATH);

      return -1;
    }

    // 옵션 파싱과정에서 마지막으로 처리된 옵션 인덱스
    lastind = 2;

    while ((option = getopt(argcnt, arglist, "dry")) != -1)
    {

      // 옵션이 올바르지 않은 경우
      if (option != 'd' && option != 'r' && option != 'y')
      {
        fprintf(stderr, "ERROR: unknown option %c\n", optopt);
        printf("Usage:\n");
        printf("  > backup <PATH> [OPTION]... : backup file if <PATH> is file\n");
        printf("    -d : backup files in directory if <PATH> is directory \n");
        printf("    -r : backup files in directory recursive if <PATH> is directory \n");
        printf("    -y : backup file although already backuped\n");
        return -1;
      }
      // 옵션이 올바르지 않은 경우
      if (optind == lastind)
      {
        fprintf(stderr, "ERROR: wrong option input\n");
        printf("Usage:\n");
        printf("  > backup <PATH> [OPTION]... : backup file if <PATH> is file\n");
        printf("    -d : backup files in directory if <PATH> is directory \n");
        printf("    -r : backup files in directory recursive if <PATH> is directory \n");
        printf("    -y : backup file although already backuped\n");
        return -1;
      }
      if (option == 'd')
      {
        // 중복으로 사용했는지 검사
        if (parameter->commandopt & OPT_D)
        {
          fprintf(stderr, "ERROR: duplicate option -%c\n", option);
          return 1;
        }
        // 만약 -r이나 -y가 있다면 추가하지 않음
        if (!(parameter->commandopt & OPT_R) && !(parameter->commandopt & OPT_Y))
        {
          // 비트연산을 통해 옵션 추가
          parameter->commandopt |= OPT_D;
        }
      }

      if (option == 'r')
      {
        if (parameter->commandopt & OPT_R)
        {
          fprintf(stderr, "ERROR: duplicate option -%c\n", option);
          return 1;
        }
        // 만약 -y가 있다면 추가하지 않음
        if (!(parameter->commandopt & OPT_Y))
        {
          // 만약 -d가 있다면 AND연산을 통해 d제거
          if (parameter->commandopt & OPT_D)
          {
            parameter->commandopt &= ~OPT_D;
          }
          parameter->commandopt |= OPT_R;
        }
      }

      if (option == 'y')
      {
        if (parameter->commandopt & OPT_Y)
        {
          fprintf(stderr, "ERROR: duplicate option -%c\n", option);
          return -1;
        }
        // 만약 -d나 -r이 있다면 이를 삭제
        if (parameter->commandopt & OPT_D || parameter->commandopt & OPT_R)
        {
          parameter->commandopt &= ~OPT_D;
          parameter->commandopt &= ~OPT_R;
        }
        parameter->commandopt |= OPT_Y;
      }
      // 옵션 개수 세주기
      optcnt++;
      lastind = optind;
    }

    if (argcnt - optcnt != 3)
    {
      fprintf(stderr, "argument error\n");
      return -1;
    }

    BackupCommand(parameter);
    break;
  }
  case CMD_HELP:
  {
    HelpCommand(arglist[2]);
    break;
  }
  case CMD_REM:
  {
    if (argcnt < 3)
    {
      fprintf(stderr, "Usage : backup <PATH> [OPTION] \n");
      return -1;
    }

    char *resolved = realpath(arglist[2], parameter->filename);
    // 경로를 입력하지 않은 경우
    if (resolved == NULL)
    {
      fprintf(stderr, "ERROR: %s is invalid filepath\n", parameter->filename);
      printf("Usage:\n");
      printf("    -d : remove backuped files in directory if <PATH> is directory \n");
      printf("    -r : remove backuped files in directory recursive if <PATH> is directory \n");
      printf("    -a : remove all backuped files \n");
      return -1;
    }
    // 경로가 길이 제한을 넘는 경우
    if (strlen(resolved) > PATHMAX)
    {
      fprintf(stderr, "ERROR : %s exceeds 4096 bytes \n", parameter->filename);
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

    // 경로가 홈 디렉토리를 벗어나는 경우, 백업 디렉토리에 백업되려고 할때
    if (strncmp(parameter->filename, homePATH, strlen(homePATH)) || !strncmp(parameter->filename, backupPATH, strlen(backupPATH)) || !strcmp(parameter->filename, homePATH))
    {
      fprintf(stderr, "ERROR: filename %s is out of home path \n", parameter->filename);

      return -1;
    }

    lastind = 2;
    while ((option = getopt(argcnt, arglist, "dra")) != -1)
    {

      // 옵션이 올바르지 않은 경우
      if (option != 'd' && option != 'r' && option != 'a')
      {
        fprintf(stderr, "ERROR: unknown option %c\n", optopt);
        printf("Usage:\n");
        printf("    -d : remove backuped files in directory if <PATH> is directory \n");
        printf("    -r : remove backuped files in directory recursive if <PATH> is directory \n");
        printf("    -a : remove all backuped files \n");
        return -1;
      }
      // 옵션이 올바르지 않은 경우
      if (optind == lastind)
      {
        fprintf(stderr, "ERROR: wrong option input\n");
        printf("Usage:\n");
        printf("    -d : remove backuped files in directory if <PATH> is directory \n");
        printf("    -r : remove backuped files in directory recursive if <PATH> is directory \n");
        printf("    -a : remove all backuped files \n");
        return -1;
      }

      if (option == 'd')
      {
        // 중복으로 사용했는지 검사
        if (parameter->commandopt & OPT_D)
        {
          fprintf(stderr, "ERROR: duplicate option -%c\n", option);
          return -1;
        }
        // 만약 -r이 있다면 추가 x
        if (!(parameter->commandopt & OPT_R))
        {

          // 비트연산을 통해 옵션 추가
          parameter->commandopt |= OPT_D;
        }
      }

      if (option == 'r')
      {
        if (parameter->commandopt & OPT_R)
        {
          fprintf(stderr, "ERROR: duplicate option -%c\n", option);
          return -1;
        }
        // 만약 -d가 있다면 AND연산을 통해 d제거
        if (parameter->commandopt & OPT_D)
        {
          parameter->commandopt &= ~OPT_D;
        }
        parameter->commandopt |= OPT_R;
      }

      if (option == 'a')
      {
        if (parameter->commandopt & OPT_A)
        {
          fprintf(stderr, "ERROR: duplicate option -%c\n", option);
          return -1;
        }
        parameter->commandopt |= OPT_A;
      }
      // 옵션 개수 세주기
      optcnt++;
      lastind = optind;
    }
    int res = argcnt - optcnt;

    if (res != 3)
    {
      fprintf(stderr, "argument error\n");
      return -1;
    }

    RemoveCommand(parameter);
    break;
  }
  case CMD_REC:
  {
    //
    if (argcnt < 3)
    {
      fprintf(stderr, "Usage : backup <PATH> [OPTION] \n");
      return -1;
    }

    char *resolved = realpath(arglist[2], parameter->filename);
    // 경로를 입력하지 않은 경우
    if (resolved == NULL)
    {
      fprintf(stderr, "ERROR: %s is invalid filepath\n", parameter->filename);
      printf("Usage:\n");
      printf("    -d : recover backuped files in directory if <PATH> is directory\n");
      printf("    -r : recover backuped files in directory recursive if <PATH> is directory \n");
      printf("    -l : recover latest backuped files \n");
      printf("    -n <NEW_PATH> : recover backuped file with new path \n");
      return -1;
    }
    // 경로가 길이 제한을 넘는 경우
    if (strlen(resolved) > PATHMAX)
    {
      fprintf(stderr, "ERROR : %s exceeds 4096 bytes \n", parameter->filename);
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

    // 경로가 홈 디렉토리를 벗어나는 경우, 백업 디렉토리에 백업되려고 할때
    if (strncmp(parameter->filename, homePATH, strlen(homePATH)) || !strncmp(parameter->filename, backupPATH, strlen(backupPATH)))
    {
      fprintf(stderr, "ERROR: filename %s is out of home path \n", parameter->filename);

      return -1;
    }

    // 경로가 홈 디렉토리를 벗어나는 경우, 백업 디렉토리에 백업되려고 할때
    if (strncmp(parameter->filename, homePATH, strlen(homePATH)) || !strncmp(parameter->filename, backupPATH, strlen(backupPATH)) || !strcmp(parameter->filename, homePATH))
    {
      fprintf(stderr, "ERROR: filename %s is out of home path \n", parameter->filename);

      return -1;
    }

    lastind = 2;
    while ((option = getopt(argcnt, arglist, "drln:")) != -1)
    {

      // 옵션이 올바르지 않은 경우
      if (option != 'd' && option != 'r' && option != 'l' && option != 'n')
      {
        fprintf(stderr, "ERROR: unknown option %c\n", optopt);
        printf("Usage:\n");
        printf("    -d : recover backuped files in directory if <PATH> is directory\n");
        printf("    -r : recover backuped files in directory recursive if <PATH> is directory \n");
        printf("    -l : recover latest backuped files \n");
        printf("    -n <NEW_PATH> : recover backuped file with new path \n");
        return -1;
      }
      // 옵션이 올바르지 않은 경우
      if (optind == lastind)
      {
        fprintf(stderr, "ERROR: wrong option input\n");
        printf("Usage:\n");
        printf("    -d : recover backuped files in directory if <PATH> is directory\n");
        printf("    -r : recover backuped files in directory recursive if <PATH> is directory \n");
        printf("    -l : recover latest backuped files \n");
        printf("    -n <NEW_PATH> : recover backuped file with new path \n");
        return -1;
      }

      if (option == 'd')
      {
        // 중복으로 사용했는지 검사
        if (parameter->commandopt & OPT_D)
        {
          fprintf(stderr, "ERROR: duplicate option -%c\n", option);
          return -1;
        }
        // 만약 -r이 있다면 추가 x
        if (!(parameter->commandopt & OPT_R))
        {

          // 비트연산을 통해 옵션 추가
          parameter->commandopt |= OPT_D;
        }
      }

      if (option == 'r')
      {
        if (parameter->commandopt & OPT_R)
        {
          fprintf(stderr, "ERROR: duplicate option -%c\n", option);
          return -1;
        }
        // 만약 -d가 있다면 AND연산을 통해 d제거
        if (parameter->commandopt & OPT_D)
        {
          parameter->commandopt &= ~OPT_D;
        }
        parameter->commandopt |= OPT_R;
      }

      if (option == 'l')
      {
        if (parameter->commandopt & OPT_L)
        {
          fprintf(stderr, "ERROR: duplicate option -%c\n", option);
          return -1;
        }
        parameter->commandopt |= OPT_L;
      }

      if (option == 'n')
      {
        char *newPath = optarg;

        realpath(newPath, parameter->tmpname);
        if (parameter->commandopt & OPT_N)
        {
          fprintf(stderr, "ERROR: duplicate option -%c\n", option);
          return -1;
        }
        parameter->commandopt |= OPT_N;
      }
      // 옵션 개수 세주기
      optcnt++;
      lastind = optind;
    }

    int res = argcnt - optcnt;
    if (res != 3 && res != 4)
    {
      fprintf(stderr, "argument error\n");
      return -1;
    }

    RecoverCommand(parameter);
    break;
  }
  case CMD_LIST:
  {
    // 경로를 입력하지 않은 경우
    if (argcnt < 3)
    {
      fprintf(stderr, "Usage : list <PATH> [OPTION] \n");
      return -1;
    }
    char *resolved = realpath(arglist[2], parameter->filename);
    // 경로가 올바르지 않은경우
    if (resolved == NULL)
    {
      fprintf(stderr, "ERROR: %s is invalid filepath\n", parameter->filename);
      return -1;
    }
    // 경로가 길이 제한을 넘는 경우
    if (strlen(resolved) > PATHMAX)
    {
      fprintf(stderr, "ERROR : %s exceeds 4096 bytes \n", parameter->filename);
      return -1;
    }

    // 파일이나 디렉토리가 존재하지 않는 경우
    if (lstat(parameter->filename, &buf) < 0)
    {
      fprintf(stderr, "ERROR: lstat error for %s\n", parameter->filename);
      return -1;
    }

    // 경로가 홈 디렉토리를 벗어나는 경우, 백업 디렉토리에 백업되려고 할때
    if (strncmp(parameter->filename, homePATH, strlen(homePATH)) || !strncmp(parameter->filename, backupPATH, strlen(backupPATH)) || !strcmp(parameter->filename, homePATH))
    {
      fprintf(stderr, "ERROR: filename %s is out of home path \n", parameter->filename);

      return -1;
    }

    ListCommand(parameter);
    break;
  }
  }
}

int Init()
{

  struct stat statbuf;
  int cnt, fd, subCnt, fd2;
  struct dirent **namelist, **subNameList;
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
    // 특정 사용자 이름(loginName)에 대한 사용자 계정 정보를 검색하고, 그 결과를 pw 변수에 저장
    pw = getpwnam(loginName);
  }

  if (pw == NULL)
  {
    printf("ERROR: Failed to get user information\n");
    return 1;
  }
  // 실행경로 받아오기
  getcwd(exePATH, PATHMAX);

  // Home경로 받아오기 (출력하지 않고 지정된 버퍼에 저장)
  sprintf(homePATH, "%s", pw->pw_dir);
  // backup경로 받아오기
  sprintf(backupPATH, "%s/backup", pw->pw_dir);
  sprintf(ssubakLogPATH, "%s/backup/ssubak.log", pw->pw_dir);
  // 메타데이터 경로받아오기
  sprintf(metaPATH, "%s/kathy.meta.txt", pw->pw_dir);

  // backup 경로에 폴더가 없다면 만들어주기
  if (access(backupPATH, F_OK))
  {
    mkdir(backupPATH, 0777);
  }
  // ssubak.log파일이 없다면 생성
  if (access(ssubakLogPATH, F_OK))
  {
    fd = 0;
    if ((fd = creat(ssubakLogPATH, 0666)) < 0)
    {
      fprintf(stderr, "error for creating ssubak.log file\n");
      return 1;
    }
  }
  // 메타 데이터 경로에 메타데이터가 없으면 생성
  if (access(metaPATH, F_OK))
  {
    fd2 = 0;
    if ((fd2 = creat(metaPATH, 0666)) < 0)
    {
      fprintf(stderr, "error for creating meta data file\n");
      return 1;
    }
  }

  // 전역 백업 링크드리스트로 상태관리

  if (refreshBackupLinkedList() < 0)
  {
    fprintf(stderr, "error for refresh backup status\n");
    return 1;
  }

  return 0;
}

int main(int argc, char *argv[])
{
  bool isCommandValid = false;
  int cmd;
  char argvStr[PATHMAX] = "";
  int argcnt = 0;
  char **arglist = NULL;
  // 초기화
  command_parameter parameter = {(char *)0, (char *)0, (char *)0, 0};

  Init();
  // GetSubstring에 전달할 명령어를 문자열로 이어붙임
  for (int i = 0; i < argc; i++)
  {
    strcat(argvStr, argv[i]);
    strcat(argvStr, " ");
  }

  arglist = GetSubstring(argvStr, &argcnt, " ");

  // 첫번째 인자를 입력하지 않을 경우 프로그램 비정상 종료
  if (argc == 1)
  {
    fprintf(stderr, "ERROR : invalid command error %s \n", argv[1]);
    exit(1);
  }

  // 첫번째 인자로 잘못된 명령어를 입력할 경우 프로그램 비정상 종료
  for (int i = 0; i < CMD_COUNT; i++)
  {
    if (!strcmp(commandData[i], argv[1]))
    {
      isCommandValid = true;
    }
  }
  if (!isCommandValid)
  {
    fprintf(stderr, "ERROR : unknown command error %s \n", argv[1]);
    exit(1);
  }

  // argv의 1번째 인자에 따라 command 설정해주기 (정수로)
  if (!strcmp(arglist[1], commandData[0]))
  {
    cmd = CMD_BACKUP;
  }
  else if (!strcmp(arglist[1], commandData[1]))
  {
    cmd = CMD_REM;
  }
  else if (!strcmp(arglist[1], commandData[2]))
  {
    cmd = CMD_REC;
  }
  else if (!strcmp(arglist[1], commandData[3]))
  {
    cmd = CMD_LIST;
  }
  else if (!strcmp(arglist[1], commandData[4]))
  {
    cmd = CMD_HELP;
  }
  else
  {
    cmd = NOT_CMD;
  }

  ParameterInit(&parameter);
  parameter.command = arglist[0];
  ParameterProcessing(argcnt, arglist, cmd, &parameter);

  exit(0);
}
