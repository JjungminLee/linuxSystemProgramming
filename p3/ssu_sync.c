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

void removeExec(char *pid_string)
{

    pid_t pid;

    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "ERROR: fork error\n");
        exit(1);
    }
    else if (pid == 0)
    {

        if ((execl("./ssu_remove", "./ssu_remove", pid_string, (char *)NULL) == -1))
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

void addExec(char *filename, int option)
{
    char option_char[PATH_MAX];
    sprintf(option_char, "%d", option);
    pid_t pid;

    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "ERROR: fork error\n");
        exit(1);
    }
    else if (pid == 0)
    {
        if ((execl("./ssu_add", "./ssu_add", filename, option_char, (char *)NULL) == -1))
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
void listExec(char *arg)
{
    pid_t pid;

    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "ERROR: fork error\n");
        exit(1);
    }
    else if (pid == 0)
    {
        if ((execl("./ssu_list", "./ssu_list", arg, (char *)NULL) == -1))
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
    int lastind;
    int optind;
    int option;
    int optopt;
    int optarg;
    int optcnt;
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
            !strncmp(parameter->filename, backupPATH, strlen(backupPATH)))
        {
            fprintf(stderr, "ERROR: filename %s can't be add \n",
                    parameter->filename);

            return -1;
        }

        // 옵션 처리
        lastind = 2;
        optind = 0;

        while ((option = getopt(argcnt, arglist, "drt:")) != -1)
        {

            // 옵션이 올바르지 않은 경우
            if (option != 'd' && option != 'r' && option != 't')
            {
                fprintf(stderr, "ERROR: unknown option %c\n", optopt);
                printf("Usage:\n");
                printf("    -d : add new daemon process of <PATH> if <PATH> is directory\n");
                printf("    -r : add new daemon process of <PATH> recursive if <PATH> is directory \n");
                printf("    -t <TIME> : set daemon process time to <TIME> sec (default : 1sec) \n");
                return -1;
            }
            // 옵션이 올바르지 않은 경우
            if (optind == lastind)
            {
                fprintf(stderr, "ERROR: wrong option input\n");
                printf("Usage:\n");
                printf("    -d : add new daemon process of <PATH> if <PATH> is directory\n");
                printf("    -r : add new daemon process of <PATH> recursive if <PATH> is directory \n");
                printf("    -t <TIME> : set daemon process time to <TIME> sec (default : 1sec) \n");
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

            if (option == 't')
            {
                if (parameter->commandopt & OPT_T)
                {
                    fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                    return -1;
                }
                parameter->commandopt |= OPT_T;
            }

            // 옵션 개수 세주기
            optcnt++;
            lastind = optind;
        }

        int res = argcnt - optcnt;

        if (res != 3 && res != 2)
        {
            fprintf(stderr, "argument error\n");
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
                    "ERROR: <PID> is not include \n");
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
        else if (!strcmp(arglist[0], "list"))
        {
            command = CMD_LIST;
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

        if (command & (CMD_ADD | CMD_REMOVE | CMD_LIST))
        {
            parameterInit(&parameter);
            parameter.command = arglist[0];

            if (parameter_processing(argcnt, arglist, command, &parameter) == -1)
            {
                continue;
            }

            if (command & CMD_ADD)
            {

                int option = parameter.commandopt;
                char *filename = parameter.filename;

                addExec(filename, option);
            }
            else if (command & CMD_REMOVE)
            {

                removeExec(arglist[1]);
            }
            else if (command & CMD_LIST)
            {

                if (argcnt == 1)
                {
                    listExec(NULL);
                }
                else if (argcnt == 2)
                {
                    listExec(arglist[1]);
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
    backupPATH = (char *)malloc(PATH_MAX);
    monitorLogPATH = (char *)malloc(PATH_MAX);

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