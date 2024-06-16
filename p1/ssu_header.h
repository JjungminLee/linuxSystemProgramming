
#define OPENSSL_API_COMPAT 0x10100000L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <wait.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <ctype.h>
#include <pwd.h>

#define true 1
#define false 0

#define HASH_MD5 33
#define HASH_SHA1 41

#define NAMEMAX 255
#define PATHMAX 4096
#define STRMAX 4096
#define QUEUE_SIZE 200
#define BUFFER_SIZE 4096

#define CMD_BACKUP 0b0000001
#define CMD_REM 0b0000010
#define CMD_REC 0b0000100
#define CMD_LIST 0b0001000
#define CMD_HELP 0b0010000
#define CMD_SYS 0b0100000
#define CMD_EXIT 0b1000000
#define NOT_CMD 0b0000000
#define CMD_COUNT 5

#define OPT_D 0b000001
#define OPT_R 0b000010
#define OPT_Y 0b000100
#define OPT_N 0b001000
#define OPT_A 0b010000
#define OPT_L 0b100000
#define NOT_OPT 0b000000

char exeNAME[PATHMAX];
char exePATH[PATHMAX];
char homePATH[PATHMAX];
char backupPATH[PATHMAX];
char ssubakLogPATH[PATHMAX];
char metaPATH[PATHMAX];
int hash;

void help();

char *commandData[10] = {
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

typedef struct command_parameter
{
  char *command;
  char *filename;
  char *tmpname;
  int commandopt;
  char *argv[10];
} command_parameter;

typedef struct list_parameter
{
  char *command;
  char *index;
  char *tmpname;
  int commandopt;
  char *argv[10];
} list_parameter;

typedef struct backupNode
{
  char backupPath[PATHMAX];
  char newPath[PATHMAX];
  struct stat statbuf;

  struct backupNode *next;
} backupNode;

typedef struct fileNode
{
  char path[PATHMAX];
  struct stat statbuf;

  backupNode *head;

  struct fileNode *next;
} fileNode;

typedef struct dirNode
{
  char path[PATHMAX];
  char backupPath[PATHMAX];
  char newPath[PATHMAX];

  fileNode *head;

  struct dirNode *next;
} dirNode;

typedef struct dirList
{
  struct dirNode *head;
  struct dirNode *tail;
} dirList;

dirList *mainDirList;

typedef struct pathList_
{
  struct pathList_ *next;
  struct pathList_ *prev;
  char path[NAMEMAX];

} pathList;

typedef struct logElement
{
  char log[PATHMAX];
  char backuplog[PATHMAX];
} logElement;

// 전역으로 백업상태관리 리스트
typedef struct backupList
{
  struct logElement *cur;
  struct backupList *prev;
  struct backupList *next;
} backupList;

backupList *mainBackupList;
logElement *backupLogElement;

typedef struct logList
{
  struct logElement *cur;
  struct logList *prev;
  struct logList *next;
} logList;

typedef struct metaList
{
  struct logElement *cur;
  struct metaList *prev;
  struct metaList *next;
} metaList;

logList *mainlogList;
logElement *logNode;
logElement *metaNode;
logList *mainlogList;
metaList *mainMetaList;

typedef struct node
{
  char *data;
  struct node *next;
} node;

typedef struct queue
{
  node *front;
  node *rear;
  int count;
} queue;
void initQueue(queue *q)
{
  q->front = q->rear = NULL;
  q->count = 0;
}
int isEmpty(queue *q)
{
  return q->count == 0;
}
void enqueue(queue *queue, char *data)
{

  node *newNode = (node *)malloc(sizeof(node) * QUEUE_SIZE);
  if (newNode == NULL)
  {
    fprintf(stderr, "ERROR: Memory allocation failed\n");
    return;
  }
  newNode->data = strdup(data);
  if (newNode->data == NULL)
  {
    fprintf(stderr, "ERROR: Memory allocation failed\n");
    free(newNode);
    return;
  }
  newNode->next = NULL;
  if (isEmpty(queue))
  {
    queue->front = newNode;
  }
  else
  {
    queue->rear->next = newNode;
  }
  queue->rear = newNode;
  queue->count++;
}

char *dequeue(queue *queue)
{

  char *data;
  node *ptr;

  if (isEmpty(queue))
  {
    printf("ERROR: Queue is Empty\n");
    exit(1);
  }
  ptr = queue->front;

  if (ptr == NULL)
  {
    printf("ERROR: Node or Data is NULL\n");
    exit(1);
  }
  if (ptr == NULL || ptr->data == NULL)
  {
    printf("ERROR: Node or Data is NULL\n");
    exit(1);
  }

  int dataLength = strlen(ptr->data);
  data = (char *)malloc(dataLength + 1);
  if (data == NULL)
  {
    printf("ERROR: Memory allocation failed\n");
    exit(1);
  }

  strcpy(data, ptr->data);
  queue->front = ptr->next;
  queue->count--;
  if (ptr->data != NULL)
  {
    free(ptr->data); // 데이터 메모리 해제 추가
  }
  free(ptr);
  return data;
}

void help();

// evp방식으로 변경
int md5(char *target_path, char *hash_result)
{

  FILE *fp;
  unsigned char hash[MD5_DIGEST_LENGTH];
  unsigned char buffer[BUFSIZ];
  int bytes = 0;
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();

  if ((fp = fopen(target_path, "rb")) == NULL)
  {
    printf("ERROR: fopen error for %s\n", target_path);
    EVP_MD_CTX_free(mdctx);
    return 1;
  }
  // 이전 버전의 	MD5_Init(&md5);
  EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);

  while ((bytes = fread(buffer, 1, BUFSIZ, fp)) > 0)
    EVP_DigestUpdate(mdctx, buffer, bytes);

  EVP_DigestFinal_ex(mdctx, hash, NULL);

  for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    sprintf(hash_result + (i * 2), "%02x", hash[i]);
  hash_result[HASH_MD5 - 1] = 0;

  fclose(fp);
  EVP_MD_CTX_free(mdctx);

  return 0;
}

// int md5(char *target_path, char *hash_result)
// {
// 	FILE *fp;
// 	unsigned char hash[MD5_DIGEST_LENGTH];
// 	unsigned char buffer[SHRT_MAX];
// 	int bytes = 0;
// 	MD5_CTX md5;

// 	if ((fp = fopen(target_path, "rb")) == NULL){
// 		printf("ERROR: fopen error for %s\n", target_path);
// 		return 1;
// 	}

// 	MD5_Init(&md5);

// 	while ((bytes = fread(buffer, 1, SHRT_MAX, fp)) != 0)
// 		MD5_Update(&md5, buffer, bytes);

// 	MD5_Final(hash, &md5);

// 	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
// 		sprintf(hash_result + (i * 2), "%02x", hash[i]);
// 	hash_result[HASH_MD5-1] = 0;

// 	fclose(fp);

// 	return 0;
// }

int ConvertHash(char *target_path, char *hash_result)
{
  md5(target_path, hash_result);
}

int cmpHash(char *path1, char *path2)
{
  char *hash1 = (char *)malloc(sizeof(char) * hash);
  char *hash2 = (char *)malloc(sizeof(char) * hash);

  ConvertHash(path1, hash1);
  ConvertHash(path2, hash2);

  return strcmp(hash1, hash2);
}

char *cvtNumComma(int a)
{
  char *str = (char *)malloc(sizeof(char) * STRMAX);
  char *ret = (char *)malloc(sizeof(char) * STRMAX);
  int i;
  for (i = 0; a > 0; i++)
  {
    str[i] = a % 10 + '0';
    a /= 10;
    if (i % 4 == 2)
    {
      i++;
      str[i] = ',';
    }
  }
  str[i] = '\0';

  for (i = 0; i < strlen(str); i++)
  {
    ret[i] = str[strlen(str) - i - 1];
  }
  ret[i] = '\0';

  return ret;
}

char *GetFileName(char file_path[])
{
  char *file_name;

  while (*file_path)
  {
    if (*file_path == '/' && (file_path + 1) != NULL)
    {
      file_name = file_path + 1;
    }
    file_path++;
  }
  return file_name;
}

char *strToHex(char *str)
{
  char *result = (char *)malloc(sizeof(char) * PATHMAX);
  for (int i = 0; i < strlen(str); i++)
  {
    sprintf(result + (i * 2), "%02X", str[i]);
  }
  result[strlen(str) * 2] = '\0';

  return result;
}

char *getDate()
{
  char *date = (char *)malloc(sizeof(char) * 50);
  time_t timer;
  struct tm *t;

  timer = time(NULL);
  t = localtime(&timer);

  sprintf(date, "%02d%02d%02d%02d%02d%02d", t->tm_year % 100, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

  return date;
}

char *strTodate(char *old_str, char **date)
{
  char *result = (char *)calloc(sizeof(char), PATHMAX);
  *date = getDate();

  sprintf(result, "%s_%s", old_str, *date);

  return result;
}

char *strTodirPATH(char *dirpath, char *filepath)
{
  char *fullpath = (char *)malloc(sizeof(char) * PATHMAX);
  strcpy(fullpath, dirpath);
  strcat(fullpath, "/");
  if (filepath != NULL)
    strcat(fullpath, filepath);
  return fullpath;
}

// 문자와 구분자를 받아들여 해당 구분자 이후의 문자열 반환
char *QuoteCheck(char **str, char del)
{
  char *tmp = *str + 1;
  int i = 0;

  while (*tmp != '\0' && *tmp != del)
  {
    tmp++;
    i++;
  }
  // 구분자를 발견 못하고 문자열 끝에 도달
  if (*tmp == '\0')
  {
    *str = tmp;
    return NULL;
  }
  // 구분자를 발견한 경우
  if (*tmp == del)
  {
    for (char *c = *str; *c != '\0'; c++)
    {
      *c = *(c + 1);
    }
    *str += i;
    for (char *c = *str; *c != '\0'; c++)
    {
      *c = *(c + 1);
    }
  }
}

// 구분자 기준으로 토큰 분리
char *Tokenize(char *str, char *del)
{
  int i = 0;
  int del_len = strlen(del);
  static char *tmp = NULL;
  char *tmp2 = NULL;

  if (str != NULL && tmp == NULL)
  {
    tmp = str;
  }

  if (str == NULL && tmp == NULL)
  {
    return NULL;
  }

  char *idx = tmp;

  while (i < del_len)
  {
    if (*idx == del[i])
    {
      idx++;
      i = 0;
    }
    else
    {
      i++;
    }
  }
  if (*idx == '\0')
  {
    tmp = NULL;
    return tmp;
  }
  tmp = idx;

  while (*tmp != '\0')
  {
    if (*tmp == '\'' || *tmp == '\"')
    {
      QuoteCheck(&tmp, *tmp);
      continue;
    }
    for (i = 0; i < del_len; i++)
    {
      if (*tmp == del[i])
      {
        *tmp = '\0';
        break;
      }
    }
    tmp++;
    if (i < del_len)
    {
      break;
    }
  }

  return idx;
}

char **GetSubstring(char *str, int *cnt, char *del)
{
  *cnt = 0;
  int i = 0;
  char *token = NULL;
  char *templist[100] = {
      NULL,
  };
  token = Tokenize(str, del);
  if (token == NULL)
  {
    return NULL;
  }

  while (token != NULL)
  {
    templist[*cnt] = token;
    *cnt += 1;
    token = Tokenize(NULL, del);
  }

  char **temp = (char **)malloc(sizeof(char *) * (*cnt + 1));
  for (i = 0; i < *cnt; i++)
  {
    temp[i] = templist[i];
  }

  return temp;
}

int ConvertPath(char *origin, char *resolved)
{
  int idx = 0;
  int i;
  char *path = (char *)malloc(sizeof(char) * PATHMAX * 2);
  char *tmppath = (char *)malloc(sizeof(char) * PATHMAX);
  char **pathlist;
  int pathcnt;

  if (origin == NULL)
  {
    return -1;
  }

  if (origin[0] == '~')
  {
    sprintf(path, "%s%s", homePATH, origin + 1);
  }
  else if (origin[0] != '/')
  {
    sprintf(path, "%s/%s", exePATH, origin);
  }
  else
  {
    sprintf(path, "%s", origin);
  }

  if (!strcmp(path, "/"))
  {
    resolved = "/";
    return 0;
  }

  if ((pathlist = GetSubstring(path, &pathcnt, "/")) == NULL)
  {
    return -1;
  }

  pathList *headpath = (pathList *)malloc(sizeof(pathList));
  pathList *currpath = headpath;

  for (i = 0; i < pathcnt; i++)
  {
    if (!strcmp(pathlist[i], "."))
    {
      continue;
    }
    else if (!strcmp(pathlist[i], ".."))
    {
      currpath = currpath->prev;
      currpath->next = NULL;
      continue;
    }

    pathList *newpath = (pathList *)malloc(sizeof(pathList));
    strcpy(newpath->path, pathlist[i]);
    currpath->next = newpath;
    newpath->prev = currpath;

    currpath = currpath->next;
  }

  currpath = headpath->next;

  strcpy(tmppath, "/");
  while (currpath != NULL)
  {
    strcat(tmppath, currpath->path);
    if (currpath->next != NULL)
    {
      strcat(tmppath, "/");
    }
    currpath = currpath->next;
  }

  strcpy(resolved, tmppath);

  return 0;
}

int cmpPath(char *path1, char *path2)
{
  int i;
  int cnt1, cnt2;
  char tmp1[PATHMAX], tmp2[PATHMAX];
  strcpy(tmp1, path1);
  strcpy(tmp2, path2);
  char **pathlist1 = GetSubstring(tmp1, &cnt1, "/");
  char **pathlist2 = GetSubstring(tmp2, &cnt2, "/");

  if (cnt1 == cnt2)
  {
    for (i = 0; i < cnt1; i++)
    {
      if (!strcmp(pathlist1[i], pathlist2[i]))
        continue;
      return -strcmp(pathlist1[i], pathlist2[i]);
    }
  }
  else
  {
    return cnt1 < cnt2;
  }
  return 1;
}
// 문자열 왼쪽 공백제거
char *trim_left(char *str)
{
  while (*str)
  {
    if (isspace(*str))
    {
      str++;
    }
    else
    {
      break;
    }
  }
  return str;
}

char *trim_right(char *str)
{
  int len = (int)strlen(str) - 1;

  while (len >= 0)
  {
    if (isspace(*(str + len)))
    {
      len--;
    }
    else
    {
      break;
    }
  }
  *(str + ++len) = '\0';
  return str;
}

char *findBackupFileByOriginPath(char *originPath)
{
  char *returnBackupPath = (char *)malloc(sizeof(char) * PATHMAX);
  int cnt;
  struct dirent **namelist;

  if ((cnt = scandir(backupPATH, &namelist, NULL, alphasort)) == -1)
  {
    fprintf(stderr, "ERROR: scandir error for %s\n", backupPATH);
    exit(1);
  }
  // 백업폴더에 로그가 하나이상 존재할경우 탐색
  if (cnt > 3)
  {

    backupList *currBackupLog = mainBackupList;

    while (true)
    {
      if (currBackupLog == NULL)
      {
        break;
      }
      if (!strcmp(currBackupLog->cur->log, originPath))
      {
        strcpy(returnBackupPath, currBackupLog->cur->backuplog);
        return currBackupLog->cur->backuplog;
      }
      currBackupLog = currBackupLog->next;
    }
  }
  return NULL;
}

// 백업경로를 인자로 주면 메타데이터를 뒤져서 원본경로 반환하는 함수
// 메타데이터는 원본경로@백업경로 형태
char *findMetaData(char *backupPath)
{
  struct stat metaStat;
  long long metasize;
  metaList *mainMetaList = NULL;
  ssize_t bytes_read;
  int prevPos = 0;
  int curPos = 0;
  int fd;
  char *returnPath = (char *)malloc(sizeof(char *) * PATHMAX);
  if ((fd = open(metaPATH, O_RDONLY, 777)) < 0)
  {
    fprintf(stderr, "ERROR: open error for %s\n", metaPATH);
    exit(1);
  }

  if (stat(metaPATH, &metaStat) == 0)
  {
    metasize = metaStat.st_size;
  }
  else
  {
    fprintf(stderr, " ERROR : file stat error %s\n", metaPATH);
    exit(1);
  }
  if (metasize > 0)
  {
    char metaBuffer[metasize];
    bytes_read = read(fd, metaBuffer, metasize - 1); // 버퍼 사이즈보다 작게 읽기
    if (bytes_read == -1)
    {
      fprintf(stderr, "ERROR : reading file %s", metaPATH);
      exit(1);
    }
    metaBuffer[bytes_read] = '\0';

    while (true)
    {

      if (metaBuffer[curPos] == '\0')
      {
        char str[PATHMAX * 2] = "";
        // 메타 데이터 한줄을 읽기
        int iidx = 0;
        for (int j = prevPos; j < curPos; j++)
        {
          str[iidx] = metaBuffer[j];
          iidx++;
        }
        trim_right(str);
        char *res = strtok(str, "@");
        char splitStrings[2][PATHMAX];
        char origin[PATHMAX] = "";
        char backup[PATHMAX] = "";
        int idx = 0;
        while (res != NULL)
        {
          strcpy(splitStrings[idx], res);
          idx++;
          res = strtok(NULL, " ");
        }
        strcpy(origin, splitStrings[0]);
        strcpy(backup, splitStrings[1]);

        metaNode = (logElement *)malloc(sizeof(logElement));
        strcpy(metaNode->log, origin);
        strcpy(metaNode->backuplog, backup);

        metaList *newMeta = (metaList *)malloc(sizeof(metaList));
        newMeta->cur = metaNode;
        newMeta->prev = NULL;
        newMeta->next = NULL;

        // 메타로그 리스트에 로그 노드 추가
        if (mainMetaList == NULL)
        {
          // mainlogList가 비어 있는 경우
          mainMetaList = newMeta;
        }
        else
        {
          metaList *curr = mainMetaList;
          while (curr->next != NULL)
          {
            curr = curr->next;
          }
          curr->next = newMeta;
          newMeta->prev = curr;
        }
        break;
      }
      else if (metaBuffer[curPos] == '\n')
      {

        char str[PATHMAX * 2] = "";
        // 메타 데이터 한줄을 읽기
        int iidx = 0;
        for (int j = prevPos; j < curPos; j++)
        {
          str[iidx] = metaBuffer[j];
          iidx++;
        }
        trim_right(str);
        char *res = strtok(str, "@");
        char splitStrings[2][PATHMAX];
        char origin[PATHMAX] = "";
        char backup[PATHMAX] = "";
        int idx = 0;
        while (res != NULL)
        {
          strcpy(splitStrings[idx], res);
          idx++;
          res = strtok(NULL, " ");
        }
        strcpy(origin, splitStrings[0]);
        strcpy(backup, splitStrings[1]);

        metaNode = (logElement *)malloc(sizeof(logElement));
        strcpy(metaNode->log, origin);
        strcpy(metaNode->backuplog, backup);

        metaList *newMeta = (metaList *)malloc(sizeof(metaList));
        newMeta->cur = metaNode;
        newMeta->prev = NULL;
        newMeta->next = NULL;

        // 메타로그 리스트에 로그 노드 추가
        if (mainMetaList == NULL)
        {
          // mainlogList가 비어 있는 경우
          mainMetaList = newMeta;
        }
        else
        {
          metaList *curr = mainMetaList;
          while (curr->next != NULL)
          {
            curr = curr->next;
          }
          curr->next = newMeta;
          newMeta->prev = curr;
        }
        prevPos = (curPos + 1);
        curPos++;
      }
      else
      {
        curPos++;
      }
    }
  }

  metaList *currMeta = mainMetaList;
  char originHash[PATHMAX] = "";
  ConvertHash(backupPath, originHash);
  while (true)
  {
    if (currMeta == NULL)
    {
      break;
    }
    char tmpHash[PATHMAX] = "";
    ConvertHash(currMeta->cur->backuplog, tmpHash);
    if (!strcmp(currMeta->cur->backuplog, backupPath) && !strcmp(tmpHash, originHash))
    {
      strcpy(returnPath, currMeta->cur->log);
      return returnPath;
    }
    currMeta = currMeta->next;
  }

  return NULL;
}

// 삭제되는 백업경로를 인자로 넘겨주면 메타데이터 내에도 해당 백업경로를 삭제하고 다시 쓰는 로직
// errno : -1 정상적 삭제 :1
int clearMetaData(char *backupPath)
{

  struct stat metaStat;
  long long metasize;
  metaList *mainMetaList = NULL;
  ssize_t bytes_read;
  int prevPos = 0;
  int curPos = 0;
  int fd;
  char deletePath[PATHMAX] = "";
  if ((fd = open(metaPATH, O_RDONLY, 777)) < 0)
  {
    fprintf(stderr, "ERROR: open error for %s\n", metaPATH);
    return -1;
  }

  if (stat(metaPATH, &metaStat) == 0)
  {
    metasize = metaStat.st_size;
  }
  else
  {
    fprintf(stderr, " ERROR : file stat error %s\n", metaPATH);
    return -1;
  }

  if (metasize > 0)
  {
    char metaBuffer[metasize];
    bytes_read = read(fd, metaBuffer, metasize - 1); // 버퍼 사이즈보다 작게 읽기
    if (bytes_read == -1)
    {
      fprintf(stderr, "ERROR : reading file %s", metaPATH);
      return -1;
    }
    metaBuffer[bytes_read] = '\0';

    while (true)
    {

      if (metaBuffer[curPos] == '\0')
      {
        char str[PATHMAX * 2] = "";
        // 메타 데이터 한줄을 읽기
        int iidx = 0;
        for (int j = prevPos; j < curPos; j++)
        {
          str[iidx] = metaBuffer[j];
          iidx++;
        }
        trim_right(str);
        char *res = strtok(str, "@");
        char splitStrings[2][PATHMAX];
        char origin[PATHMAX] = "";
        char backup[PATHMAX] = "";
        int idx = 0;
        while (res != NULL)
        {
          strcpy(splitStrings[idx], res);
          idx++;
          res = strtok(NULL, " ");
        }
        strcpy(origin, splitStrings[0]);
        strcpy(backup, splitStrings[1]);

        metaNode = (logElement *)malloc(sizeof(logElement));
        strcpy(metaNode->log, origin);
        strcpy(metaNode->backuplog, backup);

        metaList *newMeta = (metaList *)malloc(sizeof(metaList));
        newMeta->cur = metaNode;
        newMeta->prev = NULL;
        newMeta->next = NULL;

        // 메타로그 리스트에 로그 노드 추가
        if (mainMetaList == NULL)
        {
          // mainlogList가 비어 있는 경우
          mainMetaList = newMeta;
        }
        else
        {
          metaList *curr = mainMetaList;
          while (curr->next != NULL)
          {
            curr = curr->next;
          }
          curr->next = newMeta;
          newMeta->prev = curr;
        }
        break;
      }
      else if (metaBuffer[curPos] == '\n')
      {

        char str[PATHMAX * 2] = "";
        // 메타 데이터 한줄을 읽기
        int iidx = 0;
        for (int j = prevPos; j < curPos; j++)
        {
          str[iidx] = metaBuffer[j];
          iidx++;
        }
        trim_right(str);
        char *res = strtok(str, "@");
        char splitStrings[2][PATHMAX];
        char origin[PATHMAX] = "";
        char backup[PATHMAX] = "";
        int idx = 0;
        while (res != NULL)
        {
          strcpy(splitStrings[idx], res);
          idx++;
          res = strtok(NULL, " ");
        }
        strcpy(origin, splitStrings[0]);
        strcpy(backup, splitStrings[1]);

        metaNode = (logElement *)malloc(sizeof(logElement));
        strcpy(metaNode->log, origin);
        strcpy(metaNode->backuplog, backup);

        metaList *newMeta = (metaList *)malloc(sizeof(metaList));
        newMeta->cur = metaNode;
        newMeta->prev = NULL;
        newMeta->next = NULL;

        // 메타로그 리스트에 로그 노드 추가
        if (mainMetaList == NULL)
        {
          // mainlogList가 비어 있는 경우
          mainMetaList = newMeta;
        }
        else
        {
          metaList *curr = mainMetaList;
          while (curr->next != NULL)
          {
            curr = curr->next;
          }
          curr->next = newMeta;
          newMeta->prev = curr;
        }
        prevPos = (curPos + 1);
        curPos++;
      }
      else
      {
        curPos++;
      }
    }
  }
  // 메타데이터 파일 초기화
  // 1) 메타데이터 파일을 삭제
  // 2) 다시 만든다.
  // 매개변수로 받아온 경로와 같은 경로이면 메타데이터에 쓰지 않는다.

  if (remove(metaPATH) < 0)
  {
    fprintf(stderr, "ERROR : remove error %s\n", metaPATH);
    return -1;
  }
  if (access(metaPATH, F_OK))
  {
    fd = 0;
    if ((fd = creat(metaPATH, 0666)) < 0)
    {
      fprintf(stderr, "error for creating meta data file\n");
      return -1;
    }
  }
  metaList *currMeta = mainMetaList;
  while (true)
  {
    if (currMeta == NULL)
    {
      break;
    }
    char tmpHash[PATHMAX] = "";
    char metaData[PATHMAX * 3] = "";
    // 매개변수로 받아온 backupPath와 다른경우만 써준다.
    if (strcmp(currMeta->cur->backuplog, backupPath))
    {
      strcat(metaData, currMeta->cur->log);
      strcat(metaData, "@");
      strcat(metaData, currMeta->cur->backuplog);
      strcat(metaData, "\n");
      if (write(fd, metaData, strlen(metaData)) == -1)
      {
        fprintf(stderr, "ERROR: write error for %s\n", metaPATH);
        return 1;
      }
    }
    currMeta = currMeta->next;
  }

  return 1;
}

// 링크드 리스트로 백업 상태관리 (초기 빌드시, 로그 삭제시, recover시)
int refreshBackupLinkedList()
{
  struct stat statbuf;
  int cnt, fd, subCnt, fd2;
  struct dirent **namelist, **subNameList;

  if (lstat(backupPATH, &statbuf) < 0)
  {
    fprintf(stderr, "ERROR: lstat error for %s\n", backupPATH);
    return -1;
  }
  if ((cnt = scandir(backupPATH, &namelist, NULL, alphasort)) == -1)
  {
    fprintf(stderr, "ERROR: scandir error for %s\n", backupPATH);
    return -1;
  }
  // 백업폴더에 로그가 하나 이상일 경우 링크드 리스트로 상태관리
  if (cnt > 3)
  {

    queue Queue;
    initQueue(&Queue);
    for (int i = 0; i < cnt - 1; i++)
    {
      if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..") || !strcmp(namelist[i]->d_name, "ssubak.log"))
        continue;
      struct stat buf;
      char tmppath[PATHMAX] = "";
      strcat(tmppath, backupPATH);
      strcat(tmppath, "/");
      strcat(tmppath, namelist[i]->d_name);

      // /home/ubuntu/backup/24032012/ 까지 추적 -> 하위 파일 추적하러 가기
      if (lstat(tmppath, &buf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", tmppath);
        return -1;
      }

      enqueue(&Queue, tmppath);
    }

    while (true)
    {
      if (Queue.count == 0)
      {
        break;
      }
      char *nodePath = dequeue(&Queue);
      if ((subCnt = scandir(nodePath, &subNameList, NULL, alphasort)) == -1)
      {
        fprintf(stderr, "ERROR: scandir error for %s\n", nodePath);
        return -1;
      }

      struct stat statbuf;
      if (lstat(nodePath, &statbuf) < 0)
      {
        fprintf(stderr, "ERROR: lstat error for %s\n", nodePath);
        return -1;
      }

      for (int i = 0; i < subCnt; i++)
      {
        if (!strcmp(subNameList[i]->d_name, ".") || !strcmp(subNameList[i]->d_name, "..") || !strcmp(subNameList[i]->d_name, "ssubak.log"))
          continue;
        struct stat subbuf;
        char tmppath[PATHMAX] = "";
        strcat(tmppath, nodePath);
        strcat(tmppath, "/");
        strcat(tmppath, subNameList[i]->d_name);
        // /home/ubuntu/backup/24032012/ 까지 추적 -> 하위 파일 추적하러 가기
        if (lstat(tmppath, &subbuf) < 0)
        {
          fprintf(stderr, "ERROR: lstat error for %s\n", tmppath);
          return -1;
        }
        if (S_ISDIR(subbuf.st_mode))
        {
          enqueue(&Queue, tmppath);
        }
        else
        {
          // 노드 생성
          backupList *newBackupNode = (backupList *)malloc(sizeof(backupList));
          // 노드 안의 로그 생성
          logElement *backupLogElement = (logElement *)malloc(sizeof(logElement));
          memset(backupLogElement, 0, sizeof(backupLogElement));
          strcpy(backupLogElement->backuplog, tmppath);

          char *originPath = findMetaData(tmppath);
          if (originPath != NULL)
          {
            strcpy(backupLogElement->log, originPath);
          }
          newBackupNode->cur = backupLogElement;
          newBackupNode->prev = NULL;
          newBackupNode->next = NULL;
          // 로그 리스트에 로그 노드 추가
          backupList *curr;
          if (mainBackupList == NULL)
          {
            // mainlogList가 비어 있는 경우
            mainBackupList = newBackupNode;
          }
          else
          {
            curr = mainBackupList;
            while (curr->next != NULL)
            {
              curr = curr->next;
            }
            curr->next = newBackupNode;
            newBackupNode->prev = curr;
          }
        }
      }
    }
    backupList *currBackupLog = mainBackupList;
    while (true)
    {
      if (currBackupLog->next == NULL)
      {
        break;
      }
      currBackupLog = currBackupLog->next;
    }
  }

  return 0;
}

// 백업경로에서 백업 시간만 잘라내기
char *splitBackupDate(char *path)
{

  char backupLog[PATHMAX];
  char backupTimeTemp[PATHMAX];
  char *returnDate = (char *)malloc(sizeof(char *) * PATHMAX);

  strcpy(backupLog, path);
  strcpy(backupTimeTemp, path);
  char *backuptime = strstr(backupTimeTemp, backupPATH);
  if (backuptime != NULL)
  {
    strcpy(backuptime, backuptime + 1 + strlen(backupPATH));
  }
  // 백업시간만 잘라내기
  char *realBackupTime = strtok(backuptime, "/");
  strcpy(returnDate, realBackupTime);
  return returnDate;
}

char *addCommas(int num)
{
  char str[STRMAX];
  sprintf(str, "%d", num);
  int len = strlen(str);
  char *result = (char *)malloc((len / 3) * 2 + len + 1);
  if (result == NULL)
  {
    fprintf(stderr, "ERROR: Memory allocation error \n");
    exit(EXIT_FAILURE);
  }
  result[0] = '\0';

  int commaCount = 0;
  for (int i = len - 1; i >= 0; i--)
  {
    if (commaCount == 3)
    {
      strcat(result, ",");
      commaCount = 0;
    }
    char temp[2] = {str[i], '\0'};
    strcat(result, temp);
    commaCount++;
  }

  // 문자열 뒤집기
  int resultLen = strlen(result);
  for (int i = 0; i < resultLen / 2; i++)
  {
    char temp = result[i];
    result[i] = result[resultLen - i - 1];
    result[resultLen - i - 1] = temp;
  }
  return result;
}

char *concatPath(char *path1, char *path2)
{
  size_t totalLength = strlen(path1) + strlen(path2) + 2;
  char *result = malloc(totalLength);
  if (result == NULL)
  {
    fprintf(stderr, "Memory allocation failed\n");
    return NULL; // 메모리 할당 실패 처리
  }

  // 첫 번째 경로 복사
  strcpy(result, path1);

  // 경로 사이에 슬래시 추가 (이미 슬래시로 끝나는 경우를 처리하지 않음)
  strcat(result, "/");

  // 두 번째 경로 추가
  strcat(result, path2);

  return result;
}

// 마지막에 파일명만 자르는 함수

char *extractPath(const char *fullPath)
{
  // fullPath에서 마지막 '/'를 찾는다
  const char *lastSlash = strrchr(fullPath, '/');
  if (lastSlash == NULL)
  {
    // '/'가 없는 경우, 전체 경로를 그대로 반환
    return strdup(fullPath);
  }
  else
  {
    // 마지막 '/'까지의 길이를 계산
    int newPathLength = lastSlash - fullPath;

    char *newPath = (char *)malloc(newPathLength + 1);
    if (newPath == NULL)
    {
      fprintf(stderr, "ERROR: Memory allocation failed\n");
      exit(EXIT_FAILURE);
    }

    strncpy(newPath, fullPath, newPathLength);

    newPath[newPathLength] = '\0';
    return newPath;
  }
}

void removeLogAndUpdateList(logList **head, char *logToRemove)
{
  logList *temp = *head, *prev = NULL;

  while (1)
  {
    if (temp == NULL)
    {
      break;
    }
    if (temp->cur->log == NULL || temp->cur == NULL)
    {
      break;
    }

    if (!strcmp(temp->cur->log, logToRemove))
    {
      // 로그가 일치하는 경우
      if (prev == NULL)
      {
        *head = temp->next; // 헤드를 다음 노드로 업데이트
      }
      else
      {
        prev->next = temp->next; // 이전 노드의 다음을 현재 노드의 다음으로 설정
      }

      if (temp->next != NULL)
      {
        temp->next->prev = prev;
      }

      // 메모리 해제 전에 다음 노드로 이동
      logList *nextTemp = temp->next;
      free(temp->cur);
      free(temp);

      // 다음 노드로 이동 후 현재 노드 메모리 해제
      temp = nextTemp;
    }
    else
    {
      prev = temp;       // 현재 노드를 이전으로 설정
      temp = temp->next; // 다음 노드로 이동
    }
  }
}

void removeBackuplog(logList **head, char *logToRemove)
{
  logList *temp = *head, *prev = NULL;

  while (temp != NULL)
  {

    if (temp->cur != NULL && !strcmp(temp->cur->backuplog, logToRemove))
    {
      // 로그가 일치하는 경우
      if (prev == NULL)
      {
        *head = temp->next; // 헤드를 다음 노드로 업데이트
      }
      else
      {
        prev->next = temp->next; // 이전 노드의 다음을 현재 노드의 다음으로 설정
      }

      if (temp->next != NULL)
      {
        temp->next->prev = prev;
      }

      // 메모리 해제 전에 다음 노드로 이동
      logList *nextTemp = temp->next;
      free(temp->cur);
      free(temp);

      // 다음 노드로 이동 후 현재 노드 메모리 해제
      temp = nextTemp;
      return; // 하나만 삭제 후 함수 종료
    }
    else
    {
      prev = temp;       // 현재 노드를 이전으로 설정
      temp = temp->next; // 다음 노드로 이동
    }
  }
}
