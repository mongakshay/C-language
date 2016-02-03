-------- FAT FS ----------

/*
 * file:        homework.c
 * description: skeleton file for CS 5600 homework 3
 * Team#   : 20 
 * Members : Akshay & Ankeeta
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, updated April 2012
 * $Id: homework.c 452 2011-11-28 22:25:31Z pjd $
 */

#define FUSE_USE_VERSION 27

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fuse.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "cs5600fs.h"
#include "blkdev.h"

/*
 * disk access - the global variable 'disk' points to a blkdev
 * structure which has been initialized to access the image file.
 *
 * NOTE - blkdev access is in terms of 512-byte SECTORS, while the
 * file system uses 1024-byte BLOCKS. Remember to multiply everything
 * by 2.
 */


extern struct blkdev *disk;

// structure that stores the super block
struct cs5600fs_super *my_super_block = NULL;
// structure that stores the file allocation table
struct cs5600fs_entry *my_FAT = NULL;
// structure that stores the root directory
struct cs5600fs_dirent *my_root_dir = NULL;
// error code for a file / directory which does not exist
int myCheckCode = 999;
// size of a file / directory
int dirname_size = 512;

int dirname_chk = 43;
/* init - this is called once by the FUSE framework at startup.
 * This might be a good place to read in the super-block and set up
 * any global variables you need. You don't need to worry about the
 * argument or the return value.
 */

void* hw3_init(struct fuse_conn_info *conn)
{
    // allocating memory to the super block
    my_super_block = calloc(1,FS_BLOCK_SIZE);
    // read the super block into memory
    disk->ops->read(disk,0,2,(char*) my_super_block);
    // reading the length of the FAT
    int fat_length = my_super_block->fat_len;
    // allocating memory to the FAT
    my_FAT = calloc(fat_length,FS_BLOCK_SIZE);
    // reading the FAT table into memory
    disk->ops->read(disk,2, fat_length * 2,(void*) my_FAT);
    // allocating memory for the root directory
    my_root_dir = calloc(1, FS_BLOCK_SIZE);
    // assigning value to the root directory
    my_root_dir = &(my_super_block->root_dirent);
    
    return NULL;
}

// function which splits the input string based on the input delimiter
// Prof's implemented code
char *strwrd(char *s, char *buf,size_t len, char *delim)
{
    s +=strspn(s, delim);
    int n = strcspn(s, delim);
    if( len - 1 < n)
        n = len - 1;
    memset(buf, 0, len);
    memcpy(buf,s, n);
    s +=n;
    return s;
}


// function to find the offset of the input directory inside its parent directory
int get_dir_offset(char *dirname, struct cs5600fs_dirent *my_dir)
{
    int i = 0;
    int num_of_dirs = 16;
    // iterating through the directories / files inside the input parent directory
    for(i = 0; i < num_of_dirs; i++)
    {
        // checking if the directory / file is valid and is the same as the input name
        if(my_dir[i].valid && strcmp(my_dir[i].name, dirname) == 0)
            return i;
    }
    // return the file not found error as we do not find the input file even after iterating
    // through all of the 16 entries of the parent directory
    return myCheckCode;
}


// akshay will write comments

int set_stat_struct(struct stat *sb, struct cs5600fs_dirent my_dir)
{
    
    sb->st_dev = 0;           /* ID of device containing file */
    sb->st_mode = my_dir.mode|(my_dir.isDir ? S_IFDIR : S_IFREG); /* Mode of file */
    sb->st_nlink = 1;         /* Number of hard links */
    sb->st_ino = 0;           /* File serial number */
    sb->st_uid = my_dir.uid;           /* User ID of the file */
    sb->st_gid = my_dir.gid;           /* Group ID of the file */
    sb->st_rdev = 0;          /* Device ID */
    sb->st_mtime = my_dir.mtime;     /* time of last access */
    sb->st_ctime = my_dir.mtime;     /* time of last data modification */
    sb->st_atime = my_dir.mtime;     /* time of last status change */
    sb->st_size = my_dir.length;          /* file size, in bytes */
    sb->st_blocks = (my_dir.length + FS_BLOCK_SIZE -1) / FS_BLOCK_SIZE;        /* blocks allocated for file */
    sb->st_blksize = FS_BLOCK_SIZE;       /* optimal blocksize for I/O */
    // struct timespec st_birthtimespec; /* time of file creation(birth) */
    // uint32_t        st_flags;         /* user defined flags for file */
    // uint32_t        st_gen;           /* file generation number */
    // int32_t         st_lspare;        /* RESERVED: DO NOT USE! */
    // int64_t         st_qspare[2];     /* RESERVED: DO NOT USE! */
    return 0;
    
}


/* Note on path translation errors:
 * In addition to the method-specific errors listed below, almost
 * every method can return one of the following errors if it fails to
 * locate a file or directory corresponding to a specified path.
 *
 * ENOENT - a component of the path is not present.
 * ENOTDIR - an intermediate component of the path (e.g. 'b' in
 *           /a/b/c) is not a directory
 */

/* getattr - get file or directory attributes. For a description of
 *  the fields in 'struct stat', see 'man lstat'.
 *
 * Note - fields not provided in CS5600fs are:
 *    st_nlink - always set to 1
 *    st_atime, st_ctime - set to same value as st_mtime
 *
 * errors - path translation, ENOENT
 */

static int hw3_getattr(const char *path, struct stat *sb)
{
    // string of size block size
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    // copy the path into the newly created character array
    strcpy(dup_path, path);
    // starting block number of the root directory
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    // array of 16 directory entries
    struct cs5600fs_dirent my_dir[16];
    int dir_offset=0;
    // string of size 44
    char *dirname = (char *)calloc(dirname_size, sizeof(char));
    
    // if input path is "/" meaning the root directory
    if(strcmp(dup_path,"/") == 0)
    {
        // read the root directory into memory from the disk
        disk->ops->read(disk, root_block_start * 2, 2, (void *) my_dir);
        // setting the attributes of the file / directory
        return_code = set_stat_struct(sb, my_dir[0]);
        return return_code;
    }
    
    
    // iterating over the copy of the path
    while(strlen(dup_path) > 0)
    {
        // reading the directory entries present in the block, root_block_start
        // from the disk into the my_dir array
        disk->ops->read(disk, root_block_start * 2, 2, (void*) my_dir);
        // splitting the path
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        // finding the offset of the input directory name
        dir_offset = get_dir_offset(dirname, my_dir);
        
        //checking if no offset was found
        if(dir_offset == myCheckCode)
        {
            // return file not found error as no corresponding offset was found
            return_code = -ENOENT;
            break;
        }
        // checking if the path has been translated
        if(strlen(dup_path) == 0){
            // setting the attributes of the file / directory
            return_code = set_stat_struct(sb, my_dir[dir_offset]);
            break;
        }
        // checking to see if an intermediate file in the path is not a directory
        if((strlen(dup_path) > 0) && !my_dir[dir_offset].isDir)
        {
            // returning the not a directory error
            return_code = -ENOTDIR;
            break;
        }
        // updating the value of the root start block to the starting block
        // of the next file in the path
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
}



/* readdir - get directory contents.
 *
 * for each entry in the directory, invoke the 'filler' function,
 * which is passed as a function pointer, as follows:
 *     filler(buf, <name>, <statbuf>, 0)
 * where <statbuf> is a struct stat, just like in getattr.
 *
 * Errors - path resolution, ENOTDIR, ENOENT
 */

static int hw3_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    // string of size block size
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    // copy the path into the newly created character array
    strcpy(dup_path, path);
    // starting block number of the root directory
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    // array of 16 directory entries
    struct cs5600fs_dirent my_dir[16];
    int dir_offset = 0;
    struct stat sb;
    int i = 0;
    char *name = NULL;
    // string of size 44
    char *dirname = (char *)calloc(dirname_size, sizeof(char));
    
    // checking if the input path is "/" or empty meaning search in the root directory
    if((strcmp(dup_path,"/") == 0) || strcmp((dup_path + 1),"//") == 0)
    {
        // reading the root directory into my_dir array
        disk->ops->read(disk, root_block_start * 2, 2, (void *) my_dir);
        // iterating through the 16 entries
        for(i=0;i<16;i++){
            // checking to see if the file is valid
            if(my_dir[i].valid == 0)
                continue;
            // setting the attributes with details of the file
            sb.st_dev = 0;           /* ID of device containing file */
            sb.st_mode = my_dir[i].mode |(my_dir[i].isDir ? S_IFDIR : S_IFREG); /* Mode of file (see below) */
            sb.st_nlink = 1;         /* Number of hard links */
            sb.st_ino = 0;           /* File serial number */
            sb.st_uid = my_dir[i].uid;           /* User ID of the file */
            sb.st_gid = my_dir[i].gid;           /* Group ID of the file */
            sb.st_rdev = 0;          /* Device ID */
            sb.st_mtime = my_dir[i].mtime;     /* time of last access */
            sb.st_ctime = my_dir[i].mtime;     /* time of last data modification */
            sb.st_atime = my_dir[i].mtime;     /* time of last status change */
            sb.st_size = my_dir[i].length;          /* file size, in bytes */
            sb.st_blocks = (my_dir[i].length + FS_BLOCK_SIZE -1) / FS_BLOCK_SIZE; /* blocks allocated for file */
            sb.st_blksize = FS_BLOCK_SIZE;       /* optimal blocksize for I/O */
            name = my_dir[i].name;
            // calling the filler function
            filler(buf, name, &sb, 0); /* invoke callback function */
        }
        return return_code;
    }
    
    // iterating over the path
    while(strlen(dup_path) > 0)
    {
        // reading the directory entries present in the block, root_block_start
        // from the disk into the my_dir array
        disk->ops->read(disk, root_block_start * 2, 2, (void*) my_dir);
        // splitting the path
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        // finding the offset of the input directory name
        dir_offset = get_dir_offset(dirname, my_dir);
        
        // checking if the file offset was found
        if(dir_offset == myCheckCode)
        {
            // returning the file not found error as no offset was found
            return_code = -ENOENT;
            break;
        }
        
        // checking if the file is not a directory
        if(!my_dir[dir_offset].isDir)
        {
            // returning the not a directory error as the file is not a directory
            return_code = -ENOTDIR;
            break;
        }
        
        // iterating over the path
        if(strlen(dup_path) == 0){
            // setting the block number to the starting block number of the file whose offset was found
            root_block_start = my_dir[dir_offset].start;
            // reading the directory entries present in the block, root_block_start
            // from the disk into the my_dir array
            disk->ops->read(disk, root_block_start * 2, 2, (void *) my_dir);
            // iterating over the entries in the directory
            for(i=0;i<16;i++){
                // checking to see if the file / directory is valid
                if(my_dir[i].valid == 0)
                    continue;
                // setting the attributes with file details
                sb.st_dev = 0;           /* ID of device containing file */
                sb.st_mode = my_dir[i].mode|(my_dir[i].isDir ? S_IFDIR : S_IFREG); /* Mode of file (see below) */
                sb.st_nlink = 1;         /* Number of hard links */
                sb.st_ino = 0;           /* File serial number */
                sb.st_uid = my_dir[i].uid;           /* User ID of the file */
                sb.st_gid = my_dir[i].gid;           /* Group ID of the file */
                sb.st_rdev = 0;          /* Device ID */
                sb.st_mtime = my_dir[i].mtime;     /* time of last access */
                sb.st_ctime = my_dir[i].mtime;     /* time of last data modification */
                sb.st_atime = my_dir[i].mtime;     /* time of last status change */
                sb.st_size = my_dir[i].length;          /* file size, in bytes */
                sb.st_blocks = (my_dir[i].length + FS_BLOCK_SIZE - 1)/ FS_BLOCK_SIZE; /* blocks allocated for file */
                sb.st_blksize = FS_BLOCK_SIZE;       /* optimal blocksize for I/O */
                name = my_dir[i].name;
                // calling the filler function
                filler(buf, name, &sb, 0); /* invoke callback function */
            }
            return return_code;
        }
        // updating the block number to the starting block number of the
        // file whose offset is found
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
    
}


// helper function to create a file
int create_file(unsigned int block_num,
                struct cs5600fs_dirent *my_dir, char *dirname, mode_t mode)
{
	if(strlen(dirname) > dirname_chk)
		return -EINVAL;

    int i = 0;
    
    // iterating through the 16 entries of the input directory
    for( i = 0; i < 16; i++){
        // checking to see if the file is not valid
        if(!my_dir[i].valid){
            break;
        }
    }
    
    // checking if the iterator is >= 16 indicating no free block was found
    if(i >= 16){
        // returning the no space error
        return -ENOSPC;
    }
    
    // making the free block valid
    my_dir[i].valid = 1;
    // setting isDir to 0 as a file is to be created
    my_dir[i].isDir = 0;
    // setting the attributes the same as those of the root directory
    my_dir[i].pad = my_root_dir->pad;
    my_dir[i].uid = getuid();
    my_dir[i].gid = getgid();
    // setting the mode
    my_dir[i].mode = mode & 01777;
    // setting the modification time to current time
    my_dir[i].mtime = time(NULL);
    // setting the file length to 0
    my_dir[i].length = 0;
    // setting the name of the file
    strcpy(my_dir[i].name, dirname);
    
    int fat_block = 0;
    // iterating through the FAT in search of a free block
    while(fat_block < my_super_block->fs_size && my_FAT[fat_block].inUse){
        fat_block++;
    }
    
    // if no free block found
    if(fat_block >= my_super_block->fs_size)
        // return no space error
        return -ENOSPC;
    
    // setting the start block number of the file to be created to the newly
    // allocated block
    my_dir[i].start = fat_block;
    
    // writing the changes in the directory entries to the disk
    disk->ops->write(disk, block_num * 2, 2, (void *) my_dir);
    
    // setting the free block found to being in use
    my_FAT[fat_block].inUse = 1;
    // setting the end of file to true
    my_FAT[fat_block].eof = 1;
    // setting the next block to 0 as only one block is to be allocated
    my_FAT[fat_block].next = 0;
    
    // writing the changes made to the FAT to the disk
    disk->ops->write(disk, 2, my_super_block->fat_len * 2, (void *)my_FAT);
    
    return 0;
}


// function to check if the file given in the input path already exists
int check_file_exists(const char *path)
{
    // string of size block size
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    // copy the path into the above string
    strcpy(dup_path, path);
    // set the block number to the start of the root directory
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 1;
    // array of a directory with 16 entries
    struct cs5600fs_dirent my_dir[16];
    int dir_offset=0;
    // string of size 44 for directory name
    char *dirname = (char *)calloc(dirname_size, sizeof(char));
    
    // iterating over the path
    while(strlen(dup_path) > 0){
        // reading the directory entries present in the block, root_block_start
        // from the disk into the my_dir array
        disk->ops->read(disk, root_block_start * 2, 2, (void *) my_dir);
        // splitting the path
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        // finding the offset of the input directory name
        dir_offset = get_dir_offset(dirname, my_dir);
        
        // checking if an offset was found
        if(dir_offset == myCheckCode){
            return  0;
        }
        // updating the block number to the starting block number of the
        // file whose offset is found
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
}

// function to check if the file given in the input path already exists
int check_input_file_exists(const char *path)
{
    // string of size block size
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    // copy the path into the above string
    strcpy(dup_path, path);
    // set the block number to the start of the root directory
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 1;
    // array of a directory with 16 entries
    struct cs5600fs_dirent my_dir[16];
    int dir_offset=0;
    // string of size 44 for directory name
    char *dirname = (char *)calloc(dirname_size, sizeof(char));
    
    // iterating over the path
    while(strlen(dup_path) > 0){
        // reading the directory entries present in the block, root_block_start
        // from the disk into the my_dir array
        disk->ops->read(disk, root_block_start * 2, 2, (void *) my_dir);
        // splitting the path
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        // finding the offset of the input directory name
        dir_offset = get_dir_offset(dirname, my_dir);
        
        // checking if an offset was found
        if(dir_offset == myCheckCode){
            return  0;
        }
        // updating the block number to the starting block number of the
        // file whose offset is found
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
}

/* create - create a new file with permissions (mode & 01777)
 *
 * Errors - path resolution, EEXIST
 *
 * If a file or directory of this name already exists, return -EEXIST.
 */
static int hw3_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi)
{
    // string of size block size
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    // copy the path into the above string
    strcpy(dup_path, path);
    // duplicate path
    char *duplicate_path = calloc(1, FS_BLOCK_SIZE);
    // copying the input path into the duplicate string
    strcpy(duplicate_path, path);
    // set the block number to the start of the root directory
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    // array of a directory with 16 entries
    struct cs5600fs_dirent my_dir[16];
    int dir_offset = 0;
    // string of size 44 for directory name
    char *dirname = (char *)calloc(dirname_size, sizeof(char));
    
    // check if the file in the input path exists
    int check_return = check_input_file_exists(duplicate_path);
    
    // if file exists
    if(check_return == 1)
        // return file already exists error
        return -EEXIST;
    
    // iterating over the path
    while(strlen(dup_path) > 0)
    {
        // reading the directory entries present in the block, root_block_start
        // from the disk into the my_dir array
        disk->ops->read(disk, root_block_start * 2, 2, (void *) my_dir);
        // splitting the path
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        // finding the offset of the input directory name
        dir_offset = get_dir_offset(dirname, my_dir);
        
        // if the offset is not found and path has not been translated yet
        if(dir_offset == myCheckCode && strlen(dup_path) > 0)
        {
            // return file not found error
            return_code = -ENOENT;
            return return_code;
        }
        
        // if path is in the process of being translated and the current file is not
        // a directory
        if((strlen(dup_path) > 0) && !my_dir[dir_offset].isDir)
        {
            // return file not a directory error
            return_code = -ENOTDIR;
            return return_code;
        }
        
        // checking if path has been translated
        if(strlen(dup_path) == 0){
            // creating a file with the input mode
            return_code = create_file(root_block_start, my_dir, dirname, mode);
            return return_code;
        }
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
}


// function to create a directory
int create_dir(unsigned int block_num, struct cs5600fs_dirent *my_dir,
               char *dirname, mode_t mode)
{
    int i = 0;
    if(strlen(dirname) > dirname_chk)
    	return -EINVAL;

    // iterating through the 16 entries of the input directory
    for( i = 0; i < 16; i++)
    {
        // checking if the directory is valid
        if(!my_dir[i].valid)
        {
            break;
        }
    }
    
    // checking if offset is >= 16 implying there is no free block
    if(i >= 16)
    {
        // giving the no space error
        return -ENOSPC;
    }
    
    // setting the attributes of the created directory
    my_dir[i].valid = 1;
    // setting isDir to true as a directory is being created
    my_dir[i].isDir = 1;
    // setting the attributes of the file same as those of the root directory
    my_dir[i].pad = my_root_dir->pad;
    my_dir[i].uid = getuid();
    my_dir[i].gid = getgid();
    my_dir[i].mode = mode;
    // setting the modification time to the current time
    my_dir[i].mtime = time(NULL);
    // setting the length of empty directory to 0
    my_dir[i].length = 0;
    // setting the name of the directory
    strcpy(my_dir[i].name, dirname);
    
    int fat_block = 0;
    
    // iterating over the FAT checking for a free block
    while(fat_block < my_super_block->fs_size && my_FAT[fat_block].inUse)
    {
        fat_block++;
    }
    
    // return space error if no free block found
    if(fat_block >= my_super_block->fs_size)
        return -ENOSPC;	
    
    // setting the starting block number of the created directory
    my_dir[i].start = fat_block;
    
    // writing the changes made onto the disk
    disk->ops->write(disk, block_num * 2, 2, (void *) my_dir);
    
    // updating the FAT of the free block found
    my_FAT[fat_block].inUse = 1;
    my_FAT[fat_block].eof = 1;
    my_FAT[fat_block].next = 0;
    
    // writing the updates of the FAT onto the disk
    disk->ops->write(disk, 2, my_super_block->fat_len * 2, (void *)my_FAT);
    return 0;
}


/* mkdir - create a directory with the given mode.
 * Errors - path resolution, EEXIST
 * Conditions for EEXIST are the same as for create.
 */
static int hw3_mkdir(const char *path, mode_t mode)
{
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_path, path);
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    struct cs5600fs_dirent my_dir[16];
    int dir_offset = 0;
    char *dirname = (char *)calloc(dirname_size, sizeof(char));
    
    // iterating over the path
    while(strlen(dup_path) > 0)
    {
        // reading the directory from the disk
        disk->ops->read(disk, root_block_start * 2, 2, (void *) my_dir);
        // splitting the path
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        // finding the offset of the file in the parent directory
        dir_offset = get_dir_offset(dirname, my_dir);
        
        // checking if file offset found and path translation in progress
        if(dir_offset == myCheckCode && strlen(dup_path) > 0)
        {
            return_code = -ENOENT;
            return return_code;
        }
        
        // checking if the file found and path translation has been completed
        if(dir_offset != myCheckCode && strlen(dup_path) == 0)
        {
            return_code = -EEXIST;
            break;
        }
        
        // checking if file not directory and path is being translated
        if((strlen(dup_path) > 0) && !my_dir[dir_offset].isDir)
        {
            return_code = -ENOTDIR;
            return return_code;
        }
        
        // checking if file not found and path translation has been completed
        if(strlen(dup_path) == 0 && dir_offset == myCheckCode){
            // creating the directory
            return_code = create_dir(root_block_start, my_dir, dirname, mode);
            return return_code;
        }
        // updating the block with the start block number of the new file
        root_block_start = my_dir[dir_offset].start;
    }
    return -EEXIST;
}


// function to remove the file
int remove_file(unsigned int root_block_start, struct cs5600fs_dirent *my_dir,
                char *dirname){
    
    int fat_block = 0;
    int i = 0;
    
    // iterating over the entries of the parent directory
    for(i=0; i<16; i++){
        // checking for validity of file and same name
        if(my_dir[i].valid && strcmp(my_dir[i].name, dirname) == 0){
            // making file invalid
            my_dir[i].valid = 0;
            fat_block = my_dir[i].start;
        }
    }
    
    while(1)
    {
        // check if file has ended
        if(my_FAT[fat_block].eof == 1)
        {
            // check if file is invalid
            my_FAT[fat_block].inUse = 0;
            my_FAT[fat_block].next = 0;
            break;
        }
        // make file invalid
        my_FAT[fat_block].inUse=0;
        // update the block number
        fat_block =  my_FAT[fat_block].next;
        my_FAT[fat_block].next = 0;
    }
    
    // write the updated directory and FAT entries onto the disk
    disk->ops->write(disk,root_block_start*2,2,(void *)my_dir);
    disk->ops->write(disk,2,my_super_block->fat_len * 2,(void *)my_FAT);
    return 0;
}

/* unlink - delete a file
 *  Errors - path resolution, ENOENT, EISDIR
 */
static int hw3_unlink(const char *path)
{
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_path, path);
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    struct cs5600fs_dirent my_dir[16];
    int dir_offset=0;
    char *dirname = (char *)calloc(dirname_size, sizeof(char));
    
    // iterating over the path
    while(strlen(dup_path) > 0)
    {
        // read the block number into the directory entries array
        disk->ops->read(disk, root_block_start * 2, 2, (void *) my_dir);
        // splitting the path
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        // finding the offset of the input file
        dir_offset = get_dir_offset(dirname, my_dir);
        
        // checking if file offset found
        if(dir_offset == myCheckCode)
        {
            return  -ENOENT;
        }
        
        // checking if path translation in progress and file not a directory
        if((strlen(dup_path) > 0) && !my_dir[dir_offset].isDir)
        {
            return_code = -ENOTDIR;
            return return_code;
        }
        
        // checking if path translation completed and file is a directory
        if((strlen(dup_path) == 0) && my_dir[dir_offset].isDir)
        {
            return_code = -EISDIR;
            return return_code;
        }
        
        // checking path translation completed
        if(strlen(dup_path) == 0){
            // remove the file
            return_code = remove_file(root_block_start, my_dir, dirname);
            return return_code;
        }
        // updating the block number to the start block number of the newly
        // found file
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
}


// function to remove the directory
int remove_dir(unsigned int root_block_start, struct cs5600fs_dirent *my_dir,
               char *dirname){
    
    int fat_block = 0;
    int i = 0;
    struct cs5600fs_dirent to_remove_dir[16];
    
    // iterating over the entries of the directory
    for(i=0; i<16; i++){
        // checking if file valid and its name is same as the input file name
        if(my_dir[i].valid && strcmp(my_dir[i].name, dirname) == 0){
            // making file invalid
            my_dir[i].valid = 0;
            fat_block = my_dir[i].start;
            // reading the directory from the disk
            disk->ops->read(disk,fat_block*2,2,(void *)to_remove_dir);
        }
    }
    
    // iterating over the entries in the directory
    for(i =0; i<16; i++){
        // checking if file is valid
        if(to_remove_dir[i].valid)
            return -ENOTEMPTY;
    }
    
    while(1){
        // checking if end of file reached
        if(my_FAT[fat_block].eof == 1){
            // making file invalid
            my_FAT[fat_block].inUse = 0;
            my_FAT[fat_block].next = 0;
            break;
        }
        // making file invalid
        my_FAT[fat_block].inUse=0;
        // updating the block number
        fat_block =  my_FAT[fat_block].next;
        my_FAT[fat_block].next = 0;
    }
    
    // writing the changes made onto the directory and FAT onto the disk
    disk->ops->write(disk,root_block_start*2,2,(void *)my_dir);
    disk->ops->write(disk,2,my_super_block->fat_len * 2,(void *)my_FAT);
    return 0;
}


/* rmdir - remove a directory
 *  Errors - path resolution, ENOENT, ENOTDIR, ENOTEMPTY
 */
static int hw3_rmdir(const char *path)
{
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_path, path);
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    struct cs5600fs_dirent my_dir[16];
    int dir_offset=0;
    char *dirname = (char *)calloc(dirname_size, sizeof(char));
    
    // iterating over the path
    while(strlen(dup_path) > 0)
    {
        // read the block into the directory entries array
        disk->ops->read(disk, root_block_start * 2, 2, (void *) my_dir);
        // splitting the path
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        // find the directory offset
        dir_offset = get_dir_offset(dirname, my_dir);
        
        // intermediate directory not found
        if(dir_offset == myCheckCode && strlen(dup_path) > 0)
        {
            return  -ENOENT;
        }
        
        // intermediate file not a directory
        if((strlen(dup_path) > 0) && !my_dir[dir_offset].isDir)
        {
            return_code = -ENOTDIR;
            return return_code;
        }
        
        if(strlen(dup_path) == 0){
            // removing the directory
            return_code = remove_dir(root_block_start, my_dir, dirname);
            return return_code;
        }
        // updating the block number to start block of the newly
        // found file
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
}


int same_dir(char *src_path, char *dst_path){
    
    char *dup_spath = calloc(1, FS_BLOCK_SIZE);
    char *dup_spath2 = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_spath, src_path);
    strcpy(dup_spath2, src_path);
    
    char *sparent = calloc(1, FS_BLOCK_SIZE);
    char *dparent = calloc(1, FS_BLOCK_SIZE);
    
    char *dup_dpath = calloc(1, FS_BLOCK_SIZE);
    char *dup_dpath2 = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_dpath, dst_path);
    strcpy(dup_dpath2, dst_path);
    
    unsigned int root_block_start1 = my_root_dir->start;
    unsigned int root_block_start2 = my_root_dir->start;
    
    int len1, len2 = 0;
    struct cs5600fs_dirent my_dir1[16];
    struct cs5600fs_dirent my_dir2[16];
    int dir_offset1=0;
    int dir_offset2=0;
    
    char dirname1[dirname_size];
    char dirname2[dirname_size];
    
    // iterating over the path
    while(strlen(dup_spath) > 0)
    {
        memset(dirname1,0,dirname_size);
        // reading the block into directory entries
        disk->ops->read(disk, root_block_start1 * 2, 2, (void *) my_dir1);
        // splitting the path
        dup_spath = strwrd(dup_spath, dirname1, dirname_size, "/");
        // finding the offset of the file
        dir_offset1 = get_dir_offset(dirname1, my_dir1);
        
        // offset not found
        if(dir_offset1 == myCheckCode)
        {
            return  -ENOENT;
        }
        
        if(strlen(dup_spath) == 0){
            len1 = strlen(dup_spath2)-strlen(dirname1);
            break;
        }
        // updating block number
        root_block_start1 = my_dir1[dir_offset1].start;
    }
    
    strncpy(sparent,dup_spath2,len1);
    
    // iterating over the path
    while(strlen(dup_dpath) > 0)
    {
        memset(dirname1,0,dirname_size);
        // read the block number into directory entries array from the disk
        disk->ops->read(disk, root_block_start2 * 2, 2, (void *) my_dir2);
        // splitting the path
        dup_dpath = strwrd(dup_dpath, dirname2, dirname_size, "/");
        // finding offset of the file
        dir_offset2 = get_dir_offset(dirname2, my_dir2);
        
        // intermediate file's offset not found
        if(dir_offset2 == myCheckCode && strlen(dup_dpath) > 0)
        {
            return  -ENOENT;
        }
        
        if(strlen(dup_dpath) == 0){
            len2 = strlen(dup_dpath2)-strlen(dirname2);
            break;
        }
        // updating the block number
        root_block_start2 = my_dir2[dir_offset2].start;
    }
    
    strncpy(dparent,dup_dpath2,len2);
    
    if(strcmp(sparent,dparent) == 0)
        return 1;
    
    return 0;
}

// function to rename file
int rename_file(unsigned int root_block_start, struct cs5600fs_dirent *my_dir, char *dirname,
                char *dst_name){
    int i = 0;
    
    if(strlen(dirname) > dirname_chk)
    	return -EINVAL;

    // iterating over input directory entries
    for(i=0; i<16; i++){
        // file valid and it's name same as input file name
        if(my_dir[i].valid && strcmp(my_dir[i].name, dirname) == 0){
            // rename file
            strcpy(my_dir[i].name, dst_name);
            // set modification time
            my_dir[i].mtime = time(NULL);
        }
    }
    // write the changes onto the disk
    disk->ops->write(disk, root_block_start * 2, 2, (void*)my_dir);
    return 0;
}

/* rename - rename a file or directory
 * Errors - path resolution, ENOENT, EINVAL, EEXIST
 *
 * ENOENT - source does not exist
 * EEXIST - destination already exists
 * EINVAL - source and destination are not in the same directory
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 */
static int hw3_rename(const char *src_path, const char *dst_path){
    
    char *dup_spath = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_spath, src_path);
    
    char *dup_dpath = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_dpath, dst_path);
    
    unsigned int root_block_start = my_root_dir->start;
    struct cs5600fs_dirent my_dir[16];
    int return_code = 0;
    
    int dir_offset = 0;
    
    char src_name[dirname_size];
    char dst_name[dirname_size];
    
    // checking files in same parent directory
    if(!same_dir(dup_spath, dup_dpath)){
        return -EINVAL;
    }
    
    // checking if file with same name already exists
    if(check_file_exists(dup_dpath)==1){
        return -EEXIST;
    }
    
    // checking source and destination exist
    if(same_dir(dup_spath, dup_dpath) == -ENOENT){
        return -ENOENT;
    }
    
    // iterating over the path
    while(strlen(dup_dpath) > 0){
        memset(dst_name,0,dirname_size);
        // splitting path
        dup_dpath = strwrd(dup_dpath, dst_name, dirname_size, "/");
        // finding file offset
        dir_offset = get_dir_offset(dst_name, my_dir);
    }
    dir_offset = 0;
    
    // iterating over path
    while(strlen(dup_spath) > 0)
    {
        memset(src_name,0,dirname_size);
        // reading the directory from the disk
        disk->ops->read(disk, root_block_start * 2, 2, (void *) my_dir);
        // splitting the path
        dup_spath = strwrd(dup_spath, src_name, dirname_size, "/");
        // finding the offset of the file in the parent directory
        dir_offset = get_dir_offset(src_name, my_dir);
        
        // checking if file offset found
        if(dir_offset == myCheckCode)
        {
            return  -ENOENT;
        }
        
        if(strlen(dup_spath) == 0){
            // rename file
            return_code = rename_file(root_block_start, my_dir, src_name, dst_name);
            return return_code;
        }
        // updating block
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
}

// function to set permissions of the input file
int set_permission(unsigned int block_num, struct cs5600fs_dirent *my_dir,
                   char *dirname, mode_t mode){
    int i = 0;
    // iterating over the entries in the parent directory
    for(i=0; i<16; i++){
        // file valid and file's name same as input file name
        if(my_dir[i].valid && strcmp(my_dir[i].name, dirname) == 0){
            // updating mode
            my_dir[i].mode = mode;
            my_dir[i].mtime = time(NULL);
        }
    }
    // writing changes to directory onto disk
    disk->ops->write(disk,block_num*2,2,(void*)my_dir);
    return 0;
}

// function to set the utime of the file
int set_utime(unsigned int block_num, struct cs5600fs_dirent *my_dir,
              char *dirname, struct utimbuf *ut){
    int i = 0;
    // iterating over the entries in the parent directory
    for(i=0; i<16; i++){
        // file valid and file's name same as input file name
        if(my_dir[i].valid && strcmp(my_dir[i].name, dirname) == 0){
            // updating its modification time
            my_dir[i].mtime = ut->modtime;
        }
    }
    // writing changes to directory onto disk
    disk->ops->write(disk,block_num*2,2,(void*)my_dir);
    return 0;
}
/* chmod - change file permissions
 * utime - change access and modification times
 *         (for definition of 'struct utimebuf', see 'man utime')
 *
 * Errors - path resolution, ENOENT.
 */
static int hw3_chmod(const char *path, mode_t mode)
{
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_path, path);
    char dirname[dirname_size];
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    struct cs5600fs_dirent my_dir[16];
    int dir_offset=0;
    
    while(strlen(dup_path) > 0)
    {
        memset(dirname,0,dirname_size);
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        disk->ops->read(disk, root_block_start * 2, 2, (void*) my_dir);
        dir_offset = get_dir_offset(dirname, my_dir);
        
        if(dir_offset == myCheckCode)
        {
            return  -ENOENT;
        }
        
        if(strlen(dup_path) == 0){
            // changing permission
            return_code = set_permission(root_block_start, my_dir, dirname, mode);
            return return_code;
        }
        // updating the block number
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
    
}


int hw3_utime(const char *path, struct utimbuf *ut)
{
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_path, path);
    char dirname[dirname_size];
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    struct cs5600fs_dirent my_dir[16];
    int dir_offset=0;
    
    while(strlen(dup_path) > 0)
    {
        memset(dirname,0,dirname_size);
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        disk->ops->read(disk, root_block_start * 2, 2, (void*) my_dir);
        dir_offset = get_dir_offset(dirname, my_dir);
        
        // checking file exists
        if(dir_offset == myCheckCode)
        {
            return  -ENOENT;
        }
        
        if(strlen(dup_path) == 0){
            // updating file's utime
            return_code = set_utime(root_block_start, my_dir, dirname, ut);
            return return_code;
        }
        // updating the block number
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
}

// function to truncate
int truncate_parent(unsigned int block_num, struct cs5600fs_dirent *my_dir,
                    char *dirname){
    int i ,fat_block ,current = 0;
    
    // iterating over the 16 entries
    for(i=0; i<16; i++){
        // check file is valid and its name is same as input name
        if(my_dir[i].valid && strcmp(my_dir[i].name, dirname) == 0){
            // check if file is a directory
            if(my_dir[i].isDir)
                return -EISDIR;
            my_dir[i].length = 0;
            // update modification time
            my_dir[i].mtime = time(NULL);
            fat_block = my_dir[i].start;
        }
    }
    // write the changes onto the disk
    disk->ops->write(disk,block_num*2,2,(void*)my_dir);
    
    current = my_FAT[fat_block].next;
    
    // check if file's end has reached
    if(my_FAT[fat_block].eof){
        my_FAT[fat_block].inUse = 1;
        my_FAT[fat_block].next = 0;
    }
    else{
        my_FAT[fat_block].eof = 1;
        my_FAT[fat_block].inUse = 1;
        my_FAT[fat_block].next = 0;
        // loop until end of file reached
        while(!my_FAT[current].eof){
            my_FAT[current].inUse = 0;
            current = my_FAT[current].next;
        }
        my_FAT[current].inUse = 0;
        my_FAT[current].next = 0;
    }
    // write the changes onto the disk
    disk->ops->write(disk, 2, my_super_block->fat_len * 2, (void *)my_FAT);
    return 0;
}

/* truncate - truncate file to exactly 'len' bytes
 * Errors - path resolution, ENOENT, EISDIR, EINVAL
 *    return EINVAL if len > 0.
 */
static int hw3_truncate(const char *path, off_t len)
{
    /* you can cheat by only implementing this for the case of len==0,
     * and an error otherwise.
     */
    if (len != 0)
        return -EINVAL;     /* invalid argument */
    
    if (len > 0)
        return -EINVAL;
    
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_path, path);
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    struct cs5600fs_dirent my_dir[16];
    int dir_offset=0;
    char *dirname = (char *)calloc(dirname_size, sizeof(char));
    
    while(strlen(dup_path) > 0)
    {
        // split the path
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        // read the directory from the disk
        disk->ops->read(disk, root_block_start * 2, 2, (void*) my_dir);
        // find the file offset
        dir_offset = get_dir_offset(dirname, my_dir);
        
        if(dir_offset == myCheckCode)
        {
            return  -ENOENT;
        }
        
        if(strlen(dup_path) == 0){
            // truncate file's parent
            return truncate_parent(root_block_start, my_dir, dirname);
        }
        // update the block number
        root_block_start = my_dir[dir_offset].start;
    }
    return return_code;
}


// function to get the start block for read
int get_start_block(off_t offset, unsigned int block_num){
    
    int offset_in_block = 0;
    int i = 0;
    
    offset_in_block = offset / FS_BLOCK_SIZE;
    
    // iterate till you reach the block you wish to read / write on
    for( i = 0; i < offset_in_block; i++){
        // update block number to next block
        block_num = my_FAT[block_num].next;
    }
    return block_num;
}

/* read - read data from an open file.
 * should return exactly the number of bytes requested, except:
 *   - if offset >= len, return 0
 *   - on error, return <0
 * Errors - path resolution, ENOENT, EISDIR
 */
static int hw3_read(const char *path, char *buf, size_t len, off_t offset,
                    struct fuse_file_info *fi)
{
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_path, path);
    char dirname[dirname_size];
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    struct cs5600fs_dirent my_dir[16];
    int dir_offset=0;
    
    while(strlen(dup_path) > 0)
    {
        memset(dirname,0,dirname_size);
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        disk->ops->read(disk, root_block_start * 2, 2, (void*) my_dir);
        dir_offset = get_dir_offset(dirname, my_dir);
        
        if(dir_offset == myCheckCode)
        {
            return  -ENOENT;
        }
        
        if(!my_dir[dir_offset].isDir && strlen(dup_path) > 0)
        {
            return_code = -ENOTDIR;
            return return_code;
        }
        
        if(my_dir[dir_offset].isDir && strlen(dup_path) == 0)
        {
            return_code = -EISDIR;
            return return_code;
        }
        
        root_block_start = my_dir[dir_offset].start;
    }
    
    if(my_dir[dir_offset].length <= offset)
        return 0;
    
    unsigned int start_block_addr = 0;
    int blocks_to_read = 0;
    int dir_length = my_dir[dir_offset].length;
    int offset_in_file = offset % FS_BLOCK_SIZE;
    int offset_in_blocks = offset / FS_BLOCK_SIZE;
    int bytes_from_start = len + offset;
    
    start_block_addr = get_start_block(offset, my_dir[dir_offset].start);
    
    // update length if bytes to be read are larger than length of the file
    if (dir_length < bytes_from_start)
    {
        len = dir_length - offset;
    }
    
    int blocks_spanned = bytes_from_start / FS_BLOCK_SIZE;
    blocks_to_read = blocks_spanned - offset_in_blocks + 1;
    char *temp_buf = calloc(blocks_to_read * FS_BLOCK_SIZE, 1);
    int i = 0;
    int j = start_block_addr;
    
    // reading the blocks
    for(i=0; i<blocks_to_read; i++){
        // read the block from the disk
        disk->ops->read(disk, j * 2, 2, temp_buf+(i*FS_BLOCK_SIZE));
        j = my_FAT[j].next;
    }
    
    // copy the data read into the temporary buffer into input buffer
    memcpy(buf, temp_buf + offset_in_file , len);
    return len;
}

// function to check length of directory
int check_dir_length(struct cs5600fs_dirent *my_dir, char *dirname, off_t offset){
    int i = 0;
    
    for(i=0; i<16; i++){
        // file is valid and its name is same as the input name
        if(my_dir[i].valid && strcmp(my_dir[i].name, dirname) == 0){
            // offset greater then length of file
            if(offset > my_dir[i].length)
                return -EINVAL;
        }
    }
    return 0;
}


/* write - write data to a file
 * It should return exactly the number of bytes requested, except on
 * error.
 * Errors - path resolution, ENOENT, EISDIR
 *  return EINVAL if 'offset' is greater than current file length.
 */
static int hw3_write(const char *path, const char *buf, size_t len,
                     off_t offset, struct fuse_file_info *fi){
    
    
    char *dup_path = calloc(1, FS_BLOCK_SIZE);
    strcpy(dup_path, path);
    char dirname[dirname_size];
    unsigned int root_block_start = my_root_dir->start;
    int return_code = 0;
    struct cs5600fs_dirent my_dir[16];
    int dir_offset=0;
    int parent_block = 0;
    
    while(strlen(dup_path) > 0 && strcmp(path, "/") != 0)
    {
        memset(dirname,0,dirname_size);
        dup_path = strwrd(dup_path, dirname, dirname_size, "/");
        disk->ops->read(disk, root_block_start * 2, 2, (void*) my_dir);
        dir_offset = get_dir_offset(dirname, my_dir);
        
        if(dir_offset == myCheckCode)
        {
            return  -ENOENT;
        }
        
        if(!my_dir[dir_offset].isDir && strlen(dup_path) > 0)
        {
            return_code = -ENOTDIR;
            return return_code;
        }
        
        if(my_dir[dir_offset].isDir && strlen(dup_path) == 0)
        {
            return_code = -EISDIR;
            return return_code;
        }
        
        if(strlen(dup_path) == 0){
            parent_block = root_block_start;
        }
        
        root_block_start = my_dir[dir_offset].start;
    }
    
    // offset greater than length of the file
    if(offset > my_dir[dir_offset].length)
        return -EINVAL;
    
    unsigned int start_block_addr = 0;
    int blocks_to_read = 0;
    int offset_in_file = offset % FS_BLOCK_SIZE;
    int offset_in_blocks = offset / FS_BLOCK_SIZE;
    
    start_block_addr = get_start_block(offset, my_dir[dir_offset].start);
    
    int bytes_from_start = len + offset;
    
    int blocks_spanned = bytes_from_start / FS_BLOCK_SIZE;
    blocks_to_read = blocks_spanned - offset_in_blocks + 1;
    
    char *temp_buf = calloc(blocks_to_read * FS_BLOCK_SIZE, 1);
    
    int i = 0;
    int block_num = start_block_addr;
    int j = 0;
    
    // read the blocks into the temporary buffer
    for(i=0; i<blocks_to_read; i++){
        // file's end reached
        while(!my_FAT[block_num].eof){
            // read block from disk into temporary buffer
            disk->ops->read(disk, block_num * 2, 2, temp_buf+(i*FS_BLOCK_SIZE));
            block_num = my_FAT[block_num].next;
        }
        disk->ops->read(disk, block_num * 2, 2, temp_buf+(i*FS_BLOCK_SIZE));
    }
    
    // copy the data in buffer into the temporary buffer
    memcpy(temp_buf + offset_in_file , buf, len);
    
    i = 0;
    int seen_eof = 0;
    block_num = start_block_addr;
    int offset_in_buf = 0;
    
    // iterating over the blocks to read
    for(i = 0; i < blocks_to_read; i++){
        offset_in_buf = i * FS_BLOCK_SIZE;
        disk->ops->write(disk, block_num * 2, 2, temp_buf + offset_in_buf);
        
        // if end of file reached and not end of file not seen before
        if(my_FAT[block_num].eof && !seen_eof){
            seen_eof = 1;
            my_FAT[block_num].eof = 0;
        }
        
        // current block is not the last block and file's end has not yet
        // been seen or reached
        if(!my_FAT[block_num].eof && seen_eof == 0 && i < blocks_to_read - 1)
            block_num = my_FAT[block_num].next;
        
        // block not the last block to write
        if(i < blocks_to_read - 1){
            // file has already ended
            if(seen_eof){
                // searching for free blocks
                for(j = 0; j < my_super_block->fs_size; j++){
                    if(!my_FAT[j].inUse)
                        break;
                }
                if(j >= my_super_block->fs_size)
                    // no free block found
                    return -ENOSPC;
                else{
                    // updating values of the FAT for the newly allocated block
                    my_FAT[j].inUse = 1;
                    my_FAT[j].next = 0;
                    my_FAT[j].eof = 0;
                    my_FAT[block_num].next = j;
                    // updating block number
                    block_num = j;
                }
            }
        }
    }
    
    // setting end of file for the last block written
    my_FAT[block_num].eof = 1;
    
    int k = 0;
    for(k=0; k<16; k++){
        // file valid and it's name is same as the directory name
        if(my_dir[k].valid && strcmp(my_dir[k].name, dirname) == 0){
            // bytes written beyond original eof
            if(seen_eof)
                // update length
                my_dir[k].length = len + offset;
            my_dir[k].mtime = time(NULL);
        }
    }
    
    // write the changes made in the directory onto the disk
    disk->ops->write(disk, parent_block * 2, 2, (void*)my_dir);
    // write the changes made in FAT onto the disk
    disk->ops->write(disk,2,my_super_block->fat_len * 2,(void *)my_FAT);
    
    return len;
}

/* statfs - get file system statistics
 * see 'man 2 statfs' for description of 'struct statvfs'.
 * Errors - none. Needs to work.
 */
static int hw3_statfs(const char *path, struct statvfs *st)
{
    // needs to return the following fields (set others to zero):
    
    // *
    // * it's OK to calculate this dynamically on the rare occasions
    // * when this function is called.
    int i, blocks_used ,return_code = 0;
    
    for(i=0;i<FS_BLOCK_SIZE;i++){
        if(my_FAT[i].inUse == 1){
            blocks_used = blocks_used + 1;
        }
    }
    
    st->f_bsize   = my_super_block->blk_size;
    st->f_blocks  = my_super_block->fs_size - (1 + my_super_block->fat_len);
    st->f_bfree   = st->f_blocks - blocks_used;
    st->f_bavail  = st->f_bfree;
    st->f_namemax = 43;
    st->f_ffree   = 0;
    st->f_files   = 0;
    st->f_flag    = 0;
    st->f_fsid    = 0;
    st->f_frsize  = 0;
    
    return return_code;
}

/* operations vector. Please don't rename it, as the skeleton code in
 * misc.c assumes it is named 'hw3_ops'.
 */
struct fuse_operations hw3_ops = {
    .init = hw3_init,
    .getattr = hw3_getattr,
    .readdir = hw3_readdir,
    .create = hw3_create,
    .mkdir = hw3_mkdir,
    .unlink = hw3_unlink,
    .rmdir = hw3_rmdir,
    .rename = hw3_rename,
    .chmod = hw3_chmod,
    .utime = hw3_utime,
    .truncate = hw3_truncate,
    .read = hw3_read,
    .write = hw3_write,
    .statfs = hw3_statfs,
};
