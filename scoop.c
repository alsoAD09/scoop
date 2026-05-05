#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <stddef.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include "get_syscall.h"
#include "clean_print.h"


void toggle_ftrace(const char* file,const char* value )
{
    char path[256];
    sprintf(path,"/sys/kernel/debug/tracing/%s",file);
    int fd=open(path,O_WRONLY);
    if(fd>=0)
    {
        write(fd,value,strlen(value));
        close(fd);
    }
}
void clear_trace() 
{
    int fd = open("/sys/kernel/debug/tracing/trace", O_WRONLY | O_TRUNC);
    if (fd >= 0) {
        close(fd);
    }
}

int main(int argc, char *argv[]){
    load_syscall_map();
    if(argc<2)
    {
        printf("usage: %s <program>\n",argv[0]);
        return 1;
    }
    pid_t cpid=fork();
    if(cpid==0)
    {
        ptrace(PTRACE_TRACEME,0,NULL,NULL);
        execvp(argv[1],&argv[1]);
    }
    else
    {
        int status;
        struct user_regs_struct registers;
        int is_entry=1;
        char pid_str[16];
        sprintf(pid_str,"%d",cpid);
        toggle_ftrace("set_ftrace_pid",pid_str);
        toggle_ftrace("options/funcgraph-irqs", "0");
        clear_trace();
        toggle_ftrace("current_tracer","function_graph");
        toggle_ftrace("tracing_on","0");
        printf("debugging pid:%d\n",cpid);
        waitpid(cpid,&status,0);
        
        while(1)
        {
            ptrace(PTRACE_SYSCALL,cpid,NULL,NULL);
            waitpid(cpid,&status,0);
            if(WIFEXITED(status))
              break;
            ptrace(PTRACE_GETREGS,cpid,NULL,&registers);
            if(is_entry)
            {
                const char* syscall_name = get_syscall(registers.orig_rax);
                printf("\n>>> [USER] Syscall: %s", syscall_name);

                char filter[128];
                sprintf(filter, "*sys_*%s", syscall_name);
                toggle_ftrace("set_graph_function", filter); 

                toggle_ftrace("tracing_on","1");
                is_entry=0;
            }
            else
            {
                toggle_ftrace("tracing_on", "0"); 
                print_shadow_trace();             
                is_entry = 1;

            }
        }

    }
    return 0;
}