#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdio.h>
#include <fcntl.h>

#include "hxroot.h"

PRIVATE char *HxRoot = 0;
PRIVATE size_t HxRootLen = 0;
PRIVATE char *HxBinds[16] = {0};
PRIVATE int HxBindsLen = -1;
PRIVATE bool HxDebug = false;
PRIVATE char *HxLinker = 0;
PRIVATE bool HxL2s = false;
PRIVATE char *HxProot = 0;
PRIVATE int HxUid = -1;
PRIVATE int HxGid = -1;

PRIVATE char **HxEnviron = 0;
PRIVATE char *HxExe = 0;
PRIVATE int HxExeLen = 0;

extern char **environ;

static int simple_atoi(const char *s) {
    if(!s) return 0;
    int res = 0;
    while(*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}

#include <sys/syscall.h>
#ifndef __NR_newfstatat
#define __NR_newfstatat 79
#endif

static char *(*real_realpath)(const char *path, char *resolved_path);
static char *check_dir(const char *path) {
    if(!path || !path[0]) return NULL;
    struct stat st;
    if(syscall(__NR_newfstatat, AT_FDCWD, path, &st, 0) == 0 && S_ISDIR(st.st_mode)) {
        if(!real_realpath) real_realpath = dlsym(RTLD_NEXT, "realpath");
        if(real_realpath) return real_realpath(path, NULL);
        return strdup(path);
    }
    return NULL;
}

static void HxDoInit() {
    char *root = getenv("FAKECHROOT_BASE");
    if(!root) root = getenv("HxRoot");

    if(root) {
        // If provided path contains a rootfs or ROOT subdirectory, prefer it
        char sub[PATH_MAX];
        snprintf(sub, sizeof(sub), "%s/rootfs", root);
        char *resolved = check_dir(sub);
        if(!resolved) {
            snprintf(sub, sizeof(sub), "%s/ROOT", root);
            resolved = check_dir(sub);
        }
        if(resolved) {
            HxRoot = resolved;
        } else {
            if(!real_realpath) real_realpath = dlsym(RTLD_NEXT, "realpath");
            HxRoot = real_realpath ? real_realpath(root, NULL) : strdup(root);
            if(!HxRoot) HxRoot = strdup(root);
        }
    } else {
        // Auto-detect container root: rootfs as default, ROOT as fallback
        char *resolved = check_dir("rootfs");
        if(!resolved) resolved = check_dir("../rootfs");
        if(!resolved) resolved = check_dir("ROOT");
        if(!resolved) resolved = check_dir("../ROOT");

        if(resolved) {
            HxRoot = resolved;
            setenv("FAKECHROOT_BASE", HxRoot, 1);
            setenv("HxRoot", HxRoot, 1);
        }
    }

    if(HxRoot) {
        HxRootLen = strlen(HxRoot);
    }

    char *binds = getenv("FAKECHROOT_EXCLUDE_PATH");
    if(!binds) binds = getenv("HxBinds");
    if(!binds) {
        struct stat st_glibc;
        if(syscall(__NR_newfstatat, AT_FDCWD, "/data/data/com.termux/files/usr/glibc", &st_glibc, 0) == 0) {
            binds = "/dev:/proc:/sys:/data/data/com.termux/files/usr/glibc";
        } else {
            binds = "/dev:/proc:/sys";
        }
    }

    if(binds) {
        binds = strdupa(binds);

        char *saveptr = NULL;
        char *tok = strtok_r(binds, ": \n", &saveptr);
        int i = 0;
        while(tok != NULL && i < 16) {
            HxBinds[i] = strdup(tok);
            tok = strtok_r(NULL, ": \n", &saveptr);
            i += 1;
        }

        HxBindsLen = i;
    }

    char *dbg = getenv("FAKECHROOT_DEBUG");
    if(!dbg) dbg = getenv("HxDebug");
    if(dbg && simple_atoi(dbg) == 1) HxDebug = true;

    char *linker = getenv("HxLinker");
    if(linker) HxLinker = strdup(linker);

    char *uid = getenv("HxUid");
    if(uid) HxUid = simple_atoi(uid);
    else if(HxRoot || getenv("FAKECHROOT") || getenv("FAKECHROOT_BASE")) HxUid = 0;

    char *gid = getenv("HxGid");
    if(gid) HxGid = simple_atoi(gid);
    else if(HxRoot || getenv("FAKECHROOT") || getenv("FAKECHROOT_BASE")) HxGid = 0;

    char *l2s = getenv("HxL2s");
    if(l2s) HxL2s = (simple_atoi(l2s) == 1);
    else HxL2s = true; // default link2symlink to true for container hygiene

    char *proot = getenv("HxProot");
    if(proot) HxProot = strdup(proot);

    size_t size = HxCountArgv(environ)+1;
    HxEnviron = calloc(size, sizeof(char*));
    memcpy(HxEnviron, environ, size*sizeof(char*));
}

static pthread_once_t HxOnce = PTHREAD_ONCE_INIT;

__attribute__((constructor))
PRIVATE void HxInit() {
    pthread_once(&HxOnce, HxDoInit);
}
