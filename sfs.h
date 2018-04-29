
#ifndef _SFS_H_
#define _SFS_H_

#define INDIRENT_BLOCK_COUNT 128
#define INODE_BLOCK_COUNT 80
#define INDIRENT_DBL_BLOCK_COUNT 16384
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
	int opened;
	int totalSize;
	int blocks[INODE_BLOCK_COUNT];
	int blockPtrIndex;
	int blockDoublePtrIndex;
	char path[100];
} inode;

typedef struct super_block{
	int magicNumber;
	int headNum;
}super;

#endif
