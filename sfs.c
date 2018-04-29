/*
   Simple File System

   This code is derived from function prototypes found /usr/include/fuse/fuse.h
   Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
   His code is licensed under the LGPLv2.

*/

#include "params.h"
#include "block.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"
#include "sfs.h"
const int SUPER_BLOCK=0;
const int HEAD_BLOCK=1;
const int BM_BLOCK=2;
const int MAGIC_NUMBER=1234;
int BLOCK_COUNT=32768;
int BM_BLOCK_COUNT=(32768)/512;
char * FILE_PATH="/.freespace/hkc33/testfsfile";
char bitmap[32768];

char headBuf[512];
super * sup;
inode * head;

void writeBitmap(){
	int i;
	for(i=0;i<BM_BLOCK_COUNT;i++){
		int offset=i*512;
		int blockIndex=i+BM_BLOCK;
		block_write(blockIndex,bitmap+offset);
	}
}

void readBitmap(){
	int i;
	for(i=0;i<BM_BLOCK_COUNT;i++){
		int offset=i*512;
		int blockIndex=i+BM_BLOCK;
		block_read(blockIndex,bitmap+offset);
	}
}
int getUsedBlockCount(){
	int count=0;
	int i=0;
	for(i=0;i<BLOCK_COUNT;i++){
		if(bitmap[i]==1){
			count++;
		}
	}
	return count;
}

void copyInode(inode * dest, inode * target){
	strcpy(dest->path,target->path);
	dest->firstChild=target->firstChild;
	dest->sibling=target->sibling;
	dest->userId=target->userId;
	dest->groupId=target->groupId;
	dest->offset=target->offset; 
	dest->blockSize=target->blockSize;
	dest->blockCount=target->blockCount;
	dest->lastAccess=target->lastAccess;
	dest->lastMod=target->lastMod;
	dest->lastStatus=target->lastStatus;
	dest->type=target->type;
	dest->totalSize=target->totalSize;
	dest->id=target->id;
	dest->totalSize=target->totalSize;
	dest->blockPtrIndex=target->blockPtrIndex;
	dest->blockDoublePtrIndex=target->blockDoublePtrIndex;
	memcpy(dest->blocks,target->blocks,sizeof(int)*INODE_BLOCK_COUNT);
	dest->totalSize=target->totalSize;
}

inode createInode(char * path, mode_t type, off_t offset, ino_t id){
	inode newInode;
	strcpy(newInode.path,path);
	int pathEnd=strlen(path);	
	newInode.path[pathEnd]='\0';
	newInode.type=type;
	newInode.firstChild=-1;
	newInode.sibling=-1;
	newInode.userId=getuid();
	newInode.groupId=getgid();
	newInode.blockSize=512;
	newInode.lastAccess=time(NULL);
	newInode.lastMod=time(NULL);
	newInode.lastStatus=time(NULL);
	newInode.blockCount=0;
	newInode.offset=offset;
	newInode.id=id;
	newInode.blockPtrIndex=-1;
	newInode.blockDoublePtrIndex=-1;
	return newInode;
}
int findFreeBlock(){
	int i;
	for(i=0;i<BLOCK_COUNT;i++){
		if(bitmap[i]==0){
			bitmap[i]=1;
			char buff[512];
			memset(buff,0,512);
			block_write(i,buff);
			return i;
		}
	}
	/*writeBitmap();
	block_write(SUPER_BLOCK,sup);
	block_write(sup->headNum,(char *)head);*/
	return -1;
}

int insertDataBlock(int blockIndex, inode * curr){
	int res=0;
	if(curr->blockCount<INODE_BLOCK_COUNT){
		log_msg("Get Normal Block: %s", curr->path);
		curr->blockCount++;
		curr->blocks[curr->blockCount-1]=blockIndex;	
		block_write(curr->id, curr);
	}else if (curr->blockCount>=INODE_BLOCK_COUNT&&curr->blockCount<INDIRENT_BLOCK_COUNT+INODE_BLOCK_COUNT){
		log_msg("Get Indirent Block: %s", curr->path);
		int indirent[INDIRENT_BLOCK_COUNT];
		if(curr->blockPtrIndex<=0){
			int newIndex=findFreeBlock();
			log_msg("Init single indirent\n");
			if(newIndex<=0){
				log_msg("Can't get indirent block %s", curr->path );
				return -ENOMEM;
			}
			int i=0;
			for (i=0;i<INDIRENT_BLOCK_COUNT;i++){
				indirent[i]=0;
			}
			curr->blockPtrIndex=newIndex;
		}
		block_read(curr->blockPtrIndex,indirent);
		
		int indirentIndex=curr->blockCount-INODE_BLOCK_COUNT;
		indirent[indirentIndex]=blockIndex;
		log_msg("Insert Indirent Block %d\n", indirentIndex);
		block_write(curr->blockPtrIndex,indirent);	
		curr->blockCount++;
	}else if (curr->blockCount>=(INDIRENT_BLOCK_COUNT+INODE_BLOCK_COUNT)&&curr->blockCount<INDIRENT_DBL_BLOCK_COUNT+INDIRENT_BLOCK_COUNT+INODE_BLOCK_COUNT){
		log_msg("Got Double Indirent Block");
		
		if(curr->blockDoublePtrIndex<=0){
			log_msg("Init indirent\n");
			int newIndex=findFreeBlock();
			if(newIndex<=0){
				log_msg("No blocks left\n");
				return -ENOMEM;
			}
			curr->blockDoublePtrIndex=newIndex;
		}
		int indirent[INDIRENT_BLOCK_COUNT];
		log_msg("Reading from double ptr index %d\n", curr->blockDoublePtrIndex);
		block_read(curr->blockDoublePtrIndex,indirent);
		
		int doubleOffset=(curr->blockCount-INODE_BLOCK_COUNT-INDIRENT_BLOCK_COUNT);
		int indirentIndex=doubleOffset/INDIRENT_BLOCK_COUNT;
		int doubleIndex=doubleOffset%INDIRENT_BLOCK_COUNT;
		
		if(indirent[indirentIndex]<=0){
			int newIndex=findFreeBlock();
			if(newIndex<=0){
				log_msg("Double no blocks left\n");
				return -ENOMEM;
			}
			indirent[indirentIndex]=newIndex;	
		}
		log_msg("Double Indirent Offset: %d IndirentIndex: %d DoubleIndex: %d\n", doubleOffset, indirentIndex, doubleIndex);	
		int doubleInd[INDIRENT_BLOCK_COUNT];
		block_read(indirent[indirentIndex],doubleInd);
		doubleInd[doubleIndex]=blockIndex;
		
		curr->blockCount++;
		block_write(curr->id,curr);	
		block_write(curr->blockDoublePtrIndex, indirent);
		block_write(indirent[indirentIndex],doubleInd);
	}else{
		log_msg("File asking for too much\n");
	}
	return res;
}
int getDataBlock(inode * curr,int blockNum){
	if(blockNum<INODE_BLOCK_COUNT){
		return curr->blocks[blockNum];
	}else if(blockNum>=INODE_BLOCK_COUNT&&blockNum<INDIRENT_BLOCK_COUNT+INODE_BLOCK_COUNT){
		int indirent[INDIRENT_BLOCK_COUNT];
		block_read(curr->blockPtrIndex,indirent);
		int indirentIndex=blockNum-INODE_BLOCK_COUNT;
		log_msg("Get Indirent Block: %s at %d\n", curr->path, indirentIndex);
		log_msg("Index returned is %d\n", indirent[indirentIndex]);
		return indirent[indirentIndex];	
	}else if (blockNum>=INDIRENT_BLOCK_COUNT+INODE_BLOCK_COUNT&&blockNum<INDIRENT_DBL_BLOCK_COUNT+INDIRENT_BLOCK_COUNT+INODE_BLOCK_COUNT){
		log_msg("Got Double Indirent Block");
		
		log_msg("Reading from double ptr index %d\n", curr->blockDoublePtrIndex);
		int indirent[INDIRENT_BLOCK_COUNT];
		block_read(curr->blockDoublePtrIndex,indirent);
		
		int doubleOffset=(blockNum-INODE_BLOCK_COUNT-INDIRENT_BLOCK_COUNT);
		int indirentIndex=doubleOffset/INDIRENT_BLOCK_COUNT;
		int doubleIndex=doubleOffset%INDIRENT_BLOCK_COUNT;
		
		log_msg("Double Indirent Offset: %d IndirentIndex: %d DoubleIndex: %d\n", doubleOffset, indirentIndex, doubleIndex);	
		int doubleInd[INDIRENT_BLOCK_COUNT];
		block_read(indirent[indirentIndex],doubleInd);
		log_msg("Index returned is %d\n", doubleInd[doubleIndex]);
		return doubleInd[doubleIndex];	
	}else{
		log_msg("File asking for too much\n");
		return -1;
	}
}

void freeBlocks(inode * curr){
	int blockCount=curr->blockCount;
	int i;
	for (i=0;i<blockCount;i++){
		int blockIndex=getDataBlock(curr,i);		
		freeBlock(blockIndex);
	}
}

void freeBlock(int index){
	char clear[512];
	block_write(index,clear);
	bitmap[index]=0;
}

int getInode(char * realPath, inode * buf){
	char path[512];
	strcpy(path,realPath);
	char buff[512];
	block_read(HEAD_BLOCK,buff);
	inode * ptr=buff;
	char * token=strtok(path,"/");
	int found=1;
	while(ptr!=NULL&&token){
		printf("token %s\n", token);

		int ptrNum=ptr->firstChild;
		inode * childPtr;

		found=0;
		while(ptrNum>0){
			block_read(ptrNum,buff);
			childPtr=(inode *) buff;
			if(strcmp(childPtr->path,token)==0){
				found=1;
				break;
			}
			ptrNum=childPtr->sibling;
		}
		if(!found){
			log_msg("Failed traversing\n");	
			break;
		}
		ptr=childPtr;
		token=strtok(NULL,"/");
	} 

	if(found){
		copyInode(buf,ptr);
		return 0;
	}else{
		return -ENOENT;
	}		
}

int insertInode(char * path, mode_t type){
	int retstat=0;
	inode tempNode;
	int res=getInode(path,&tempNode);	

	//build components of path
	/*char * part=strok(path,"/");	
	  char * arr[50];
	  int i=0;
	  while(part!=NULL){
	  arr[i]=part;
	  part=strtok(path,"/");
	  i++;
	  }*/

	if(res!=0){ //File not already there->can safely create
		char temp[100];
		memcpy(temp,path,strlen(path)+1);
		char * ptr=temp;
		log_msg("Start %s\n", ptr);
		while(ptr!=NULL){//Genereate a path string for new file's directory
			char * tempPtr=strchr(ptr,'/');
			if(tempPtr==NULL){
				break;
			}
			tempPtr++;
			ptr=tempPtr;
			log_msg("Part %s\n", ptr);	
		}
		if(ptr!=temp){
			*(ptr-1)='\0';//clips full path to be just path for directory
		}
		res=getInode(temp,&tempNode);

		if(res==0){ //Director exists->can safely place file in it
			int newBlock=findFreeBlock();
			if(newBlock!=-1){
				inode newFile=createInode(ptr,type,0,newBlock);	
				int blockNum=tempNode.firstChild;
				if(blockNum<=0){ //New file will be directory's first child
					tempNode.firstChild=newBlock;	
					block_write(tempNode.id,&tempNode);	
				}else{ //New file will be sibling of pre-existent file
					char buf[512];
					block_read(blockNum,buf);
					inode * ptr=(inode *)buf;
					while(blockNum>0){
						block_read(blockNum,buf);
						ptr=(inode *)buf;
						blockNum=ptr->sibling;
					}
					ptr->sibling=newBlock;
					block_write(ptr->id,ptr);
				}
				block_write(newBlock,&newFile);	
				bitmap[newBlock]=1;
			}else{
				log_msg("Can't get new block %s\n", path);
				retstat=-ENOMEM;
			}	
		}else{
			log_msg("Directory don't exit %s\n",path);
			retstat=-ENOENT;
		}

	}else{
		log_msg("File %s exists\n",path);
		retstat=EEXIST;
	}
	return retstat;

}


/*int deleteInode(char * realPath, inode * buf){
	char path[512];
	strcpy(path,realPath);
	char buff[512];
	block_read(HEAD_BLOCK,buff);
	inode * ptr=buff;

	char parentBuff[512];
	block_read(HEAD_BLOCK, parentBuff);
	inode * parentPtr = parentBuff;
	int lastParentNum = HEAD_BLOCK;

	char * token=strtok(path,"/");
	int found=1;
	while (ptr != NULL && token) { //check first set of children
		printf("token %s\n", token);
		int ptrNum = ptr->firstChild;
		inode * childPtr;
		found = 0;
		char lastChildBuff[512];
		inode * lastChildPtr;
		inode * lastPtrNum;
		if (ptrNum > 0) {//check first first child
			block_read(ptrNum, buff);
			childPtr = (inode *) buff;
			if (strcmp(childPtr->path, token)==0) {
				if (childPtr->sibling <= 0) {//no other siblings
					parentPtr->firstChild = -1; //unlink
					block_write(lastParentNum,parentBuff); //write change
					bitmap[ptrNum] = 0;
					return 0;
				}
				else {//one other sibling
					parentPtr->firstChild = childPtr->sibling;
					block_write(lastParentNum, parentBuff);
					bitmap[ptrNum] = 0;	
					return 0;
				}
			}
			block_read(ptrNum, lastChildBuff);
			lastChildPtr = (inode *) lastChildBuff;
			lastPtrNum = ptrNum;

			ptrNum=childPtr->sibling;
		}
		while (ptrNum > 0) {//now check the siblings of first first child
			block_read(ptrNum, buff);
			childPtr=(inode *) buff;
			if (strcmp(childPtr->path, token)==0) {
				if (childPtr->sibling < 0) {//last sibling
					lastChildPtr->sibling = -1;
					block_write(lastPtrNum, lastChildBuff);
					bitmap[ptrNum] = 0;
					return 0;
				}
				else {//skip sibling
					lastChildPtr->sibling = childPtr->sibling;
					block_write(lastPtrNum, lastChildBuff);
					bitmap[ptrNum] = 0;
					return 0;
				}
			}
			block_read(ptrNum, lastChildBuff);
			lastChildPtr = (inode *) lastChildBuff;
			lastPtrNum = ptrNum;

			ptrNum=childPtr->sibling;
		}
		if (ptrNum <= 0) {
			log_msg("Failed traversing\n");
			break;
		}

		block_read(ptrNum, parentBuff);
		parentPtr=(inode *) parentBuff;
		lastParentNum = ptrNum;

		ptr = childPtr;
		token = strtok(NULL, "/");
	}

	return -ENOENT;		
}*/


int deleteInode(char * path){
	int res=0;
	inode curr;
	res=getInode(path,&curr); //Get current inode
	if(res!=0) return res;


	char temp[100];
	memcpy(temp,path,strlen(path)+1);
	char * ptr=temp;
	log_msg("Start %s\n", ptr);
	while(ptr!=NULL){//Genereate a path string for new file's directory
		char * tempPtr=strchr(ptr,'/');
		if(tempPtr==NULL){
			break;
		}
		tempPtr++;
		ptr=tempPtr;
		log_msg("Part %s\n", ptr);	
	}
	if(ptr!=temp){
		*(ptr-1)='\0';//clips full path to be just path for directory
	}
	
	inode parent;
	getInode(temp,&parent); //Get parent inode
	
	int prevBlockNum=-1;
	int ptrBlockNum=parent.firstChild;
	while(ptrBlockNum!=-1){
		char buf[512];
		block_read(ptrBlockNum,buf);
		inode * ptr=buf;
		if(strcmp(ptr->path,curr.path)==0){
			log_msg("Found inode to delete \n", ptr->path);
			break;
		}
		prevBlockNum=ptrBlockNum;
		ptrBlockNum=ptr->sibling;
	}
	if(prevBlockNum==-1){
		parent.firstChild=curr.sibling;
	}else{
		char buf[512];
		block_read(prevBlockNum, buf);
		inode * prev=buf;
		prev->sibling=curr.sibling;
		block_write(prev->id,prev);
	}
	block_write(parent.id,&parent);
	freeInode(&curr);
	return res;
}

void freeInode(inode * curr){
	bitmap[curr->id]=0;
	char clear[512];
	freeBlocks(curr);
	block_write(curr->id,clear);
}
inode testInode(int blockNum){
	char buf[512];
	block_read(blockNum,buf);
	inode * curr=buf;
	return *curr;	
} 

int testIndirent(int blockIndex, int indirentBlockNum){
	int indirent[INDIRENT_BLOCK_COUNT];
	block_read(blockIndex,indirent);
	return indirent[indirentBlockNum];
}

int testDoubleIndirent(int blockIndex, int indirentBlockNum, int doubleIndex){
	int indirent[INDIRENT_BLOCK_COUNT];
	block_read(blockIndex,indirent);
	int dbl[INDIRENT_BLOCK_COUNT];
	block_read(indirent[indirentBlockNum], dbl);

	return dbl[doubleIndex];
}
///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *sfs_init(struct fuse_conn_info *conn)
{
	fprintf(stderr, "in bb-init\n");
	log_msg("Process ID: %d\n",getpid());
	log_msg("\nsfs_init()\n");
	disk_open(FILE_PATH);
	block_read(BM_BLOCK,bitmap);   
	block_read(SUPER_BLOCK,headBuf);

	sup=(super *)headBuf;
	if(sup->magicNumber!=MAGIC_NUMBER){ //Have to reformat 
		log_msg("Have to reformat\n");
		//Update head (super block)
		head=malloc(sizeof(inode));
		strcpy(head->path,"");
		head->firstChild=-1;
		head->sibling=-1;
		head->userId=getuid();
		head->groupId=getgid();
		head->lastAccess=time(NULL);
		head->lastMod=time(NULL);
		head->lastStatus=time(NULL);
		head->type=S_IFDIR;
		head->id=HEAD_BLOCK;

		memset(bitmap,0,128);
		int i;
		for(i=0;i<BLOCK_COUNT;i++){
			bitmap[i]=0;
		}
		bitmap[SUPER_BLOCK]=1;
		bitmap[HEAD_BLOCK]=1;
		
		for(i=BM_BLOCK;i<BM_BLOCK_COUNT+BM_BLOCK;i++){
			bitmap[i]=1;
		}
		sup->magicNumber=MAGIC_NUMBER;
		sup->headNum=HEAD_BLOCK;
		block_write(SUPER_BLOCK,(char *) sup);	
		block_write(sup->headNum,(char *)head);
		writeBitmap();
		char bu[512];
		block_read(sup->headNum,bu);

		inode * yo=(inode *) bu;
		log_msg("Retrieved value %d\n",yo->id );
		log_msg("Wrote Head Block at: %d\n",sup->headNum);
	}else{
		block_read(sup->headNum,headBuf); //Can safely grab head block	
		head=(inode *) headBuf;
		readBitmap();
	}		
	log_msg("Starting Num Used Blocks: %d\n", getUsedBlockCount());
	log_conn(conn);
	log_fuse_context(fuse_get_context());

	log_msg("Done initing\n"); 
	return SFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata)
{
	log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
	writeBitmap();
	block_write(SUPER_BLOCK,sup);
	block_write(sup->headNum,(char *)head);
	disk_close(FILE_PATH);
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{
	log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
			path, statbuf);
	int retstat = 0;
	char fpath[PATH_MAX];
	//retstat=lstat(path,statbuf);
	if(retstat<0){
		retstat=-errno;
	}

	struct inode tar;
	int res=getInode(path,&tar);
	struct inode * target=&tar;
	if(res==0){
		//fill stat buff
		statbuf->st_uid=target->userId;
		statbuf->st_gid=target->groupId;
		statbuf->st_size=target->offset; 
		statbuf->st_blksize=target->blockSize;
		statbuf->st_blocks=target->blockCount;
		statbuf->st_atime=target->lastAccess;
		statbuf->st_mtime=target->lastMod;
		statbuf->st_ctime=target->lastStatus;
		statbuf->st_mode=target->type;
		statbuf->st_size=target->totalSize;
	}else{
		log_msg("File %s\n does not exist\n", path);
		retstat=-ENOENT;
	} 
	log_msg("Stat result: %d\n",retstat);
	return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
			path, mode, fi);
	retstat=insertInode(path, S_IFREG);
	return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path)
{
	int retstat = 0;
	log_msg("sfs_unlink(path=\"%s\")\n", path);
	inode tempNode;
	retstat = deleteInode(path);

	log_msg("Result Unlink %s: %d\n", path,retstat);
	log_msg("Num Used Blocks %d\n", getUsedBlockCount());
	return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
			path, fi);


	return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
			path, fi);


	return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
			path, buf, size, offset, fi);
	inode curr;
	getInode(path,&curr);
	int blockNum=(int)offset/512;
	int blockOffset=offset%512;

	int blockIndex=-1;
	int writeSize=512-blockOffset;
	int incSize=0;
	while(incSize<size&&incSize<curr.totalSize){
		blockIndex=getDataBlock(&curr,blockNum);
		char myBuf[512];
		block_read(blockIndex,myBuf);
		memcpy(buf+incSize,myBuf+blockOffset,writeSize);
		blockNum++;
		incSize+=writeSize;
		int remSize=size-incSize;
		if(remSize>=512){
			writeSize=512;
		}else{
			writeSize=remSize;
		}
		blockOffset-=blockOffset;
	}	
	int bytesRead=0;
	if(size<curr.totalSize){
		bytesRead=size;
	}else{
		bytesRead=curr.totalSize;
	}
	log_msg("Bytes Read: %d\n", bytesRead);	
	return bytesRead;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
			path, buf, size, offset, fi);
	inode curr;
	getInode(path,&curr);
	int blockNum=(int)offset/512;
	int blockOffset=offset%512;

	int blockIndex=-1;

	int incSize=0;

	while(incSize<size){
		if(curr.blockCount<=blockNum){
			int newBlock=findFreeBlock();
			log_msg("new block id is %d\n", newBlock);
			if(newBlock==-1) {\
				log_msg("fucked\n");
				size=incSize;
				return size;
			}
			blockIndex=newBlock;
			int res=insertDataBlock(blockIndex, &curr);
			if(res!=0){
				log_msg("Gotta cut writing\n");
				size=incSize;
				return size;
			}
		}else{
			blockIndex=curr.blocks[blockNum];
		}
		int writeSize=512-blockOffset;
		char myBuf[512];
		block_read(blockIndex,myBuf);
		memcpy(myBuf+blockOffset,buf+incSize,writeSize);
		block_write(blockIndex,myBuf);
		blockOffset-=blockOffset;
		blockNum++;
		incSize+=writeSize;
	}	

	if(offset+size>curr.totalSize){
		curr.totalSize=offset+size;
	}

	block_write(curr.id,&curr);

	log_msg("Size written: %d\n",size);
	return size;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
	int retstat = 0;
	log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
			path, mode);
	retstat=insertInode(path,S_IFDIR);
	
	return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path)
{
	int retstat = 0;
	log_msg("sfs_rmdir(path=\"%s\")\n",
			path);

	inode tempNode;
	retstat = deleteInode(path);

	log_msg("Num Used Blocks %d\n", getUsedBlockCount());

	return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n",
			path, fi);


	return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("sfs_readdir\n");
	filler(buf, ".",NULL,0);
	filler(buf,"..",NULL,0);
	inode tar;
	int res=getInode(path,&tar);
	if(res==0){
		char buff[512];
		int  inodeNum=tar.firstChild;
		while(inodeNum>0){
			block_read(inodeNum,buff);
			inode * curr=(inode *)buff;
			filler(buf,curr->path,NULL,0);
			inodeNum=curr->sibling;
		}	

	}else{
		retstat=res;
	}

	return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;


	return retstat;
}

struct fuse_operations sfs_oper = {
	.init = sfs_init,
	.destroy = sfs_destroy,

	.getattr = sfs_getattr,
	.create = sfs_create,
	.unlink = sfs_unlink,
	.open = sfs_open,
	.release = sfs_release,
	.read = sfs_read,
	.write = sfs_write,

	.rmdir = sfs_rmdir,
	.mkdir = sfs_mkdir,

	.opendir = sfs_opendir,
	.readdir = sfs_readdir,
	.releasedir = sfs_releasedir
};

void sfs_usage()
{
	fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
	abort();
}

int main(int argc, char *argv[])
{
	int fuse_stat;
	struct sfs_state *sfs_data;

	// sanity checking on the command line
	if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
		sfs_usage();

	sfs_data = malloc(sizeof(struct sfs_state));
	if (sfs_data == NULL) {
		perror("main calloc");
		abort();
	}

	// Pull the diskfile and save it in internal data
	sfs_data->diskfile = argv[argc-2];
	argv[argc-2] = argv[argc-1];
	argv[argc-1] = NULL;
	argc--;

	sfs_data->logfile = log_open();

	// turn over control to fuse
	fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
	fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
	fprintf(stderr, "fuse_main returned %d\n", fuse_stat);

	return fuse_stat;
}
