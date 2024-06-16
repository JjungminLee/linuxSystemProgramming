#include "ssu_header.h"

dirNode *backup_dir_list = NULL;
dirNode *monitor_dir_list = NULL;
dirNode *print_create_dir_list = NULL;
dirNode *log_dir_list = NULL;
char *exeNAME = NULL;
char *exePATH = NULL;
char *pwdPATH = NULL;
char *backupPATH = NULL;
char *monitorLogPATH = NULL;

int hash = 0;

volatile sig_atomic_t signalRecv = 0;

// 시그널 핸들러 함수는 하나의 매개변수를 가져야 하며,
// 그 매개변수는 받은 시그널의 번호 따라서, 시그널 핸들러는 자동적으로 시그널 번호를 매개변수로 받게 됨
void signalHandler(int signum)
{
    char signalerrorlog[PATH_MAX];
    sprintf(signalerrorlog, "Received signal %d (%s)\n", signum, strsignal(signum));
    log_error(signalerrorlog);
    if (signum == SIGUSR1)
    {
        log_error("Received SIGUSR1 signal\n");
        signalRecv = 1;
    }
}

int createDaemon(void)
{
    pid_t pid;
    int fd, maxfd;

    // 자식 프로세스 생성
    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "ERROR: fork error\n");
        log_error("ERROR: fork error\n");
        exit(1);
    }
    // 부모 종료
    else if (pid != 0)
        exit(0);

    pid = getpid();
    setsid();

    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGUSR1, signalHandler);

    maxfd = getdtablesize();
    for (fd = 0; fd < maxfd; fd++)
        close(fd);

    umask(0);
    // chdir("/");

    fd = open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    return getpid();
}
// 백업리스트안에 backupFilePath가 있는지 확인
// 없으면 생성
// 있는데 수정시간 다르면 수정
// 없으면 -1 반환
int exist = -1;
int processFilesIndirectory(char *path, char *backupDirPath, dirNode *dirNode, char *daemonLogPath, int pid)
{
    char pid_string[PATH_MAX];
    sprintf(pid_string, "%d", pid);
    char daemonbuf[PATH_MAX];
    char backupFilePath[PATH_MAX];
    struct stat statbuf;

    int backup_fd;
    int origin_fd;
    int daemonLog_fd;
    int len;
    char buf[PATH_MAX];
    char formattedTime[PATH_MAX];

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

            if (strlen(bNode->origin_path) > 0 && !strcmp(pid_string, bNode->pid))
            {

                if ((daemonLog_fd = open(daemonLogPath, O_WRONLY | O_CREAT | O_APPEND, 0777)) < 0)
                {
                    fprintf(stderr, "ERROR: daemon log open error\n");

                    log_error("ERROR: daemon log open error1");
                }

                if (!strcmp(bNode->origin_path, path))
                {

                    exist = 1;

                    if (stat(path, &statbuf) < 0)
                    {
                        fprintf(stderr, "ERROR: stat error\n");
                        log_error("stat error");
                    }
                    struct tm *mtime = localtime(&statbuf.st_mtime);
                    time_t fileModTime = mktime(mtime);

                    struct tm lastBackupTm = {0};
                    lastBackupTm.tm_isdst = -1;

                    strptime(bNode->time, "%Y-%m-%d %H:%M:%S", &lastBackupTm); // 날짜 문자열을 struct tm으로 변환
                    time_t lastBackupTime = mktime(&lastBackupTm);             // struct tm을 time_t로 변환

                    // 원본 파일 수정시간이 백업시간보다
                    if (fileModTime > lastBackupTime)
                    {
                        time_t now = time(NULL);
                        struct tm *tm_now = localtime(&now);
                        strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", tm_now);
                        sprintf(daemonbuf, "[%s] [modify] [%s] \n", formattedTime, path);
                        if (write(daemonLog_fd, daemonbuf, strlen(daemonbuf)) < 0)
                        {
                            fprintf(stderr, "ERROR: write log error\n");
                            log_error("ERROR: write log error\n");
                            exit(1);
                        }

                        // 백업로직
                        // @todo: 백업파일 생성
                        // 실행경로 제거
                        char *removeExePath = remove_base_path(path, exePATH);
                        if (removeExePath == NULL || strlen(removeExePath) == 0)
                        {
                            log_error("Error: Path after removing base path is invalid.\n");
                        }

                        char removeExePathBuf[PATH_MAX];
                        strcpy(removeExePathBuf, removeExePath);

                        char *filename = strrchr(removeExePath, '/');
                        if (filename != NULL)
                        {
                            filename++; // '/' 다음 문자로 포인터 이동
                        }
                        else
                        {
                            // '/'가 없는 경우, 전체 문자열이 파일 이름임을 가정
                            filename = removeExePath;
                        }

                        char formattedbuf[PATH_MAX];
                        now = time(NULL);
                        tm_now = localtime(&now);

                        memset(formattedbuf, 0, strlen(formattedbuf));
                        strftime(formattedbuf, sizeof(formattedbuf), "%Y%m%d%H%M%S", tm_now);
                        memset(backupFilePath, 0, PATH_MAX);
                        sprintf(backupFilePath, "%s/%s_%s", backupDirPath, filename, formattedbuf);
                        if ((backup_fd = open(backupFilePath, O_CREAT | O_WRONLY | O_TRUNC)) < 0)
                        {
                            fprintf(stderr, "ERROR: open error\n");
                            log_error("ERROR: open error\n");
                            exit(1);
                        }
                        if ((origin_fd = open(path, O_RDONLY)) < 0)
                        {
                            fprintf(stderr, "ERROR: open error\n");
                            log_error("ERROR: open error\n");
                            exit(1);
                        }
                        while (1)
                        {
                            len = read(origin_fd, buf, strlen(buf));
                            if (len <= 0)
                            {
                                break;
                            }
                            write(backup_fd, buf, len);
                        }
                        close(origin_fd);
                        close(backup_fd);
                        init_backup_list();

                        return 1; // 파일이 수정됨
                    }
                    // mtime이 같은 경우 hash랑 사이즈 값 비교하기
                    else if (lastBackupTime == fileModTime)
                    {
                        char lastBackHash[PATH_MAX] = "";
                        char fileModHash[PATH_MAX] = "";
                        cvtHash(bNode->origin_path, lastBackHash);
                        cvtHash(path, fileModHash);
                        struct stat laststatbuf;
                        struct stat filestatbuf;
                        if (stat(bNode->origin_path, &laststatbuf) < 0)
                        {
                            log_error("stat error");
                        }
                        if (stat(path, &filestatbuf) < 0)
                        {
                            log_error("stat error");
                        }
                        // 둘의 해시값 다르면 수정된거
                        if (strcmp(lastBackHash, fileModHash) && (laststatbuf.st_size != filestatbuf.st_size))
                        {
                            return 1;
                        }
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
        init_backup_list();
        processFilesIndirectory(path, backupDirPath, dirNode, daemonLogPath, pid); // 재귀 호출
        dirNode = dirNode->next_dir;
    }
    if (exist == -1)
    {

        return -1;
    }
}

// 매개변수로 받은 pid에 해당하는 백업파일이 원본 경로에 존재하지 않으면 삭제임 (-1리턴)
// 존재하면 1리턴

void findBackupFileByPidOptionD(dirNode *dirNode, int pid, char *inputPath, char *daemonLogPath)
{
    char pid_string[PATH_MAX];
    sprintf(pid_string, "%d", pid);
    int cnt;
    struct stat statbuf;
    struct dirent **namelist;
    int isExist = -1;
    char formattedTime[PATH_MAX];
    char daemonbuf[PATH_MAX];
    int daemonLog_fd;

    if (dirNode == NULL)
        return;

    fileNode *fNode = dirNode->file_head;
    while (fNode != NULL)
    {
        backupNode *bNode = fNode->backup_head;
        while (bNode != NULL)
        {

            if ((strlen(bNode->origin_path) > 0) && (!strcmp(pid_string, bNode->pid)))
            {

                // 백업리스트의 경로가 매개변수의 하위 경로라면
                if (!strncmp(bNode->origin_path, inputPath, strlen(inputPath)))
                {
                    if (access(bNode->origin_path, F_OK) != 0)
                    {
                        if ((daemonLog_fd = open(daemonLogPath, O_WRONLY | O_CREAT | O_APPEND, 0777)) < 0)
                        {
                            fprintf(stderr, "ERROR: daemon log open error\n");
                            log_error("ERROR: daemon log open error1");
                        }
                        time_t now = time(NULL);
                        struct tm *tm_now = localtime(&now);
                        strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", tm_now);
                        sprintf(daemonbuf, "[%s] [remove] [%s] \n", formattedTime, bNode->origin_path);
                        if (write(daemonLog_fd, daemonbuf, strlen(daemonbuf)) < 0)
                        {
                            fprintf(stderr, "ERROR: write log error\n");
                            log_error("ERROR: write log error\n");
                            exit(1);
                        }
                        close(daemonLog_fd);
                        init_backup_list();
                        return;
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
        findBackupFileByPidOptionD(dirNode, pid, inputPath, daemonLogPath);

        dirNode = dirNode->next_dir;
    }
}

// monitorPathByOptionD(tmp_path, daemon_pid, daemonLogPath, namelist[i]->d_name, backupDirPath);
void monitorPathByOptionD(char *path, int pid, char *daemonLogPath, char *backupDirPath)
{

    struct stat statbuf;

    char formattedTime[20];
    char backupFilePath[PATH_MAX];
    char buf[BUF_MAX];
    int len;
    char pid_string[BUF_MAX];
    int cnt;
    struct dirent **namelist;
    char tmp_path[PATH_MAX];
    char daemonbuf[PATH_MAX];
    int sub_cnt;
    int backup_fd;
    int origin_fd;
    int daemonLog_fd;
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    memset(formattedTime, 0, strlen(formattedTime));
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", tm_now);

    // outer : 백업리스트를 돌면서 매개변수로 받아온 경로 하위 파일인지 판단 (strcmp)
    // inner : 매개변수로 받아온 하위 경로가 맞는데 현재 원본경로에서는 사라지면 remove

    if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1)
    {

        fprintf(stderr, "ERROR: scandir error for %s\n", path);
        log_error("ERROR: scandir error\n");
    }
    for (int i = 0; i < cnt; i++)
    {
        char tmp_path[PATH_MAX];
        memset(tmp_path, 0, PATH_MAX);
        int sub_fd;
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
            continue;
        sprintf(tmp_path, "%s/%s", path, namelist[i]->d_name);

        if (lstat(tmp_path, &statbuf) < 0)
        {
            fprintf(stderr, "ERROR: lstat error\n");
            log_error("ERROR:  lstat error\n");
        }

        if (S_ISREG(statbuf.st_mode))
        {
            now = time(NULL);
            tm_now = localtime(&now);
            strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", tm_now);

            exist = -1;
            // @todo: 생성,수정,삭제 판단
            int result = processFilesIndirectory(tmp_path, backupDirPath, backup_dir_list, daemonLogPath, pid);

            // 생성임
            if (result == -1)
            {

                if ((daemonLog_fd = open(daemonLogPath, O_WRONLY | O_CREAT | O_APPEND, 0777)) < 0)
                {
                    fprintf(stderr, "ERROR: daemon log open error\n");
                    log_error("ERROR: daemon log open error1");
                }
                sprintf(daemonbuf, "[%s] [%s] [%s] \n", formattedTime, "create", tmp_path);
                if (write(daemonLog_fd, daemonbuf, strlen(daemonbuf)) == -1)
                {
                    fprintf(stderr, "ERROR: daemon log write error\n");
                    log_error("ERROR: daemon log write error\n");
                }
                close(daemonLog_fd);

                // @todo: 백업파일 생성
                // 실행경로 제거
                char *removeExePath = remove_base_path(tmp_path, exePATH);
                char removeExePathBuf[PATH_MAX];
                strcpy(removeExePathBuf, removeExePath);
                // 맨마지막 파일 경로 생성 /a/b/c.txt에서 c.txt만 추출
                char *filename = strrchr(removeExePath, '/');
                if (filename != NULL)
                {
                    // 파일이름만
                    filename++;
                }

                now = time(NULL);
                tm_now = localtime(&now);
                memset(formattedTime, 0, strlen(formattedTime));
                strftime(formattedTime, sizeof(formattedTime), "%Y%m%d%H%M%S", tm_now);
                memset(backupFilePath, 0, PATH_MAX);
                sprintf(backupFilePath, "%s/%s_%s", backupDirPath, filename, formattedTime);
                if ((backup_fd = open(backupFilePath, O_CREAT | O_WRONLY | O_TRUNC)) < 0)
                {
                    fprintf(stderr, "ERROR: open error\n");
                    log_error("ERROR: open error\n");
                }
                if ((origin_fd = open(path, O_RDONLY)) < 0)
                {
                    fprintf(stderr, "ERROR: open error\n");
                    log_error("ERROR: open error\n");
                }
                while (1)
                {
                    len = read(origin_fd, buf, strlen(buf));
                    if (len <= 0)
                    {
                        break;
                    }
                    write(backup_fd, buf, len);
                }
                close(origin_fd);
                close(backup_fd);
                init_backup_list();
            }
        }
    }
    close(daemonLog_fd);

    // 삭제 판단

    findBackupFileByPidOptionD(backup_dir_list, pid, path, daemonLogPath);
}
int monitorPath(char *path, int pid, char *daemonLogPath, char *backupDirPath)
{
    struct stat statbuf;

    char logBuffer[PATH_MAX + 100];
    char formattedTime[20];
    char backupFilePath[PATH_MAX];
    char buf[BUF_MAX];
    int len;
    int backup_fd;
    int origin_fd;
    int daemonLog_fd;
    char pid_string[BUF_MAX];
    char daemonbuf[PATH_MAX];
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    memset(formattedTime, 0, strlen(formattedTime));
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", tm_now);

    init_backup_list();

    int result = processFilesIndirectory(path, backupDirPath, backup_dir_list, daemonLogPath, pid);

    // 생성해야
    if (result == -1)
    {
        if ((daemonLog_fd = open(daemonLogPath, O_WRONLY | O_CREAT | O_APPEND, 0777)) < 0)
        {
            fprintf(stderr, "ERROR: daemon log open error\n");
            log_error("ERROR: daemon log open error1");
        }
        sprintf(daemonbuf, "[%s] [%s] [%s] \n", formattedTime, "create", path);
        if (write(daemonLog_fd, daemonbuf, strlen(daemonbuf)) == -1)
        {
            fprintf(stderr, "ERROR: daemon log write error\n");
            log_error("ERROR: daemon log write error\n");
        }
        close(daemonLog_fd);
        // 백업로직
        // @todo: 백업파일 생성
        // 실행경로 제거
        char *removeExePath = remove_base_path(path, exePATH);
        if (removeExePath == NULL || strlen(removeExePath) == 0)
        {
            fprintf(stderr, "ERROR: invalid path\n");
            log_error("ERROR: invalid path\\n");
            return 0;
        }

        char removeExePathBuf[PATH_MAX];
        strcpy(removeExePathBuf, removeExePath);

        char *filename = strrchr(removeExePath, '/');
        if (filename != NULL)
        {
            filename++;
        }
        else
        {
            // '/'가 없는 경우, 전체 문자열이 파일 이름임을 가정
            filename = removeExePath;
        }
        now = time(NULL);
        tm_now = localtime(&now);
        memset(formattedTime, 0, strlen(formattedTime));
        strftime(formattedTime, sizeof(formattedTime), "%Y%m%d%H%M%S", tm_now);
        memset(backupFilePath, 0, PATH_MAX);
        sprintf(backupFilePath, "%s/%s_%s", backupDirPath, filename, formattedTime);
        if ((backup_fd = open(backupFilePath, O_CREAT | O_WRONLY | O_TRUNC)) < 0)
        {
            fprintf(stderr, "ERROR: open error\n");
            log_error("ERROR: open error");
        }
        if ((origin_fd = open(path, O_RDONLY)) < 0)
        {
            fprintf(stderr, "ERROR: open error\n");
            log_error("ERROR: open error");
        }
        while (1)
        {
            len = read(origin_fd, buf, strlen(buf));
            if (len <= 0)
            {
                break;
            }
            write(backup_fd, buf, len);
        }
        close(backup_fd);
        close(origin_fd);
        close(daemonLog_fd);
        init_backup_list();
    }

    // 삭제 판단
    findBackupFileByPidOptionD(backup_dir_list, pid, path, daemonLogPath);

    return 0;
}

int initDaemon(char *path, int option)
{
    int monitorList_fd;
    int daemonLog_fd;
    pid_t pid;
    pid_t daemon_pid = 0;
    char daemonLogPath[PATH_MAX];
    struct stat statbuf;
    char formattedTime[20];
    char backupDirPath[PATH_MAX];
    int cnt;
    struct dirent **namelist;
    char backupFilePath[PATH_MAX];
    char pid_string[PATH_MAX];
    int path_fd;
    int backupFile_fd;
    char buf[PATH_MAX];

    // 자식 프로세스 생성
    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "ERROR: fork error\n");
        log_error("ERROR: fork error");
    }
    // 자식을 데몬으로
    else if (pid == 0)
    {

        if ((daemon_pid = createDaemon()) < 0)
        {
            fprintf(stderr, "ERROR: create daemon error\n");
            log_error("ERROR: create daemon error");
            exit(1);
        }
        char monitorbuf[PATH_MAX * 2];
        int len;

        // monitor_list에 로그 써주기
        if ((monitorList_fd = open(monitorLogPATH, O_WRONLY | O_APPEND | O_CREAT)) < 0)
        {
            fprintf(stderr, "ERROR: monitor log list open error\n");
            log_error("ERROR: monitor log list open error");
        }
        sprintf(monitorbuf, "%d : %s\n", daemon_pid, path);
        if (write(monitorList_fd, monitorbuf, strlen(monitorbuf)) == -1)
        {
            fprintf(stderr, "ERROR: monitor log list write error\n");
            log_error("ERROR: monitor log list  write error");
        }

        // $(pid).log파일 생성
        sprintf(daemonLogPath, "%s/%d.log", backupPATH, daemon_pid);

        // 백업 디렉토리 생성
        sprintf(backupDirPath, "%s/%d", backupPATH, daemon_pid);
        if (access(backupDirPath, F_OK))
            mkdir(backupDirPath, 0777);

        if (lstat(path, &statbuf) < 0)
        {
            fprintf(stderr, "ERROR: lstat error\n");
            log_error("ERROR:lstat error");
        }

        if (S_ISDIR(statbuf.st_mode) && (option & OPT_D))
        {
            while (!signalRecv)
            {
                monitorPathByOptionD(path, daemon_pid, daemonLogPath, backupDirPath);
                sleep(1);
            }
        }
        else if (option & OPT_R)
        {
            // 재귀 모니터링 로직
        }
        else if (S_ISREG(statbuf.st_mode))
        {
            while (!signalRecv)
            {

                monitorPath(path, daemon_pid, daemonLogPath, backupDirPath);
                sleep(1);
            }
        }

        return daemon_pid;
    }
    else
    {

        int status;
        waitpid(pid, &status, 0);
    }
}

int ssu_add(char *path, int option)
{

    pid_t daemon_pid;
    daemon_pid = initDaemon(path, option);
    char realpathBuf[PATH_MAX];
    realpath(path, realpathBuf);

    printf("monitoring started (%s) : %d\n", realpathBuf, daemon_pid + 1);

    return 0;
}

int check_monitorList(dirNode *dirNode, char *path)
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
                char resolved_origin_path[PATH_MAX];
                char resolved_input_path[PATH_MAX];

                // 경로를 절대 경로로 변환
                realpath(bNode->origin_path, resolved_origin_path);
                realpath(path, resolved_input_path);

                if (!strcmp(resolved_origin_path, resolved_input_path))
                {
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
        int res = check_monitorList(dirNode, path);
        if (res == 1)
        {
            return 1;
        }

        dirNode = dirNode->next_dir;
    }

    return -1;
}

void init_monitorlist()
{
    int list_fd;
    int len;
    char buf[PATH_MAX];

    if ((list_fd = open(monitorLogPATH, O_RDONLY)) < 0)
    {
        fprintf(stderr, "ERROR: open error\n");
        return;
    }

    while ((len = read(list_fd, buf, BUF_MAX)) > 0)
    {
        char *ptr = strchr(buf, '\n');
        if (ptr != NULL)
        {
            ptr[0] = '\0';
        }

        char *pid_string = strtok(buf, " : ");
        char *path_string = strtok(NULL, " : ");

        if (pid_string && path_string)
        {
            char resolved_path[PATH_MAX];
            realpath(path_string, resolved_path); // 경로를 절대 경로로 변환

            backup_list_insert(monitor_dir_list, "", resolved_path, "", pid_string);
        }
    }

    close(list_fd);
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

    char *filename = argv[1];
    int option = atoi(argv[2]);
    struct stat statbuf;

    if (init(argv[0]) == -1)
    {
        fprintf(stderr, "ERROR: init error.\n");
        log_error("ERROR: init error ");
        return -1;
    }
    if (lstat(filename, &statbuf) < 0)
    {
        fprintf(stderr, "ERROR: lstat error for %s \n", filename);
        log_error("ERROR: lstat error ");
        return 1;
    }

    // 디렉토리인데 옵션으로 -d 또는 -r로 지정되지 않았다면 에러 메시지를 출력하고 -1을 반환
    if (S_ISDIR(statbuf.st_mode) && !((option & OPT_D) || (option & OPT_R)))
    {
        fprintf(stderr, "ERROR: %s is a directory \n", filename);

        return -1;
    }

    // 파일인데 옵션으로 -d 또는 -r로 지정했다면 에러 메시지를 출력하고 -1을 반환
    if (S_ISREG(statbuf.st_mode) && ((option & OPT_D) || (option & OPT_R)))
    {
        fprintf(stderr, "ERROR: %s is a file \n", filename);
        return -1;
    }

    // @todo: 인자로 입력받은 경로에 대해 데몬 프로세스가 이미 존재한다면 에러 처리 후 프롬프트 재출력

    init_monitorlist();
    int checkval = check_monitorList(monitor_dir_list, filename);

    if (checkval == 1)
    {
        fprintf(stderr, "ERROR: daemon process already in logs\n");
        exit(1);
    }
    ssu_add(filename, option);

    exit(0);
}