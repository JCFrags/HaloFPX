#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Test-only LD_PRELOAD injector for syscall-wrapper error paths.
 * It does not emulate filesystem writeback, journal replay, block-layer EIO,
 * controller caches, or power loss. Use dm/VM/hardware tests for those cases.
 */

static ssize_t (*real_write_fn)(int, const void *, size_t);
static ssize_t (*real_pwrite_fn)(int, const void *, size_t, off_t);
static int (*real_fdatasync_fn)(int);
static int (*real_fsync_fn)(int);
static int (*real_renameat_fn)(int, const char *, int, const char *);
static int (*real_linkat_fn)(int, const char *, int, const char *, int);
static int (*real_fallocate_fn)(int, int, off_t, off_t);
static pthread_once_t once = PTHREAD_ONCE_INIT;

static _Atomic unsigned long write_calls;
static _Atomic unsigned long pwrite_calls;
static _Atomic unsigned long fdatasync_calls;
static _Atomic unsigned long fsync_calls;
static _Atomic unsigned long renameat_calls;
static _Atomic unsigned long linkat_calls;
static _Atomic unsigned long fallocate_calls;

static void init_real(void)
{
    real_write_fn = dlsym(RTLD_NEXT, "write");
    real_pwrite_fn = dlsym(RTLD_NEXT, "pwrite");
    real_fdatasync_fn = dlsym(RTLD_NEXT, "fdatasync");
    real_fsync_fn = dlsym(RTLD_NEXT, "fsync");
    real_renameat_fn = dlsym(RTLD_NEXT, "renameat");
    real_linkat_fn = dlsym(RTLD_NEXT, "linkat");
    real_fallocate_fn = dlsym(RTLD_NEXT, "fallocate");
}

static long env_long(const char *name, long fallback)
{
    const char *s = getenv(name);
    char *end = NULL;
    long value;

    if (!s || !*s)
        return fallback;
    value = strtol(s, &end, 10);
    return end && *end == '\0' ? value : fallback;
}

static int fault_errno(void)
{
    const char *s = getenv("PF_STAGE_ERRNO");

    if (!s || !*s || strcmp(s, "EIO") == 0)
        return EIO;
    if (strcmp(s, "ENOSPC") == 0)
        return ENOSPC;
    if (strcmp(s, "EDQUOT") == 0)
        return EDQUOT;
    if (strcmp(s, "EINTR") == 0)
        return EINTR;
    if (strcmp(s, "EROFS") == 0)
        return EROFS;
    return atoi(s);
}

static int should_fault(const char *op, _Atomic unsigned long *counter)
{
    const char *selected = getenv("PF_STAGE_OP");
    const unsigned long call = atomic_fetch_add(counter, 1);
    const long at = env_long("PF_STAGE_AT", 0);

    return selected && strcmp(selected, op) == 0 && at >= 0 && call == (unsigned long)at;
}

static ssize_t fail_ssize(void)
{
    errno = fault_errno();
    return -1;
}

static int fail_int(void)
{
    errno = fault_errno();
    return -1;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    pthread_once(&once, init_real);
    if (should_fault("write", &write_calls))
        return fail_ssize();
    return real_write_fn(fd, buf, count);
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    pthread_once(&once, init_real);
    if (should_fault("pwrite", &pwrite_calls))
        return fail_ssize();
    return real_pwrite_fn(fd, buf, count, offset);
}

int fdatasync(int fd)
{
    pthread_once(&once, init_real);
    if (should_fault("fdatasync", &fdatasync_calls))
        return fail_int();
    return real_fdatasync_fn(fd);
}

int fsync(int fd)
{
    pthread_once(&once, init_real);
    if (should_fault("fsync", &fsync_calls))
        return fail_int();
    return real_fsync_fn(fd);
}

int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath)
{
    pthread_once(&once, init_real);
    if (should_fault("renameat", &renameat_calls))
        return fail_int();
    return real_renameat_fn(olddirfd, oldpath, newdirfd, newpath);
}

int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags)
{
    pthread_once(&once, init_real);
    if (should_fault("linkat", &linkat_calls))
        return fail_int();
    return real_linkat_fn(olddirfd, oldpath, newdirfd, newpath, flags);
}

int fallocate(int fd, int mode, off_t offset, off_t len)
{
    pthread_once(&once, init_real);
    if (should_fault("fallocate", &fallocate_calls))
        return fail_int();
    return real_fallocate_fn(fd, mode, offset, len);
}
