# 2024 Spring Linux System Programming

## Getting started
- Linux
Ubuntu 20.04 LTS 
- openssl  
    You need to install openssl library 

    sudo apt-get install libssl-dev 

- git clone  https://github.com/JjungminLee/linux.git


## P1 : SSU-Backup 

A program that manages user-desired file backups in a backup folder, handled through a global linked list and metadata, allowing for flexible deletion, restoration, and listing of files. 

### Usage 

When the program is run, a backup folder and a metadata file named "Kathy.meta.txt" are created. This metadata file stores the state of the original files alongside their corresponding backup files, formatted as “OriginalFilePath@BackupFilePath”. When deleting the backup folder, the metadata file must also be deleted to accurately reflect the state in the global linked list. The global backup linked list holds nodes that contain both the original and backup paths.



## p2 : SSU-Repo 

A program where users can add file paths to a staging list, which then tracks whether the files are new, removed, or modified. This enables free generation of commit, status, and log outputs. The program also backs up files during a revert operation. If a file path is removed from the staging list, it will no longer be tracked. The program manages committed files through a linked list.

## p3 : SSU-Sync 
When a user enters a specific path, the program uses a daemon process to monitor it for modifications and deletions. If a modification occurs, a backup file is created, but no backup file is made in the case of deletion. Backup logs are viewable in a tree format. Removing the daemon process's PID not only stops the daemon but also deletes all associated backup files.
