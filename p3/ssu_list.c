#include "ssu_header.h"

dirNode *backup_dir_list = NULL;
dirNode *monitor_dir_list = NULL;
char *exeNAME = NULL;
char *exePATH = NULL;
char *pwdPATH = NULL;
char *backupPATH = NULL;
char *monitorLogPATH = NULL;
dirNode *print_create_dir_list = NULL;
dirNode *log_dir_list = NULL;
int logCnt = 0;
int hash = 0;
int inputPathDepth = 0;

int count_depth(const char *str)
{
    int count = 0;
    while (*str)
    {
        if (*str == '/')
        {
            count++;
        }
        str++;
    }
    return count;
}

char *find_path_by_pid(dirNode *dirNode, char *pid_string)
{

    if (dirNode == NULL)
        return NULL;

    fileNode *fNode = dirNode->file_head;
    while (fNode != NULL)
    {
        backupNode *bNode = fNode->backup_head;
        while (bNode != NULL)
        {
            if (strlen(bNode->origin_path) > 0)
            {

                if (!strcmp(bNode->pid, pid_string))
                {

                    return bNode->origin_path;
                }
            }
            bNode = bNode->next_backup;
        }
        fNode = fNode->next_file;
    }
    dirNode = dirNode->subdir_head;
    while (dirNode != NULL)
    {
        char *result = find_path_by_pid(dirNode, pid_string);
        if (result != NULL)
        {
            return result;
        }

        dirNode = dirNode->next_dir;
    }
    return NULL;
}

void init_tree(char *pid)
{
    int list_fd;
    char buf[PATH_MAX];
    int len;
    char logfilePath[PATH_MAX];
    sprintf(logfilePath, "%s/%s.log", backupPATH, pid);
    if ((list_fd = open(logfilePath, O_RDONLY)) < 0)
    {
        fprintf(stderr, "ERROR4: open error\n");
    }
    while (len = read(list_fd, buf, BUF_MAX))
    {

        char *ptr = strchr(buf, '\n');
        ptr[0] = '\0';

        lseek(list_fd, -(len - strlen(buf)) + 1, SEEK_CUR);
        char *before;
        char *after;

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

            backup_list_insert(print_create_dir_list, before, after, "", pid);
        }
    }
    close(list_fd);
}

void init_log_list(char *path, char *logfilePath)
{
    int list_fd;
    char buf[PATH_MAX];
    int len;

    if ((list_fd = open(logfilePath, O_RDONLY)) < 0)
    {
        fprintf(stderr, "ERROR1: open error\n");
    }
    while (len = read(list_fd, buf, BUF_MAX))
    {

        char *ptr = strchr(buf, '\n');
        ptr[0] = '\0';

        lseek(list_fd, -(len - strlen(buf)) + 1, SEEK_CUR);
        char *before;
        char *after;

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

            if (!strcmp(path, after))
            {
                backup_list_insert(log_dir_list, before, after, "", "create");
            }
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

            if (!strcmp(path, after))
            {
                backup_list_insert(log_dir_list, before, after, "", "modify");
            }
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

            if (!strcmp(path, after))
            {
                backup_list_insert(log_dir_list, before, after, "", "remove");
            }
        }
    }
    close(list_fd);
}

void count_logs(dirNode *dirNode)
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
                logCnt++;
            }
            bNode = bNode->next_backup;
        }
        fNode = fNode->next_file;
    }
    dirNode = dirNode->subdir_head;
    while (dirNode != NULL)
    {
        count_logs(dirNode);
        dirNode = dirNode->next_dir;
    }
}
int curCnt = 0;
void print_logs(dirNode *dirNode, int depthDiff)
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
                curCnt++;
                if (curCnt < logCnt)
                {
                    if (depthDiff == 0)
                    {
                        printf("┣ [ %s ] [ %s ]\n", bNode->pid, bNode->time);
                    }
                    else if (depthDiff == 1)
                    {
                        printf("┃ ┣ [ %s ] [ %s ]\n", bNode->pid, bNode->time);
                    }
                }
                else if (curCnt == logCnt)
                {
                    if (depthDiff == 0)
                    {
                        printf("┗ [ %s ] [ %s ]\n", bNode->pid, bNode->time);
                    }
                    else if (depthDiff == 1)
                    {
                        printf("┃ ┗ [ %s ] [ %s ]\n", bNode->pid, bNode->time);
                    }
                }
            }
            bNode = bNode->next_backup;
        }
        fNode = fNode->next_file;
    }
    dirNode = dirNode->subdir_head;
    while (dirNode != NULL)
    {
        print_logs(dirNode, depthDiff); // 재귀 호출
        dirNode = dirNode->next_dir;
    }
}

void printTreeByDir(dirNode *dirNode, int inputPathDepth)
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
                int pathDepth = count_depth(bNode->origin_path);
                char prefixDate[PATH_MAX];
                // 이때는 파일임 /home/ubuntu/a.txt
                if (pathDepth - inputPathDepth == 1)
                {

                    struct tm tm;
                    time_t t;
                    if (strptime(bNode->time, "%Y-%m-%d %H:%M:%S", &tm) == NULL)
                    {
                        fprintf(stderr, "ERROR: strptime failed to parse time string: %s\n", bNode->time);
                        return;
                    }

                    // struct tm을 time_t로 변환
                    t = mktime(&tm);
                    if (t == -1)
                    {
                        fprintf(stderr, "ERROR: mktime failed\n");
                        return;
                    }

                    // struct tm을 localtime으로 변환
                    struct tm *local_tm = localtime(&t);
                    if (local_tm == NULL)
                    {
                        fprintf(stderr, "ERROR: localtime failed\n");
                        return;
                    }

                    // 원하는 출력 형식으로 변환
                    if (strftime(prefixDate, sizeof(prefixDate), "%Y%m%d%H%M%S", local_tm) == 0)
                    {
                        fprintf(stderr, "ERROR: strftime failed\n");
                        return;
                    }

                    char originpath[PATH_MAX];
                    strcpy(originpath, bNode->origin_path);
                    char pathbuf[PATH_MAX];

                    const char *last_slash = strrchr(originpath, '/');

                    if (last_slash != NULL)
                    {
                        // 슬래시 이후의 문자열을 복사한다.
                        strcpy(pathbuf, last_slash + 1);
                    }

                    char filePath[PATH_MAX];
                    // prefixDate제거해야함
                    sprintf(filePath, "%s", pathbuf);
                    printf("┣ %s\n", filePath);
                    char logfilePath[PATH_MAX];
                    sprintf(logfilePath, "%s/%s.log", backupPATH, bNode->pid);
                    dirNode_init(log_dir_list);
                    init_log_list(bNode->origin_path, logfilePath);
                    count_logs(log_dir_list);
                    print_logs(log_dir_list, 1);
                }
            }
            bNode = bNode->next_backup;
        }
        fNode = fNode->next_file;
    }
    dirNode = dirNode->subdir_head;
    while (dirNode != NULL)
    {
        printTreeByDir(dirNode, inputPathDepth); // 재귀 호출
        dirNode = dirNode->next_dir;
    }
}
void printTreeByReg(dirNode *dirNode, int inputPathDepth)
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
                int pathDepth = count_depth(bNode->origin_path);
                char prefixDate[PATH_MAX];

                // 이때는 파일임 /home/ubuntu/a.txt

                struct tm tm;
                time_t t;
                if (strptime(bNode->time, "%Y-%m-%d %H:%M:%S", &tm) == NULL)
                {
                    fprintf(stderr, "ERROR: strptime failed to parse time string: %s\n", bNode->time);
                    return;
                }

                // struct tm을 time_t로 변환
                t = mktime(&tm);
                if (t == -1)
                {
                    fprintf(stderr, "ERROR: mktime failed\n");
                    return;
                }

                // struct tm을 localtime으로 변환
                struct tm *local_tm = localtime(&t);
                if (local_tm == NULL)
                {
                    fprintf(stderr, "ERROR: localtime failed\n");
                    return;
                }

                // 원하는 출력 형식으로 변환
                if (strftime(prefixDate, sizeof(prefixDate), "%Y%m%d%H%M%S", local_tm) == 0)
                {
                    fprintf(stderr, "ERROR: strftime failed\n");
                    return;
                }

                char originpath[PATH_MAX];
                strcpy(originpath, bNode->origin_path);
                char pathbuf[PATH_MAX];

                const char *last_slash = strrchr(originpath, '/');

                if (last_slash != NULL)
                {
                    // 슬래시 이후의 문자열을 복사한다.
                    strcpy(pathbuf, last_slash + 1);
                }

                char filePath[PATH_MAX];
                // prefixDate제거해야
                sprintf(filePath, "%s", pathbuf);
                printf("%s\n", filePath);
                char logfilePath[PATH_MAX];
                sprintf(logfilePath, "%s/%s.log", backupPATH, bNode->pid);
                dirNode_init(log_dir_list);
                init_log_list(bNode->origin_path, logfilePath);
                count_logs(log_dir_list);
                print_logs(log_dir_list, 0);
            }
            bNode = bNode->next_backup;
        }
        fNode = fNode->next_file;
    }
    dirNode = dirNode->subdir_head;
    while (dirNode != NULL)
    {
        printTreeByReg(dirNode, inputPathDepth); // 재귀 호출
        dirNode = dirNode->next_dir;
    }
}
int isValid = -1;
int find_pid_valid(dirNode *dirNode, char *pid_string)
{

    if (dirNode == NULL)
        return -1;

    fileNode *fNode = dirNode->file_head;
    while (fNode != NULL)
    {
        backupNode *bNode = fNode->backup_head;
        while (bNode != NULL)
        {
            if (strlen(bNode->origin_path) > 0)
            {

                if (!strcmp(bNode->pid, pid_string))
                {

                    isValid = 1;
                    return 1;
                }
            }
            bNode = bNode->next_backup;
        }
        fNode = fNode->next_file;
    }
    dirNode = dirNode->subdir_head;
    while (dirNode != NULL)
    {
        int res = find_pid_valid(dirNode, pid_string);

        dirNode = dirNode->next_dir;
    }
    if (isValid == -1)
    {

        return isValid;
    }
}
void ssu_list_detail(char *pid)
{
    int cnt;
    struct stat statbuf;
    struct dirent **namelist;
    int list_fd;
    int len;
    char buf[PATH_MAX];
    if ((list_fd = open(monitorLogPATH, O_RDONLY)) < 0)
    {
        fprintf(stderr, "ERROR2: open error\n");
    }
    while (len = read(list_fd, buf, BUF_MAX))
    {

        char *ptr = strchr(buf, '\n');
        ptr[0] = '\0';

        lseek(list_fd, -(len - strlen(buf)) + 1, SEEK_CUR);

        char *pid_string;
        char *path_string;

        // 첫 번째 " : "을 기준으로 문자열을 분리
        pid_string = strtok(buf, " : ");

        // 다음 " : "을 기준으로 나머지 문자열을 분리
        path_string = strtok(NULL, " : ");

        backup_list_insert(monitor_dir_list, "", path_string, "", pid_string);
    }
    int findValidPid = find_pid_valid(monitor_dir_list, pid);
    if (findValidPid == -1)
    {
        fprintf(stderr, "ERROR: invalid pid\n");
        exit(1);
    }

    char *path = find_path_by_pid(monitor_dir_list, pid);

    int inputPathDepth = count_depth(path);
    init_tree(pid);
    if (stat(path, &statbuf) < 0)
    {
    }
    if (S_ISREG(statbuf.st_mode))
    {

        printTreeByReg(print_create_dir_list, inputPathDepth);
    }
    else
    {
        // 지우지마! 경로 printf한거임!!
        printf("%s\n", path);
        printTreeByDir(print_create_dir_list, inputPathDepth);
    }
}
void ssu_list()
{

    int list_fd;
    int len;
    char buf[PATH_MAX];
    if ((list_fd = open(monitorLogPATH, O_RDONLY)) < 0)
    {
        fprintf(stderr, "ERROR3: open error\n");
    }
    while (len = read(list_fd, buf, BUF_MAX))
    {

        char *ptr = strchr(buf, '\n');
        ptr[0] = '\0';

        lseek(list_fd, -(len - strlen(buf)) + 1, SEEK_CUR);
        printf("%s\n", buf);

        char *pid_string;
        char *path_string;

        // 첫 번째 " : "을 기준으로 문자열을 분리합니다.
        pid_string = strtok(buf, " : ");

        // 다음 " : "을 기준으로 나머지 문자열을 분리합니다.
        path_string = strtok(NULL, " : ");

        backup_list_insert(monitor_dir_list, "", path_string, "", pid_string);
    }
}

int main(int argc, char *argv[])
{
    exePATH = (char *)malloc(PATH_MAX);
    exeNAME = (char *)malloc(PATH_MAX);
    pwdPATH = (char *)malloc(PATH_MAX);
    backupPATH = (char *)malloc(PATH_MAX);
    monitorLogPATH = (char *)malloc(PATH_MAX);
    monitor_dir_list = (dirNode *)malloc(sizeof(dirNode));
    dirNode_init(monitor_dir_list);
    print_create_dir_list = (dirNode *)malloc(sizeof(dirNode));
    dirNode_init(print_create_dir_list);
    log_dir_list = (dirNode *)malloc(sizeof(dirNode));
    dirNode_init(log_dir_list);

    if (init(argv[0]) == -1)
    {
        fprintf(stderr, "ERROR: init error.\n");
        return -1;
    }

    if (argc == 2)
    {

        ssu_list_detail(argv[1]);
    }
    else if (argc == 1)
    {

        ssu_list();
    }

    return 0;
}