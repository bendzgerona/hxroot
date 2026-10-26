#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "hxroot.h"

int chroot(const char *path) {
    HxInit();
    if (!path) {
        errno = EFAULT;
        return -1;
    }

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    char real[PATH_MAX];
    if (!realpath(new_path, real)) {
        return -1;
    }

    if (HxRoot) free(HxRoot);
    HxRoot = strdup(real);
    HxRootLen = strlen(HxRoot);

    setenv("HxRoot", HxRoot, 1);
    setenv("FAKECHROOT_BASE", HxRoot, 1);

    if (HxDebug) {
        eprintf("chroot(\"%s\" -> \"%s\")\n", path, HxRoot);
    }

    return 0;
}
