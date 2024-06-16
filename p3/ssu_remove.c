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
char removePath[PATH_MAX];

int hash = 0;
int isValid = -1;

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
void rewrite_monitorlist(dirNode *dirNode, int fd, char *pid_string)
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
                if (strcmp(bNode->pid, pid_string))
                {
                    char logbuf[PATH_MAX];
                    sprintf(logbuf, "%s : %s\n", bNode->pid, bNode->origin_path);
                    write(fd, logbuf, strlen(logbuf));
                }
            }
            bNode = bNode->next_backup;
        }
        fNode = fNode->next_file;
    }
    dirNode = dirNode->subdir_head;
    while (dirNode != NULL)
    {
        rewrite_monitorlist(dirNode, fd, pid_string);
        dirNode = dirNode->next_dir;
    }
}

void append_monitorList(int pid)
{
    int monitor_fd;
    char monitor_path[PATH_MAX];
    int len;
    char buf[PATH_MAX];
    char delete_pid_string[PATH_MAX];
    sprintf(delete_pid_string, "%d", pid);

    sprintf(monitor_path, "%s/%s", backupPATH, "monitor_list.log");

    if ((monitor_fd = open(monitor_path, O_RDONLY)) < 0)
    {
        fprintf(stderr, "ERROR: open error\n");
        log_error("ERROR: open error");
    }

    while ((len = read(monitor_fd, buf, BUF_MAX)) > 0)
    {
        buf[len] = '\0';

        char *ptr = strchr(buf, '\n');
        if (ptr != NULL)
        {
            *ptr = '\0';

            lseek(monitor_fd, -(len - (ptr - buf + 1)), SEEK_CUR);

            char *pid_string;
            char *path_string;

            // 첫 번째 " : "을 기준으로 문자열을 분리합니다.
            pid_string = strtok(buf, " : ");

            // 다음 " : "을 기준으로 나머지 문자열을 분리합니다.
            path_string = strtok(NULL, " : ");

            backup_list_insert(monitor_dir_list, "", path_string, "", pid_string);
        }
    }
    close(monitor_fd);

    int findPidValid = find_pid_valid(monitor_dir_list, delete_pid_string);
    char pidbuf[PATH_MAX];
    sprintf(pidbuf, "%d", findPidValid);
    log_error(pidbuf);

    if (findPidValid == -1)
    {
        fprintf(stderr, "ERROR: pid doesn't valid\n");
    }
    if ((monitor_fd = open(monitor_path, O_WRONLY | O_TRUNC)) < 0)
    {
        fprintf(stderr, "ERROR: open error\n");
    }

    char *path = find_path_by_pid(monitor_dir_list, delete_pid_string);

    printf("monitoring ended (%s) : %d\n", path, pid);
    rewrite_monitorlist(monitor_dir_list, monitor_fd, delete_pid_string);
}
void remove_backup_directory_and_file(char *dirPath)
{
    struct stat statbuf;
    struct dirent **namelist;
    int cnt;
    if ((cnt = scandir(dirPath, &namelist, NULL, alphasort)) == -1)
    {

        fprintf(stderr, "ERROR: scandir error for %s\n", dirPath);
    }
    if (stat(dirPath, &statbuf) < 0)
    {
        fprintf(stderr, "ERROR:scandir error\n");
    }
    if (S_ISREG(statbuf.st_mode))
    {
        if (remove(dirPath) < 0)
        {
            fprintf(stderr, "ERROR:remove error\n");
            log_error("ERROR:remove error");
        }
    }
    for (int i = 0; i < cnt; i++)
    {
        char tmp_path[PATH_MAX];
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
        {
            continue;
        }
        sprintf(tmp_path, "%s/%s", dirPath, namelist[i]->d_name);
        if (stat(tmp_path, &statbuf) < 0)
        {
            fprintf(stderr, "ERROR: stat error\n");
        }
        if (S_ISDIR(statbuf.st_mode))
        {
            remove_backup_directory_and_file(tmp_path);
        }
        else if (S_ISREG(statbuf.st_mode))
        {
            if (remove(tmp_path) < 0)
            {
                fprintf(stderr, "ERROR:remove error\n");
                log_error("ERROR:remove error");
            }
        }
    }
}

void ssu_remove(int pid)
{
    // pid받으면 해당 프로세스 kill  SIGUSR1 시그널을 보내 데몬 프로세스를 종료
    char pidLogPath[PATH_MAX];
    char backupDirPath[PATH_MAX];
    sprintf(pidLogPath, "%s/%d.log", backupPATH, pid);
    sprintf(backupDirPath, "%s/%d", backupPATH, pid);
    int kill_ans;
    kill_ans = kill(pid, SIGUSR1);
    if (kill_ans == -1)
    {
        printf("ERROR: invalid pid\n");
    }
    if (kill_ans == 0)
    {
        // monitor_list에서 pid있는 줄 삭제하고 전체 다시쓰기
        append_monitorList(pid);

        // 백업폴더 및 하위 파일들 삭제
        remove_backup_directory_and_file(backupDirPath);
        if (remove(backupDirPath) < 0)
        {
            fprintf(stderr, "ERROR:remove error\n");
            log_error("ERROR:remove error");
        }

        // {pid}.log삭제
        if (remove(pidLogPath) < 0)
        {
            fprintf(stderr, "ERROR:remove error\n");
        }
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

    if (init(argv[0]) == -1)
    {
        fprintf(stderr, "ERROR: init error.\n");
        return -1;
    }
    int pid = atoi(argv[1]);

    ssu_remove(pid);

    exit(0);
}