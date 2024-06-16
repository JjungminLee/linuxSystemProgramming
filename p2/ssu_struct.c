#include "ssu_header.h"

void dirNode_init(dirNode *dir_node)
{
  dir_node->root_dir = NULL;

  dir_node->file_cnt = 0;
  dir_node->subdir_cnt = 0;
  dir_node->backup_cnt = 0;
  memset(dir_node->dir_path, 0, sizeof(dir_node->dir_path));
  memset(dir_node->dir_name, 0, sizeof(dir_node->dir_name));

  dir_node->file_head = (fileNode *)malloc(sizeof(fileNode));
  dir_node->file_head->prev_file = NULL;
  dir_node->file_head->next_file = NULL;
  dir_node->file_head->root_dir = dir_node;

  dir_node->subdir_head = (dirNode *)malloc(sizeof(dirNode));
  dir_node->subdir_head->prev_dir = NULL;
  dir_node->subdir_head->next_dir = NULL;
  dir_node->subdir_head->root_dir = dir_node;

  dir_node->prev_dir = NULL;
  dir_node->next_dir = NULL;
}

dirNode *dirNode_get(dirNode *dir_head, char *dir_name)
{
  dirNode *curr_dir = dir_head->next_dir;

  while (curr_dir != NULL)
  {
    if (curr_dir != NULL && !strcmp(dir_name, curr_dir->dir_name))
    {
      return curr_dir;
    }
    curr_dir = curr_dir->next_dir;
  }
  return NULL;
}

dirNode *dirNode_insert(dirNode *dir_head, char *dir_name, char *dir_path)
{
  dirNode *curr_dir = dir_head;
  dirNode *next_dir, *new_dir;

  while (curr_dir != NULL)
  {
    next_dir = curr_dir->next_dir;

    if (next_dir != NULL && !strcmp(dir_name, next_dir->dir_name))
    {
      return curr_dir->next_dir;
    }
    else if (next_dir == NULL || strcmp(dir_name, next_dir->dir_name) < 0)
    {
      new_dir = (dirNode *)malloc(sizeof(dirNode));
      dirNode_init(new_dir);

      new_dir->root_dir = dir_head->root_dir;

      sprintf(new_dir->dir_name, "%s", dir_name);
      sprintf(new_dir->dir_path, "%s%s/", dir_path, dir_name);

      new_dir->prev_dir = curr_dir;
      new_dir->next_dir = next_dir;

      if (next_dir != NULL)
      {
        next_dir->prev_dir = new_dir;
      }
      curr_dir->next_dir = new_dir;

      dir_head->root_dir->subdir_cnt++;

      return curr_dir->next_dir;
    }
    curr_dir = next_dir;
  }
  return NULL;
}

dirNode *dirNode_append(dirNode *dir_head, char *dir_name, char *dir_path)
{
  dirNode *curr_dir = dir_head;
  dirNode *next_dir, *new_dir;

  while (curr_dir != NULL)
  {
    next_dir = curr_dir->next_dir;
    if (next_dir == NULL)
    {
      new_dir = (dirNode *)malloc(sizeof(dirNode));
      dirNode_init(new_dir);

      new_dir->root_dir = dir_head->root_dir;

      sprintf(new_dir->dir_name, "%s", dir_name);
      sprintf(new_dir->dir_path, "%s%s/", dir_path, dir_name);

      new_dir->prev_dir = curr_dir;
      new_dir->next_dir = next_dir;

      if (next_dir != NULL)
      {
        next_dir->prev_dir = new_dir;
      }
      curr_dir->next_dir = new_dir;

      // dir_head->root_dir->subdir_cnt++;

      return curr_dir->next_dir;
    }
    curr_dir = next_dir;
  }
  return NULL;
}

void dirNode_remove(dirNode *dir_node)
{
  dirNode *next_dir = dir_node->next_dir;
  dirNode *prev_dir = dir_node->prev_dir;

  if (next_dir != NULL)
  {
    next_dir->prev_dir = dir_node->prev_dir;
    prev_dir->next_dir = dir_node->next_dir;
  }
  else
  {
    prev_dir->next_dir = NULL;
  }

  free(dir_node->subdir_head);
  free(dir_node->file_head);
  dir_node->subdir_head = NULL;
  dir_node->file_head = NULL;

  free(dir_node);
}

void fileNode_init(fileNode *file_node)
{
  file_node->root_dir = NULL;

  file_node->backup_cnt = 0;
  memset(file_node->file_name, 0, sizeof(file_node->file_name));
  memset(file_node->file_path, 0, sizeof(file_node->file_path));

  file_node->backup_head = (backupNode *)malloc(sizeof(backupNode));
  file_node->backup_head->prev_backup = NULL;
  file_node->backup_head->next_backup = NULL;
  file_node->backup_head->root_file = file_node;

  file_node->prev_file = NULL;
  file_node->next_file = NULL;
}

fileNode *fileNode_get(fileNode *file_head, char *file_name)
{
  fileNode *curr_file = file_head->next_file;

  while (curr_file != NULL)
  {
    if (curr_file != NULL && !strcmp(file_name, curr_file->file_name))
    {
      return curr_file;
    }
    curr_file = curr_file->next_file;
  }
  return NULL;
}

fileNode *fileNode_insert(fileNode *file_head, char *file_name,
                          char *dir_path)
{
  fileNode *curr_file = file_head;
  fileNode *next_file, *new_file;

  while (curr_file != NULL)
  {
    next_file = curr_file->next_file;

    if (next_file != NULL && !strcmp(file_name, next_file->file_name))
    {
      return curr_file->next_file;
    }
    else if (next_file == NULL ||
             strcmp(file_name, next_file->file_name) < 0)
    {
      new_file = (fileNode *)malloc(sizeof(fileNode));
      fileNode_init(new_file);

      new_file->root_dir = file_head->root_dir;

      strcpy(new_file->file_name, file_name);
      strcpy(new_file->file_path, dir_path);
      strcat(new_file->file_path, file_name);

      new_file->prev_file = curr_file;
      new_file->next_file = next_file;

      if (next_file != NULL)
      {
        next_file->prev_file = new_file;
      }
      curr_file->next_file = new_file;

      file_head->root_dir->file_cnt++;

      return curr_file->next_file;
    }
    curr_file = next_file;
  }
  return NULL;
}

void fileNode_remove(fileNode *file_node)
{
  fileNode *next_file = file_node->next_file;
  fileNode *prev_file = file_node->prev_file;

  if (next_file != NULL)
  {
    next_file->prev_file = file_node->prev_file;
    prev_file->next_file = file_node->next_file;
  }
  else
  {
    prev_file->next_file = NULL;
  }

  free(file_node->backup_head);
  file_node->backup_head = NULL;

  free(file_node);
}

void backupNode_init(backupNode *backup_node)
{
  backup_node->root_version_dir = NULL;
  backup_node->root_file = NULL;
  memset(backup_node->origin_path, 0, sizeof(backup_node->origin_path));
  memset(backup_node->backup_path, 0, sizeof(backup_node->backup_path));
  memset(backup_node->command, 0, sizeof(backup_node->command));
  memset(backup_node->commit_message, 0, sizeof(backup_node->commit_message));

  backup_node->prev_backup = NULL;
  backup_node->next_backup = NULL;
}

backupNode *backupNode_get(backupNode *backup_head, char *command)
{
  backupNode *curr_backup = backup_head->next_backup;

  while (curr_backup != NULL)
  {
    if (curr_backup != NULL)
    {
      return curr_backup;
    }
    curr_backup = curr_backup->next_backup;
  }

  return NULL;
}

backupNode *backupNode_insert(backupNode *backup_head, char *command,
                              char *file_name, char *dir_path,
                              char *backup_path, char *commit_message)
{
  backupNode *new_backup = (backupNode *)malloc(sizeof(backupNode));
  backupNode_init(new_backup);
  new_backup->root_file = backup_head->root_file;
  strcpy(new_backup->command, command);
  strcpy(new_backup->origin_path, dir_path);
  strcat(new_backup->origin_path, file_name);
  strcpy(new_backup->backup_path, backup_path);
  strcpy(new_backup->commit_message, commit_message);

  // 백업 리스트의 끝을 찾아 새 노드를 연결
  backupNode *curr_backup = backup_head;
  while (curr_backup->next_backup != NULL)
  {
    curr_backup = curr_backup->next_backup; // 마지막 노드로 이동
  }

  // 마지막 노드와 새 노드 연결
  curr_backup->next_backup = new_backup;
  new_backup->prev_backup = curr_backup;

  // root_file의 백업 카운트 업데이트
  if (backup_head->root_file != NULL)
  {
    backup_head->root_file->backup_cnt++;
  }

  return new_backup; // 새로 삽입된 노드 반환
}

void backupNode_remove(backupNode *backup_node)
{
  backupNode *next_backup = backup_node->next_backup;
  backupNode *prev_backup = backup_node->prev_backup;

  if (next_backup != NULL)
  {
    next_backup->prev_backup = backup_node->prev_backup;
    prev_backup->next_backup = backup_node->next_backup;
  }
  else
  {
    prev_backup->next_backup = NULL;
  }

  free(backup_node);
}
