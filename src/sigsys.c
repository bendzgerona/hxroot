#include <ucontext.h>
#include <signal.h>
#include <stddef.h>

#define SYS_SECCOMP      1
#define NR_rt_sigaction  134
#define SIGSYS            31
#define KERNEL_SIGSETSIZE 8

/* --- syscall numbers we emulate on Android aarch64 --- */
#define __NR_set_robust_list         274
#define __NR_rseq                    293
#define __NR_setgid                  144
#define __NR_setuid                  146
#define __NR_setfsuid                150
#define __NR_setfsgid                151
#define __NR_io_uring_setup          425
#define __NR_io_uring_enter          426
#define __NR_io_uring_register       427
#define __NR_clone3                  435
#define __NR_faccessat2              439
#define __NR_landlock_create_ruleset 444
#define __NR_landlock_add_rule       445
#define __NR_landlock_restrict_self  446
#define __NR_faccessat               48

/* Raw rt_sigaction direct syscall avoiding libc wrapper */
static long raw_rt_sigaction(int sig, const struct sigaction *act,
                             struct sigaction *oldact, size_t sigsetsize)
{
    register long x0 __asm__("x0") = sig;
    register const struct sigaction *x1 __asm__("x1") = act;
    register struct sigaction *x2 __asm__("x2") = oldact;
    register size_t x3 __asm__("x3") = sigsetsize;
    register long x8 __asm__("x8") = NR_rt_sigaction;
    __asm__ volatile(
        "svc #0"
        : "+r"(x0)
        : "r"(x1), "r"(x2), "r"(x3), "r"(x8)
        : "memory"
    );
    return x0;
}

static struct sigaction old_sa;

/* The SIGSYS interception handler for seccomp-blocked syscalls */
static void sigsys_handler(int sig, siginfo_t *info, void *uctx)
{
    /* Forward non-seccomp signals */
    if (info->si_code != SYS_SECCOMP) {
        if (old_sa.sa_flags & SA_SIGINFO) {
            void (*fn)(int, siginfo_t *, void *) = old_sa.sa_sigaction;
            if (fn) fn(sig, info, uctx);
        } else if (old_sa.sa_handler && old_sa.sa_handler != SIG_DFL
                                    && old_sa.sa_handler != SIG_IGN) {
            old_sa.sa_handler(sig);
        }
        return;
    }

    ucontext_t *uc = (ucontext_t *)uctx;
    unsigned long long *regs = uc->uc_mcontext.regs;
    unsigned long long *pc   = &uc->uc_mcontext.pc;
    long nr = (long)regs[8];

    switch (nr) {
    /* ---- return 0 (success) group ---- */
    case __NR_set_robust_list:
    case __NR_setgid:
    case __NR_setuid:
    case __NR_setfsuid:
    case __NR_setfsgid:
    case 145:   /* setresgid */
    case 143:   /* setresuid */
        regs[0] = 0;
        *pc += 4;
        return;

    /* ---- return -ENOSYS group ---- */
    case __NR_clone3:
    case __NR_rseq:
    case __NR_landlock_create_ruleset:
    case __NR_landlock_add_rule:
    case __NR_landlock_restrict_self:
    case __NR_io_uring_setup:
    case __NR_io_uring_enter:
    case __NR_io_uring_register:
        regs[0] = (unsigned long long)(-38);   /* -ENOSYS */
        *pc += 4;
        return;

    /* ---- faccessat2 -> faccessat rewrite ---- */
    case __NR_faccessat2:
        regs[8] = __NR_faccessat;  /* dirfd, path, mode, flags */
        return;                     /* do NOT advance pc: re-execute svc as faccessat */

    default:
        regs[0] = (unsigned long long)(-38);
        *pc += 4;
        return;
    }
}

__attribute__((constructor))
static void sigsys_shim_init(void)
{
    struct sigaction sa;
    sa.sa_sigaction = sigsys_handler;
    sa.sa_flags     = SA_SIGINFO;
    __builtin_memset(&sa.sa_mask, 0, sizeof(sa.sa_mask));

    raw_rt_sigaction(SIGSYS, &sa, &old_sa, KERNEL_SIGSETSIZE);
}
