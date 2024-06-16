#include "ssu_header.h"

char *get_file_name(char *path)
{
    int i;
    char tmp_path[PATH_MAX];
    char *file_name = (char *)malloc(sizeof(char) * FILE_MAX);

    strcpy(tmp_path, path);
    for (i = 0; tmp_path[i]; i++)
    {
        if (tmp_path[i] == '/')
            strcpy(file_name, tmp_path + i + 1);
    }

    return file_name;
}

char *cvt_time_2_str(time_t time)
{
    struct tm *time_tm = localtime(&time);
    char *time_str = (char *)malloc(sizeof(char) * 32);
    sprintf(time_str, "%02d%02d%02d%02d%02d%02d",
            (time_tm->tm_year + 1900) % 100,
            time_tm->tm_mon + 1,
            time_tm->tm_mday,
            time_tm->tm_hour,
            time_tm->tm_min,
            time_tm->tm_sec);
    return time_str;
}

char *substr(char *str, int beg, int end)
{
    char *ret = (char *)malloc(sizeof(char) * (end - beg + 1));

    for (int i = beg; i < end && *(str + i) != '\0'; i++)
    {
        ret[i - beg] = str[i];
    }
    ret[end - beg] = '\0';

    return ret;
}

char *c_str(char *str)
{
    return substr(str, 0, strlen(str));
}

int path_list_init(pathNode *curr_path, char *path)
{
    pathNode *new_path = curr_path;
    char *ptr;
    char *next_path = "";

    if (!strcmp(path, ""))
        return 0;

    if (ptr = strchr(path, '/'))
    {
        next_path = ptr + 1;
        ptr[0] = '\0';
    }

    if (!strcmp(path, ".."))
    {
        new_path = curr_path->prev_path;
        new_path->tail_path = new_path;
        new_path->next_path = NULL;
        new_path->head_path->tail_path = new_path;

        new_path->head_path->depth--;

        if (new_path->head_path->depth == 0)
            return -1;
    }
    else if (strcmp(path, "."))
    {
        new_path = (pathNode *)malloc(sizeof(pathNode));
        strcpy(new_path->path_name, path);

        new_path->head_path = curr_path->head_path;
        new_path->tail_path = new_path;

        new_path->prev_path = curr_path;
        new_path->next_path = curr_path->next_path;

        curr_path->next_path = new_path;
        new_path->head_path->tail_path = new_path;

        new_path->head_path->depth++;
    }

    if (strcmp(next_path, ""))
    {
        return path_list_init(new_path, next_path);
    }

    return 0;
}

char *cvt_path_2_realpath(char *path)
{
    pathNode *path_head;
    pathNode *curr_path;
    char *ptr;
    char origin_path[PATH_MAX];
    char ret_path[PATH_MAX] = "";

    path_head = (pathNode *)malloc(sizeof(pathNode));
    path_head->depth = 0;
    path_head->tail_path = path_head;
    path_head->head_path = path_head;
    path_head->next_path = NULL;

    if (path[0] != '/')
    {
        sprintf(origin_path, "%s/%s", pwdPATH, path);
    }
    else
    {
        strcpy(origin_path, path);
    }

    if (path_list_init(path_head, origin_path) == -1)
    {
        return NULL;
    }

    curr_path = path_head->next_path;
    while (curr_path != NULL)
    {
        strcat(ret_path, curr_path->path_name);
        if (curr_path->next_path != NULL)
        {
            strcat(ret_path, "/");
        }
        curr_path = curr_path->next_path;
    }

    if (strlen(ret_path) == 0)
    {
        strcpy(ret_path, "/");
    }

    return c_str(ret_path);
}

int make_dir_path(char *path)
{
    pathNode *path_head;
    pathNode *curr_path;
    char ret_path[PATH_MAX] = "";
    struct stat statbuf;

    path_head = (pathNode *)malloc(sizeof(pathNode));
    path_head->depth = 0;
    path_head->tail_path = path_head;
    path_head->head_path = path_head;
    path_head->next_path = NULL;

    curr_path = path_head;

    if (path_list_init(path_head, c_str(path)) == -1)
    {
        return -1;
    }

    curr_path = path_head->next_path;
    while (curr_path != NULL)
    {
        strcat(ret_path, curr_path->path_name);
        strcat(ret_path, "/");

        curr_path = curr_path->next_path;

        if (!access(ret_path, F_OK))
        {
            if (lstat(ret_path, &statbuf) < 0)
            {
                fprintf(stderr, "ERROR: lstat error for %s\n", ret_path);
                return -1;
            }

            if (!S_ISDIR(statbuf.st_mode))
            {
                fprintf(stderr, "ERROR: %s is already exist as file\n", ret_path);
                return -1;
            }

            continue;
        }

        if (mkdir(ret_path, 0755) == -1)
        {
            fprintf(stderr, "ERROR: mkdir error for '%s'\n", ret_path);
            return -1;
        };
    }

    return 0;
}

int md5(char *target_path, char *hash_result)
{
    FILE *fp;
    unsigned char hash[MD5_DIGEST_LENGTH];
    unsigned char buffer[SHRT_MAX];
    int bytes = 0;
    MD5_CTX md5;

    if ((fp = fopen(target_path, "rb")) == NULL)
    {
        printf("ERROR: fopen error for %s\n", target_path);
        return 1;
    }

    MD5_Init(&md5);

    while ((bytes = fread(buffer, 1, SHRT_MAX, fp)) != 0)
        MD5_Update(&md5, buffer, bytes);

    MD5_Final(hash, &md5);

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(hash_result + (i * 2), "%02x", hash[i]);
    hash_result[HASH_MD5 - 1] = 0;

    fclose(fp);

    return 0;
}

int cvtHash(char *target_path, char *hash_result)
{
    md5(target_path, hash_result);
}

int cmpHash(char *path1, char *path2)
{
    char *hash1 = (char *)malloc(sizeof(char) * HASH_MD5);
    char *hash2 = (char *)malloc(sizeof(char) * HASH_MD5);

    cvtHash(path1, hash1);
    cvtHash(path2, hash2);

    return strcmp(hash1, hash2);
}

char *quote_check(char **str, char del)
{
    char *tmp = *str + 1;
    int i = 0;

    while (*tmp != '\0' && *tmp != del)
    {
        tmp++;
        i++;
    }
    if (*tmp == '\0')
    {
        *str = tmp;
        return NULL;
    }
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

char *tokenize(char *str, char *del)
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
            quote_check(&tmp, *tmp);
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

char **get_substring(char *str, int *cnt, char *del)
{
    *cnt = 0;
    int i = 0;
    char *token = NULL;
    char *templist[100] = {
        NULL,
    };
    token = tokenize(str, del);
    if (token == NULL)
    {
        return NULL;
    }

    while (token != NULL)
    {
        templist[*cnt] = token;
        *cnt += 1;
        token = tokenize(NULL, del);
    }

    char **temp = (char **)malloc(sizeof(char *) * (*cnt + 1));
    for (i = 0; i < *cnt; i++)
    {
        temp[i] = templist[i];
    }
    return temp;
}
char *getRelativePath(char *absolutePath)
{

    char cwd[1024];
    getcwd(cwd, 1024);
    // absolutePath 내에서 cwd를 찾음
    char *found = strstr(absolutePath, cwd);

    // cwd 이후의 문자열 시작 위치 계산
    char *startAfterCwd = found + strlen(cwd);

    // '/' 문자로 시작하면 건너뛰기
    if (*startAfterCwd == '/')
        startAfterCwd++;
    char *newPath = (char *)malloc(STR_MAX);
    memset(newPath, 0, STR_MAX);

    if (!strcmp(startAfterCwd, ""))
    {
        strcat(newPath, ".");
    }
    else
    {
        strcat(newPath, "./");
        strcat(newPath, startAfterCwd);
    }
    return newPath;
}

void log_error(const char *message)
{
    char logPath[PATH_MAX];
    sprintf(logPath, "%s/log/log.txt", pwdPATH);
    FILE *log_file = fopen(logPath, "a");
    if (log_file)
    {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

// /home/ubuntu/a/b/c.txt에서 /home/ubuntu만 제거
char *remove_base_path(char *full_path, const char *base_path)
{

    char *pos = strstr(full_path, base_path);

    if (pos != NULL)
    {

        size_t len = strlen(base_path);

        if (pos[len] == '/')
            len++;
        return pos + len;
    }

    return full_path;
}
