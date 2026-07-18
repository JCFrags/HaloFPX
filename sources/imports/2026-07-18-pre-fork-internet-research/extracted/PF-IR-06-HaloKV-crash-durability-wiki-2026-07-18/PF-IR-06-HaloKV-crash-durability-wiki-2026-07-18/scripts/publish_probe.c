#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/fs.h>

#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE (1U << 0)
#endif

static void die(const char *what) {
    int e=errno;
    fprintf(stderr,"%s: %s (errno=%d)\n",what,strerror(e),e);
    exit(1);
}
static void failpoint(const char *name) {
    const char *p=getenv("PF_FAILPOINT");
    if (p && strcmp(p,name)==0) {
        fprintf(stderr,"PF_FAILPOINT=%s: SIGKILL\n",name);
        fflush(NULL);
        kill(getpid(),SIGKILL);
        _exit(137);
    }
}
static void write_exact(int fd,const unsigned char *p,size_t n) {
    size_t off=0;
    while(off<n) {
        ssize_t r=pwrite(fd,p+off,n-off,(off_t)off);
        if(r>0) { off+=(size_t)r; continue; }
        if(r==0) { errno=EIO; die("zero-length pwrite with bytes remaining"); }
        if(errno==EINTR) continue;
        die("pwrite");
    }
}
static uint64_t fnv1a(const unsigned char *p,size_t n) {
    uint64_t h=UINT64_C(1469598103934665603);
    for(size_t i=0;i<n;i++){h^=p[i];h*=UINT64_C(1099511628211);} return h;
}
static int rename_noreplace(int dfd,const char *oldn,const char *newn) {
#ifdef SYS_renameat2
    return (int)syscall(SYS_renameat2,dfd,oldn,dfd,newn,RENAME_NOREPLACE);
#else
    errno=ENOSYS; return -1;
#endif
}
int main(int argc,char **argv) {
    if(argc!=4){fprintf(stderr,"usage: %s DIR FINAL_NAME PAYLOAD\n",argv[0]);return 64;}
    const char *dir=argv[1],*final=argv[2],*payload=argv[3];
    if(strchr(final,'/')){fprintf(stderr,"FINAL_NAME must be one component\n");return 64;}
    int dfd=open(dir,O_RDONLY|O_DIRECTORY|O_CLOEXEC); if(dfd<0)die("open directory");
    char tmp[128]; snprintf(tmp,sizeof(tmp),".pf-ir-06.tmp.%ld",(long)getpid());
    int fd=openat(dfd,tmp,O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC,0600); if(fd<0)die("openat temp");
    failpoint("after_create");
    size_t n=strlen(payload); write_exact(fd,(const unsigned char*)payload,n);
    if(ftruncate(fd,(off_t)n)<0)die("ftruncate");
    struct stat st; if(fstat(fd,&st)<0)die("fstat");
    if((uint64_t)st.st_size!=(uint64_t)n){errno=EIO;die("size mismatch");}
    fprintf(stderr,"payload_len=%zu fnv1a=%016" PRIx64 "\n",n,fnv1a((const unsigned char*)payload,n));
    failpoint("after_write");
    if(fdatasync(fd)<0)die("fdatasync temp");
    failpoint("after_file_sync");
    bool replace=getenv("PF_REPLACE") && strcmp(getenv("PF_REPLACE"),"0")!=0;
    if(replace) {
        if(renameat(dfd,tmp,dfd,final)<0)die("renameat replace");
    } else {
        if(rename_noreplace(dfd,tmp,final)<0)die("renameat2 RENAME_NOREPLACE");
    }
    failpoint("after_publish");
    if(fsync(dfd)<0)die("fsync directory");
    failpoint("after_dir_sync");
    if(close(fd)<0)fprintf(stderr,"diagnostic close(temp): %s\n",strerror(errno));
    if(close(dfd)<0)fprintf(stderr,"diagnostic close(dir): %s\n",strerror(errno));
    puts("ACK_AFTER_DIRECTORY_FSYNC");
    return 0;
}
