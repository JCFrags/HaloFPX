#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void die(const char *s){fprintf(stderr,"%s: %s (errno=%d)\n",s,strerror(errno),errno);exit(1);}
static void exact_write(int fd,const char *p,size_t n){size_t o=0;while(o<n){ssize_t r=pwrite(fd,p+o,n-o,o);if(r>0)o+=(size_t)r;else if(r<0&&errno==EINTR)continue;else{if(r==0)errno=EIO;die("pwrite");}}}
int main(int argc,char **argv){
    if(argc!=3){fprintf(stderr,"usage: %s DIR LINK_NAME\n",argv[0]);return 64;}
    if(strchr(argv[2],'/')) return 64;
    int dfd=open(argv[1],O_RDONLY|O_DIRECTORY|O_CLOEXEC);if(dfd<0)die("open dir");
    int fd=openat(dfd,".",O_TMPFILE|O_RDWR|O_CLOEXEC,0600);if(fd<0)die("O_TMPFILE open");
    const char payload[]="PF-IR-06 O_TMPFILE probe\n";
    exact_write(fd,payload,sizeof(payload)-1);
    if(fdatasync(fd)<0)die("fdatasync");
    if(linkat(fd,"",dfd,argv[2],AT_EMPTY_PATH)<0)die("linkat AT_EMPTY_PATH");
    if(fsync(dfd)<0)die("fsync dir");
    puts("O_TMPFILE_CREATE_LINK_FILESYNC_DIRSYNC_OK");
    unlinkat(dfd,argv[2],0); fsync(dfd);
    close(fd);close(dfd);return 0;
}
