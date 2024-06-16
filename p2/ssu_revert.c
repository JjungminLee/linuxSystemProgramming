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

int main(int argc, char *argv[])
{
    printf(" %s revert옴\n", argv[0]);
    exit(0);
}