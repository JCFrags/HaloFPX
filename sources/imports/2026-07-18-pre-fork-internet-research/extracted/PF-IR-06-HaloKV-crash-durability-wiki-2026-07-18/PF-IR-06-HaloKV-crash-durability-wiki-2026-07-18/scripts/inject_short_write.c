#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static ssize_t (*real_write)(int,const void*,size_t);
static ssize_t (*real_pwrite)(int,const void*,size_t,off_t);
static pthread_once_t once=PTHREAD_ONCE_INIT;
static _Atomic unsigned long calls;
static void init(void){real_write=dlsym(RTLD_NEXT,"write");real_pwrite=dlsym(RTLD_NEXT,"pwrite");}
static long envlong(const char*n,long d){const char*s=getenv(n);if(!s||!*s)return d;char*e;long v=strtol(s,&e,10);return *e?d:v;}
static int injected(unsigned long c){long at=envlong("PF_INJECT_AT",-1);return at>=0&&(unsigned long)at==c;}
static ssize_t choose_count(size_t n){long cap=envlong("PF_WRITE_CAP",-1);if(cap>0&&(size_t)cap<n)return (ssize_t)cap;return (ssize_t)n;}
static int choose_errno(void){const char*s=getenv("PF_INJECT_ERRNO");if(!s)return 0;if(!strcmp(s,"EINTR"))return EINTR;if(!strcmp(s,"ENOSPC"))return ENOSPC;if(!strcmp(s,"EDQUOT"))return EDQUOT;if(!strcmp(s,"EIO"))return EIO;return atoi(s);}
ssize_t write(int fd,const void*b,size_t n){pthread_once(&once,init);unsigned long c=atomic_fetch_add(&calls,1);if(injected(c)){int e=choose_errno();if(!e)return 0;errno=e;return -1;}return real_write(fd,b,(size_t)choose_count(n));}
ssize_t pwrite(int fd,const void*b,size_t n,off_t o){pthread_once(&once,init);unsigned long c=atomic_fetch_add(&calls,1);if(injected(c)){int e=choose_errno();if(!e)return 0;errno=e;return -1;}return real_pwrite(fd,b,(size_t)choose_count(n),o);}
