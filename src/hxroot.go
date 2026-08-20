// ============================================================================
// ARCHITECTURAL SPECIFICATION & AI AGENT GUIDE: hxroot (Go Port Study)
// ============================================================================
// Purpose: Prototype study for porting hxroot launcher/wrapper logic to Go,
// providing a statically compiled, high-performance execution frontend with
// clean flag parsing (-h, --help), environment sanitization, and fallback orchestration.
// ============================================================================

package main

import (
	"fmt"
	"os"
	"os/exec"
	"syscall"
)

func printHelp() {
	fmt.Println("hxroot (Go Port Prototype)")
	fmt.Println("Usage: hxroot [options] [--] [command...]")
	fmt.Println("")
	fmt.Println("Options:")
	fmt.Println("  -h, --help     Display this help message and exit")
	fmt.Println("  --proot-fallback Enable automatic proot fallback")
	os.Exit(0)
}

func main() {
	if len(os.Args) > 1 && (os.Args[1] == "-h" || os.Args[1] == "--help") {
		printHelp()
	}

	// Sanitize bionic environment pollution
	os.Unsetenv("LD_PRELOAD")
	os.Unsetenv("LD_LIBRARY_PATH")

	glibcPrefix := "/data/data/com.termux/files/usr/glibc"
	path := glibcPrefix + "/bin:" + os.Getenv("PATH")
	libPath := glibcPrefix + "/lib"

	os.Setenv("PATH", path)
	os.Setenv("LD_LIBRARY_PATH", libPath)

	args := os.Args[1:]
	if len(args) == 0 {
		args = []string{glibcPrefix + "/bin/bash"}
	}

	binary, err := exec.LookPath(args[0])
	if err != nil {
		fmt.Fprintf(os.Stderr, "hxroot: command not found: %s\n", args[0])
		os.Exit(1)
	}

	err = syscall.Exec(binary, args, os.Environ())
	if err != nil {
		fmt.Fprintf(os.Stderr, "hxroot: exec failed: %v\n", err)
		os.Exit(1)
	}
}
