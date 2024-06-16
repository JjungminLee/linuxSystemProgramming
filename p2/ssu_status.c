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
int dircnt = 0;
int change = 0;
int insert = 0;
int delete = 0;

void print_current_commit(dirNode *dirNode)
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

                printf("    %s : %s\n", bNode->command, relativePath);
            }
            bNode = bNode->next_backup;
        }
        fNode = fNode->next_file;
    }
    dirNode = dirNode->subdir_head->next_dir;
    while (dirNode != NULL)
    {
        print_current_commit(dirNode); // 재귀 호출
        dirNode = dirNode->next_dir;
    }
}

void lookup_current_commit(dirNode *dirNode)
{
    change = 0;
    insert = 0;
    delete = 0;
    if (current_commit_dir_list == NULL)
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
int remove_bfs(char *target_path, char *staging_path)
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
            fprintf(stderr, "ERROR1: lstat error for %s\n", curr_dir->dir_name);
            return -1;
        }

        if (S_ISDIR(statbuf.st_mode))
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
                    fprintf(stderr, "ERROR2: lstat error for %s\n", tmp_path);
                    return -1;
                }

                if (S_ISREG(statbuf.st_mode) && !strcmp(target_path, tmp_path))
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
int traverseRemoveBackupNodesReverse(backupNode *backup_head, char *target_path)
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
            if (remove_bfs(backup->origin_path, target_path) < 0)
            {
                // 전역 currentlist에 추가
                dircnt++;
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
int traverseRemoveFileNodesReverse(fileNode *file_head, char *target_path)
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
            int result = traverseRemoveBackupNodesReverse(lastFile->backup_head, target_path);
            if (result == 1)
                return 1; // 파일 수정 발견
        }
        lastFile = lastFile->prev_file;
    }
    return -1;
}

int traverseRemoveDirNodesReverse(dirNode *dir_head, char *target_path)
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
            int result = traverseRemoveFileNodesReverse(lastDir->file_head, target_path);
            if (result == 1)
                return 1; // 파일 수정 발견
        }
        if (lastDir->subdir_head)
        {
            int result = traverseRemoveDirNodesReverse(lastDir->subdir_head, target_path);
            if (result == 1)
                return 1; // 하위 디렉토리에서 파일 수정 발견
        }
        lastDir = lastDir->prev_dir;
    }
    return -1;
}
void check_remove_file(dirNode *dirNode, char *staging_path)
{
    // 역순으로 commit list순회
    if (traverseRemoveDirNodesReverse(commit_dir_list, staging_path) == 1)
    {
    }
    else
    {
        return;
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

int check_add_staging_list(dirNode *dirNode, char *target_path)
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
        if (check_add_staging_list(dirNode, target_path) == 1)
        {
            return 1; // 하위에서 일치하는 경로 발견
        }
        dirNode = dirNode->next_dir;
    }

    return -1; // 일치하는 경로를 찾지 못함
}

// 스테이징 경로가 현재 작업경로에 존재하지 않으면 -1반환
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

                // 커밋경로랑 tmp_path가 같다는건 스테이징 경로에 해당 커밋 경로가 있다는거! 즉 안없어진걸 의미
                int len = strlen(target_path) < strlen(tmp_path) ? strlen(target_path) : strlen(tmp_path);

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
// 이미 커밋된건지 판단 (이미 commit_dir_list에 있는 경로인지 판단) 이미 커밋된거면 1반환
int is_commited(dirNode *dirNode, char *target_path)
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
                    return 1;
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
        // staging 로그에서 remove된 애인지 판단
        if (check_remove_staging_list(staging_remove_dir_list, curr_dir->dir_name) > 0)
        {
            return;
        }

        if (S_ISREG(statbuf.st_mode))
        {

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
                    fprintf(stderr, "ERROR4: lstat error for %s\n", tmp_path);
                    return;
                }

                int num = check_remove_staging_list(staging_remove_dir_list, tmp_path);
                // staging 로그에서 remove된 애인지 판단 -> remove되어야 하는 애면 가차없이 리턴
                if (check_remove_staging_list(staging_remove_dir_list, tmp_path) > 0)
                {
                    return;
                }

                if (S_ISREG(statbuf.st_mode))
                {

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
void check_staging_list(dirNode *dirNode)
{
    if (dirNode == NULL)
    {
        return;
    }

    // 파일 노드 순회 시작
    fileNode *fNode = dirNode->file_head->next_file;
    while (fNode != NULL)
    {
        backupNode *bNode = fNode->backup_head->next_backup;
        while (bNode != NULL)
        {
            if (strlen(bNode->origin_path) > 0)
            {
                if (check_exist_pwd(bNode->origin_path) > 0)
                {

                    bfs(bNode->origin_path);
                }

                // 백업 파일에는 있는데 현재 디렉토리에는 없는 경우

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
int untrack_cnt = 0;
void untrack_bfs()
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

    dirNode_append(dirList, exePATH, "");

    curr_dir = dirList->next_dir;

    while (curr_dir != NULL)
    {

        if (lstat(curr_dir->dir_name, &statbuf) < 0)
        {
            fprintf(stderr, "ERROR5: lstat error for %s\n", curr_dir->dir_name);
            return;
        }
        // staging 로그에서 remove된 애인지 판단
        if (check_remove_staging_list(staging_remove_dir_list, curr_dir->dir_name) > 0 || check_add_staging_list(staging_dir_list, curr_dir->dir_name) > 0)
        {
            continue;
        }

        if (S_ISREG(statbuf.st_mode))
        {
            untrack_cnt++;
            backup_list_insert(untrack_dir_list, "new file", curr_dir->dir_name, "", "");
        }
        else if (S_ISDIR(statbuf.st_mode))
        {

            if ((cnt = scandir(curr_dir->dir_name, &namelist, NULL, alphasort)) == -1)
            {
                fprintf(stderr, "ERROR: scandir error for %s\n", curr_dir->dir_name);
                return;
            }

            for (int i = 0; i < cnt; i++)
            {

                if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
                {
                    continue;
                }

                char tmp_path[PATH_MAX];
                memset(tmp_path, 0, PATH_MAX);
                sprintf(tmp_path, "%s/%s", curr_dir->dir_name, namelist[i]->d_name);

                if (lstat(tmp_path, &statbuf) < 0)
                {
                    fprintf(stderr, "ERROR6: lstat error for %s\n", tmp_path);
                    return;
                }

                // // remove나 add된건지 판단
                if (check_remove_staging_list(staging_remove_dir_list, tmp_path) > 0 || check_add_staging_list(staging_dir_list, tmp_path) > 0)
                {
                    continue;
                }

                if (S_ISREG(statbuf.st_mode))
                {
                    untrack_cnt++;
                    backup_list_insert(untrack_dir_list, "new file", tmp_path, "", "");
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

void print_current_untrack(dirNode *dirNode)
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

                printf("    %s : %s\n", bNode->command, relativePath);
            }
            bNode = bNode->next_backup;
        }
        fNode = fNode->next_file;
    }
    dirNode = dirNode->subdir_head->next_dir;
    while (dirNode != NULL)
    {
        print_current_untrack(dirNode); // 재귀 호출
        dirNode = dirNode->next_dir;
    }
}
void print_untrack_files()
{
    untrack_bfs();
    printf("Untracked files:\n");
    if (untrack_cnt == 0)
    {
        return;
    }
    else
    {
        print_current_untrack(untrack_dir_list);
    }
}
int ssu_status()
{
    int fd;
    if ((fd = open(commitLogPATH, O_WRONLY | O_CREAT | O_APPEND)) < 0)
    {
        fprintf(stderr, "ERROR: file open error %s\n", commitLogPATH);
    }

    // @현재작업경로 내의 모든 파일들
    check_staging_list(staging_dir_list);
    check_remove_file(commit_dir_list, exePATH);

    if (dircnt == 0)
    {
        printf("Nothing to commit\n");
    }
    else
    {
        // 개수만 세는 용도
        lookup_current_commit(current_commit_dir_list);

        printf("Changes to be committed:\n");
        print_current_commit(current_commit_dir_list);

        // @todo : untrack확인하기
        // 현재 작업경로 내에 있는 파일 들 중에 staging_dir_list와 remove_staging_dir_list에 없는애들
        print_untrack_files();
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
    untrack_dir_list = (dirNode *)malloc(sizeof(dirNode));
    dirNode_init(untrack_dir_list);
    // 현재 커밋 담는 리스트 init
    current_commit_dir_list = (dirNode *)malloc(sizeof(dirNode));
    dirNode_init(current_commit_dir_list);

    if (init(argv[0]) == -1)
    {
        fprintf(stderr, "ERROR: init error.\n");
        return -1;
    }

    ssu_status();
    exit(0);
}