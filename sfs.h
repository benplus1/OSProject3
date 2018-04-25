
#ifndef _SFS_H_
#define _SFS_H_

typedef struct inode{
	ino_t id;
	int firstChild;
	int sibling;
	uid_t userId;
	gid_t groupId;
	off_t offset; 
	blksize_t blockSize;
	blkcnt_t blockCount;
	time_t lastAccess;
	time_t lastMod;
	time_t lastStatus;
	mode_t type;
	int totalSize;
	int blocks[50];
	int * blockPtrs[10];
	char path[100];
} inode;

typedef struct super_block{
	int magicNumber;
	int headNum;
}super;

#endif
