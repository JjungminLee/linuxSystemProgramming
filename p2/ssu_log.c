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

void lookup_one_commit(dirNode *dirNode, char *commit)
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

                if (!strcmp(bNode->commit_message, commit))
                {
                    printf("    - %s : \"%s\"\n", bNode->command, bNode->origin_path);
                }
            }
            bNode = bNode->next_backup;
        }
        fNode = fNode->next_file;
    }
    dirNode = dirNode->subdir_head;
    while (dirNode != NULL)
    {
        lookup_one_commit(dirNode, commit); // 재귀 호출
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

    int cnt;
    struct dirent **namelist;

    if (init(argv[0]) == -1)
    {
        fprintf(stderr, "ERROR: init error.\n");
        return -1;
    }
    if (argv[1] != NULL)
    {
        if ((cnt = scandir(repoPATH, &namelist, NULL, alphasort)) == -1)
        {
            fprintf(stderr, "ERROR: scandir error %s\n", repoPATH);
        }
        for (int i = 0; i < cnt; i++)
        {
            if (!strcmp(argv[1], namelist[i]->d_name))
            {
                printf("commit : \"%s\"\n", argv[1]);
                lookup_one_commit(commit_logs_list, argv[1]);
            }
        }
    }
    else
    {
        // @todo  커밋 이름만 뽑아오기

        if ((cnt = scandir(repoPATH, &namelist, NULL, alphasort)) == -1)
        {
            fprintf(stderr, "ERROR: scandir error %s\n", repoPATH);
        }
        for (int i = 0; i < cnt; i++)
        {
            if (!strcmp(".", namelist[i]->d_name) || !strcmp("..", namelist[i]->d_name) || !strcmp(".commit.log", namelist[i]->d_name) || !strcmp(".staging.log", namelist[i]->d_name))
            {
                continue;
            }
            printf("commit: \"%s\"\n", namelist[i]->d_name);
            lookup_one_commit(commit_logs_list, namelist[i]->d_name);
        }
    }
    exit(0);
}