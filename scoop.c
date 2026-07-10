#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <stddef.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include "get_syscall.h"
#include "clean_print.h"

static pid_t child_pid = -1;

void toggle_ftrace(const char* file, const char* value);
void clear_trace();

void handle_sigint(int sig) {
    (void)sig;
    printf("\n[!] Caught SIGINT. Cleaning up trace state...\n");
    if (child_pid > 0) {
        kill(child_pid, SIGTERM);
    }
    toggle_ftrace("tracing_on", "0");
    clear_trace();
    exit(EXIT_FAILURE);
}

void toggle_ftrace(const char* file, const char* value)
{
    char path[256];
    if ((size_t)snprintf(path, sizeof(path), "/sys/kernel/debug/tracing/%s", file) >= sizeof(path)) {
        fprintf(stderr, "Error: Path truncated for %s\n", file);
        return;
    }
    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", path, strerror(errno));
        return;
    }
    if (write(fd, value, strlen(value)) < 0) {
        fprintf(stderr, "Error writing to %s: %s\n", path, strerror(errno));
    }
    if (close(fd) < 0) {
        fprintf(stderr, "Error closing %s: %s\n", path, strerror(errno));
    }
}
void clear_trace() 
{
    const char *path = "/sys/kernel/debug/tracing/trace";
    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", path, strerror(errno));
        return;
    }
    if (close(fd) < 0) {
        fprintf(stderr, "Error closing %s: %s\n", path, strerror(errno));
    }
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS] <program> [args...]\n", prog_name);
    printf("Trace system calls and kernel functions (ftrace) of a given program.\n\n");
    printf("Options:\n");
    printf("  -h, --help    Show this help message and exit\n");
}

int main(int argc, char *argv[]){

    int opt;
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: Expected program to trace.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (geteuid() != 0) {
        fprintf(stderr, "Error: scoop requires root privileges to configure ftrace. Please run with sudo.\n");
        return EXIT_FAILURE;
    }

    // Register signal handler for clean exit
    signal(SIGINT, handle_sigint);

    load_syscall_map();
    pid_t cpid = fork();
    if (cpid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }
    
    child_pid = cpid; // For signal handler
    
    if(cpid == 0)
    {
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
            perror("ptrace PTRACE_TRACEME");
            exit(EXIT_FAILURE);
        }
        execvp(argv[optind], &argv[optind]);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else
    {
        int status;
        struct user_regs_struct registers;
        int is_entry = 1;
        char pid_str[16];
        if ((size_t)snprintf(pid_str, sizeof(pid_str), "%d", cpid) >= sizeof(pid_str)) {
            fprintf(stderr, "Error: PID string truncated\n");
            return EXIT_FAILURE;
        }
        toggle_ftrace("set_ftrace_pid", pid_str);
        toggle_ftrace("options/funcgraph-irqs", "0");
        clear_trace();
        toggle_ftrace("current_tracer", "function_graph");
        toggle_ftrace("tracing_on", "0");
        printf("debugging pid:%d\n", cpid);
        if (waitpid(cpid, &status, 0) < 0) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
        
        // Configure ptrace to distinguish syscall stops from normal signal stops
        if (ptrace(PTRACE_SETOPTIONS, cpid, 0, PTRACE_O_TRACESYSGOOD) < 0) {
            perror("ptrace PTRACE_SETOPTIONS");
            return EXIT_FAILURE;
        }
        
        while(1)
        {
            if (ptrace(PTRACE_SYSCALL, cpid, NULL, NULL) < 0) {
                perror("ptrace PTRACE_SYSCALL");
                break;
            }
            if (waitpid(cpid, &status, 0) < 0) {
                perror("waitpid");
                break;
            }
            if(WIFEXITED(status) || WIFSIGNALED(status))
              break;
            
            // Only process the toggle if it's genuinely a syscall stop (indicated by the 0x80 bit)
            if (WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | 0x80)) {
                if (ptrace(PTRACE_GETREGS, cpid, NULL, &registers) < 0) {
                    perror("ptrace PTRACE_GETREGS");
                    break;
                }
                if(is_entry)
                {
                    const char* syscall_name = get_syscall(registers.orig_rax);
                    if (!syscall_name) {
                        syscall_name = "unknown";
                    }
                    printf("\n>>> [USER] Syscall: %s", syscall_name);

                    char filter[128];
                    if ((size_t)snprintf(filter, sizeof(filter), "*sys_*%s", syscall_name) >= sizeof(filter)) {
                        fprintf(stderr, "Error: filter string truncated for %s\n", syscall_name);
                    } else {
                        toggle_ftrace("set_graph_function", filter); 
                    }

                    toggle_ftrace("tracing_on", "1");
                    is_entry = 0;
                }
                else
                {
                    toggle_ftrace("tracing_on", "0"); 
                    print_shadow_trace();             
                    is_entry = 1;
                }
            }
        }

    }
    return EXIT_SUCCESS;
}