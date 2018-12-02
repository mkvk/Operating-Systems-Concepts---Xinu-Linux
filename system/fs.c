#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef FS
#include <fs.h>

static struct fsystem fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0
#define BM_BLK 1
#define RT_BLK 2

#define NUM_FD 16
struct filetable oft[NUM_FD];
int next_open_fd = 0;

#define INODES_PER_BLOCK (fsd.blocksz / sizeof(struct inode))
#define NUM_INODE_BLOCKS (( (fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

int fileblock_to_diskblock(int dev, int fd, int fileblock);

int fs_open(char *filename, int flags) { 
   
  struct directory r_dir = fsd.root_dir;
  struct inode in;
  int p=0,in_number;

  while( p < r_dir.numentries) {// search for file name in the root directory
    if (strcmp(r_dir.entry[p].name, filename) == 0) {  // if root directory has this particular file
      in_number = r_dir.entry[p].inode_num; // then fetch inode number 
      fs_get_inode_by_num(dev0, in_number, &in); // get respective inode
      // modify in file table
      oft[next_open_fd].state = FSTATE_OPEN;
      oft[next_open_fd].fileptr = 0;
      oft[next_open_fd].de = &(r_dir.entry[p]);
      oft[next_open_fd].in=in;  
      oft[next_open_fd].mode = flags;    
      next_open_fd++; // increment to next available index for oft
      return next_open_fd - 1;
    }
    p++;
  }
  
  fprintf(stderr, "Error from fs_open.\nFile not found\n"); // if invalid, print error
  return SYSERR;
}

int fs_close(int fd) {
  // verify if the passed fd is valid
  if (fd >= 0 && fd < next_open_fd) {  
    oft[fd].state = FSTATE_CLOSED;  // if fd is valid change state to CLOSED
    return OK;
  } 
  
  fprintf(stderr, "Error from fs_close.\n Invalid file\n"); // if fd is invalid, print error 
  return SYSERR;
}

int fs_create(char *filename, int mode) {   

  struct filetable ft;
  struct inode in;
  int fd=next_open_fd;
  if(mode==O_CREAT) {
    struct directory r_dir=fsd.root_dir;
    int p=0;
    while(p<r_dir.numentries) { //verify if any file already exists with the same name 
      if(strcmp(r_dir.entry[p].name,filename)==0) {
        fprintf(stderr, "Error from fs_create.\nA file with same name already exists\n");
        return SYSERR;
      }
      p++;
    }
    
    next_open_fd++;    // make filetable entry
    ft.state=FSTATE_OPEN;    
    ft.mode = O_RDWR; // setting file mode to read and write   
    ft.fileptr=0;    
    ft.de=&(r_dir.entry[r_dir.numentries++]);    
    strcpy((ft.de)->name, filename);    
    fsd.root_dir=r_dir;  // update root dir
    in.type=INODE_TYPE_FILE;    
    in.id=1;    
    fs_put_inode_by_num(0,in.id,&in);  // store inode 
    int bl = in.id / INODES_PER_BLOCK;
    bl += FIRST_INODE_BLOCK;
    fs_setmaskbit(bl);    // set bitmask
    ft.in=in;    // store inode to filetable
    (ft.de)->inode_num=in.id; // make changes to ident
    r_dir.entry[r_dir.numentries-1].inode_num=in.id;
    oft[fd]=ft;   // assign ft
    fsd.inodes_used++;  //update inodes used  
   return fd;
  }
  fprintf(stderr, "Error from fs_create.\nUse O_CREAT mode to create\n");
  return SYSERR;
}               

int fs_seek(int fd, int offset) {

  if (fd >= 0 && fd < next_open_fd) {  
    offset=oft[fd].fileptr+offset;    // get existing offset and add the passed offset
    if(offset>=0) {
      (oft+fd)->fileptr=offset;
      return fd;
    }
    // if offset is -ve, set to 0 and return error
    offset=0;
    fprintf(stderr,"Error from fs_seek.\nSet pointer to 0 (start of file)\n");
    return SYSERR;
  }
  fprintf(stderr, "Error from fs_seek.\nInvalid file\n"); // if fd is invalid, print error 
  return SYSERR;
}

int fs_read(int fd, void *buf, int nbytes) { 

  struct filetable ft=oft[fd];
  struct inode in = ft.in;
  // verify mode of file
  if(oft[fd].mode==O_WRONLY){
    fprintf(stderr, "Cannot Read.\nFile is present in write only mode.\n");
    return SYSERR;
  }
  // verify file state
  if(ft.state==FSTATE_CLOSED){   
    fprintf(stderr, "Error in read\nState is closed\n");
    return SYSERR;
  }
  //find block & offset
  int bl=ft.fileptr / fsd.blocksz;
  int offset=(ft.fileptr % fsd.blocksz);
  int t_buf=nbytes;
  if (bl<INODEBLOCKS){ // if block is less than inodeblocks then serve request
    while(nbytes>0){    // till requested bytes are processed
      if(nbytes<(fsd.blocksz-offset)){ // incase single block can accommodate
        bs_bread(0,ft.in.blocks[bl],offset,buf,nbytes);
        ft.fileptr+=nbytes;
        buf+=nbytes;
        nbytes=0;
        return t_buf;
      }
      if(bl==INODEBLOCKS-1){ // if requested size is more than max
          fprintf(stderr, "Requested size cannot be returned.\nReturning only till available size limit\n");
          return t_buf-nbytes;
      }
      bs_bread(0,ft.in.blocks[bl],offset,buf,fsd.blocksz-offset);
      nbytes-=(fsd.blocksz-offset);
      buf+=(fsd.blocksz-offset);
      ft.fileptr+=(fsd.blocksz-offset);
      offset=0;
      bl++;
      ft.in.blocks[bl]=in.blocks[bl];
      oft[fd]=ft;         
    }
  }
  return t_buf-nbytes;
}

int fs_write(int fd, void *buf, int nbytes) { 

  struct filetable ft=oft[fd];
  struct inode in = ft.in; 
  int t_buf=nbytes;  
  int p,c=0;
  int bl= (ft.fileptr / fsd.blocksz); 
  int offset=(ft.fileptr % fsd.blocksz);
  // verify mode of file
  if(oft[fd].mode==O_RDONLY){
    fprintf(stderr, "Cannot Write.\nFile is present in read only mode.\n");
    return SYSERR;
  }
  // verify the file state
  if(ft.state==FSTATE_CLOSED){
    fprintf(stderr, "Error in write.\nFile in closed state.\n");
    return SYSERR;
  }
  else if (bl<INODEBLOCKS){ // if block is less than inodeblocks then serve request
    while(nbytes>0){  // till requested bytes are processed
      if(in.blocks[bl]==0){    
        for(;c<fsd.nblocks;c++) if(fs_getmaskbit(c)==0)  {p=c; break;} // get next availabile free block
        if(c==fsd.nblocks&&p==0) { //if all the blocks are used up, display error msg
          fprintf(stderr, "Error in write.\nUnable to get free block at the moment\n");
          return SYSERR;
        }
        // after obtaining free space, store respective inode and mask the index
        in.blocks[bl]=p;
        ft.in=in;
        oft[fd].in=in;
        fs_put_inode_by_num(0,in.id,&in);
        fs_setmaskbit(p);
      }
      else if(in.blocks[bl]>0)  p=in.blocks[bl]; 
      if(nbytes<(fsd.blocksz-offset)){ // check if the requested size is available
        bs_bwrite(0,p,offset,buf,nbytes);
        ft.fileptr+=nbytes;
        oft[fd]=ft;
        nbytes=0;
        return t_buf;
      }
      if(bl==INODEBLOCKS-1){ // if requested size is more than max
        fprintf(stderr, "Requested size cannot be returned.\nReturning only till available size limit\n");
        return t_buf-nbytes;
      }
      bs_bwrite(0,p,offset,buf,fsd.blocksz-offset);
      nbytes-=(fsd.blocksz-offset);
      buf+=(fsd.blocksz-offset);
      ft.fileptr+=(fsd.blocksz-offset);
      oft[fd]=ft;
      bl++;
      offset=0;
    }
    return t_buf-nbytes;
  }
  return SYSERR;
}

int fileblock_to_diskblock(int dev, int fd, int fileblock) {
  int diskblock;

  if (fileblock >= INODEBLOCKS - 2) {
    printf("No indirect block support\n");
    return SYSERR;
  }

  diskblock = oft[fd].in.blocks[fileblock]; //get the logical block address

  return diskblock;
}

/* read in an inode and fill in the pointer */
int fs_get_inode_by_num(int dev, int inode_number, struct inode *in) {
  int bl, inn;
  int inode_off;

  if (dev != 0) {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    printf("fs_get_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(struct inode);

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  printf("inn*sizeof(struct inode): %d\n", inode_off);
  */

  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(in, &block_cache[inode_off], sizeof(struct inode));

  return OK;

}

int fs_put_inode_by_num(int dev, int inode_number, struct inode *in) {
  int bl, inn;

  if (dev != 0) {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    printf("fs_put_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  */

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn*sizeof(struct inode))], in, sizeof(struct inode));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}

int fs_mkfs(int dev, int num_inodes) {
  int i;

  if (dev == 0) {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  }
  else {
    printf("Unsupported device\n");
    return SYSERR;
  }

  if (num_inodes < 1) {
    fsd.ninodes = DEFAULT_NUM_INODES;
  }
  else {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ( (i % 8) != 0) {i++;}
  fsd.freemaskbytes = i / 8;

  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *)SYSERR) {
    printf("fs_mkfs getmem failed.\n");
    return SYSERR;
  }

  /* zero the free mask */
  for(i=0;i<fsd.freemaskbytes;i++) {
    fsd.freemask[i] = '\0';
  }

  fsd.inodes_used = 0;

  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(struct fsystem));

  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  
  fs_setmaskbit(RT_BLK);
  bs_bwrite(dev0, RT_BLK, 0, &(fsd.root_dir), sizeof(struct directory));


  return 1;
}

void fs_print_fsd(void) {

  printf("fsd.ninodes: %d\n", fsd.ninodes);
  printf("sizeof(struct inode): %d\n", sizeof(struct inode));
  printf("INODES_PER_BLOCK: %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS: %d\n", NUM_INODE_BLOCKS);
}

/* specify the block number to be set in the mask */
int fs_setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

/* specify the block number to be read in the mask */
int fs_getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
  return OK;

}

/* specify the block number to be unset in the mask */
int fs_clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
  return OK;
}

/* This is maybe a little overcomplicated since the lowest-numbered
   block is indicated in the high-order bit.  Shift the byte by j
   positions to make the match in bit7 (the 8th bit) and then shift
   that value 7 times to the low-order bit to print.  Yes, it could be
   the other way...  */
void fs_printfreemask(void) {
  int i,j;

  for (i=0; i < fsd.freemaskbytes; i++) {
    for (j=0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  printf("\n");
}

#endif /* FS */