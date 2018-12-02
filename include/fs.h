#ifndef FS_H
#define FS_H

/* Modes for file creation*/ 
#define O_CREAT 11 

/* Flags of file*/ 
#define O_RDONLY 0 
#define O_WRONLY 1 
#define O_RDWR 2

#define FILENAMELEN 32
#define INODEBLOCKS 12 
#define INODEDIRECTBLOCKS (INODEBLOCKS - 2) 
#define DIRECTORY_SIZE 16 

#define MDEV_BLOCK_SIZE 512
#define MDEV_NUM_BLOCKS 512
#define DEFAULT_NUM_INODES (MDEV_NUM_BLOCKS / 4)

#define INODE_TYPE_FILE 1
#define INODE_TYPE_DIR 2

/* Structure of inode*/
struct inode {
  int id;
  short int type; // file or to a directory
  // If the type of the inode is a directory, then the data blocks of the inode contain a directory structure ( struct directory)
  short int nlink;
  int device;
  int size; // how many bytes the file contains
  int blocks[INODEBLOCKS]; // ?! big files ? // If the type is a file, the data for a file is stored in the block numbers indicated in the blocks array of the inode. 
  //The index of the block in the array indicates the position of the block in the file. 
};

/*File states to check if file is open or closed*/
#define FSTATE_CLOSED 0
#define FSTATE_OPEN 1

/*Struct to store file details like state, fileptr*/
struct filetable {
  int state;
  int fileptr;
  struct dirent *de;
  struct inode in;
  int mode; // added extra to track mode of file
};

/*Struct to store directory entry*/
struct dirent {
  int inode_num;
  char name[FILENAMELEN];
};

/*Struct to store directory details*/
struct directory {
  int numentries;
  struct dirent entry[DIRECTORY_SIZE];
};

/*Struct to file system details*/
struct fsystem {
  int nblocks;
  int blocksz;
  int ninodes;
  int inodes_used;
  int freemaskbytes;
  char *freemask;
  struct directory root_dir;
};

/* file and directory functions */
int fs_open(char *filename, int flags);
int fs_close(int fd);
int fs_create(char *filename, int mode);
int fs_seek(int fd, int offset);
int fs_read(int fd, void *buf, int nbytes);
int fs_write(int fd, void *buf, int nbytes);

/* filesystem functions */
int fs_mkfs(int dev, int num_inodes);
int fs_mount(int dev);

/* filesystem internal functions */
int fs_get_inode_by_num(int dev, int inode_number, struct inode *in);
int fs_put_inode_by_num(int dev, int inode_number, struct inode *in);
int fs_setmaskbit(int b);
int fs_clearmaskbit(int b);
int fs_getmaskbit(int b);

/*
  Block Store functions
  bread, bwrite,
  bput, bget write entire blocks (macro with offset=0, len=blocksize)
 */
int bs_mkdev(int dev, int blocksize, int numblocks);
int bs_bread(int bsdev, int block, int offset, void *buf, int len);
int bs_bwrite(int bsdev, int block, int offset, void * buf, int len);

/* debugging functions */
void fs_printfreemask(void);
void fs_print_fsd(void);

#endif /* FS_H */