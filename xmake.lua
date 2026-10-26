-- xmake.lua for hxroot / libfakechroot container interception runtime
-- Target: Termux Android aarch64 (bphone/nabu mesh)

set_project("hxroot")
set_version("0.2.0")
set_license("LGPL-2.1")
set_description("High-performance unprivileged container virtualization runtime with SIGSYS filtering")

add_rules("mode.release", "mode.debug")

-- Deployment directory defaults to ~/.local
local prefix = path.join(os.getenv("HOME"), ".local")
local bindir = path.join(prefix, "bin")
local libdir = path.join(prefix, "lib")

target("libfakechroot")
    set_kind("shared")
    set_filename("libfakechroot.so")
    add_files("src/*.c")
    add_includedirs("src")
    
    on_build(function (target)
        local out = target:targetfile()
        os.mkdir(path.directory(out))
        print("==> Compiling libfakechroot.so with zig cc (aarch64-linux-gnu)...")
        os.execv("zig", {"cc", "-target", "aarch64-linux-gnu", "-shared", "-fPIC", "-O2", "-Isrc", "src/access.c", "src/attr.c", "src/audit.c", "src/chmod.c", "src/chown.c", "src/chroot.c", "src/dlopen.c", "src/elfutil.c", "src/exec.c", "src/fixup_argv.c", "src/getcwd.c", "src/getuid.c", "src/glob.c", "src/group.c", "src/hxroot.c", "src/ldlib.c", "src/link.c", "src/mkdir.c", "src/mkfifo.c", "src/mktemp.c", "src/open.c", "src/opendir.c", "src/passwd.c", "src/posix_spawn.c", "src/prctl.c", "src/readlink.c", "src/realpath.c", "src/rename.c", "src/scandir.c", "src/setgroups.c", "src/setmntent.c", "src/setuid.c", "src/shadow.c", "src/sigsys.c", "src/socket.c", "src/stat.c", "src/statvfs.c", "src/syscall.c", "src/sysvsem.c", "src/unlink.c", "src/util.c", "src/utime.c", "src/wordexp.c", "src/xattr.c", "-o", out})
        
        local hxroot_so = path.join(path.directory(out), "libhxroot.so")
        os.rm(hxroot_so)
        os.ln(out, hxroot_so)
        print("==> Built: " .. out .. " and symlink " .. hxroot_so)
    end)

target("install")
    set_kind("phony")
    set_default(false)
    add_deps("libfakechroot")
    on_build(function ()
        os.mkdir(bindir)
        os.mkdir(libdir)

        -- Deploy shared objects from xmake build directory
        local lib_src = path.join(os.projectdir(), "build", "linux", "arm64", "release", "libfakechroot.so")
        if not os.isfile(lib_src) then
            lib_src = path.join(os.projectdir(), "build", "aarch64", "release", "libfakechroot.so")
        end
        if not os.isfile(lib_src) then
            lib_src = path.join(os.projectdir(), "libfakechroot.so")
        end

        if os.isfile(lib_src) then
            local lib_dst = path.join(libdir, "libfakechroot.so")
            os.cp(lib_src, lib_dst)
            os.rm(path.join(libdir, "libhxroot.so"))
            os.ln(lib_dst, path.join(libdir, "libhxroot.so"))
            print("==> Deployed: " .. lib_dst .. " and libhxroot.so")
        end

        -- Deploy CLI launchers
        local bin_src = path.join(os.projectdir(), "fakechroot")
        local bin_dst = path.join(bindir, "fakechroot")
        os.cp(bin_src, bin_dst)
        os.execv("chmod", {"+x", bin_dst})
        os.rm(path.join(bindir, "hxroot"))
        os.ln(bin_dst, path.join(bindir, "hxroot"))
        print("==> Deployed: " .. bin_dst .. " and hxroot")

        -- Deploy conversion utilities
        for _, tool in ipairs({"hxconvert", "hxunconvert"}) do
            local src = path.join(os.projectdir(), tool)
            if os.isfile(src) then
                local dst = path.join(bindir, tool)
                os.cp(src, dst)
                os.execv("chmod", {"+x", dst})
                print("==> Deployed: " .. dst)
            end
        end
        print("==> Installation complete in " .. prefix)
    end)
