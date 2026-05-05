#include "clean_print.h"
#include "scoop.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
static int current_depth = 0;
static int ignore_depth = 0;
const char *noise_filter[] = {
    "tick_", "tmigr_", "smp_", "apic_", "irq_", "hrtimer_", "ktime_",
    "account_", "update_", "cpuacct_", "sched_", "cond_resched", "preempt_", "might_sleep", "idle",
    "rcu_", "spin_", "mutex_", "_raw_spin_", "up_", "down_",
    "audit_", "apparmor_", "security_", "fsnotify_"
};

void scoop_clean_and_print(char *raw_line) {
    char *pipe_ptr = strchr(raw_line, '|');
    if (!pipe_ptr) return;
    
    char *function_data = pipe_ptr + 2;
    
    // Ignore pure formatting lines (like | | |) that don't contain function calls or closures
    if (!strchr(function_data, '(') && !strchr(function_data, '}')) {
        return;
    }
    
    int is_open = (strchr(function_data, '{') != NULL);
    int is_close = (strchr(function_data, '}') != NULL);

    if (is_close) {
        if (ignore_depth > 0) {
            ignore_depth--;
            return;
        } else if (current_depth > 0) {
            current_depth--;
        }
        return; 
    }

    if (ignore_depth > 0) {
        if (is_open) ignore_depth++;
        return;
    }

    int num_filters = sizeof(noise_filter) / sizeof(noise_filter[0]);
    for (int i = 0; i < num_filters; i++) {
        if (strstr(function_data, noise_filter[i])) {
            if (is_open) ignore_depth++;
            return;
        }
    }
   
    function_data[strcspn(function_data, "\n")] = 0;
    printf("%s\n", function_data);
    if (is_open) {
        current_depth++;
    }
}
void print_shadow_trace(){
    FILE *trace=fopen("/sys/kernel/debug/tracing/trace","r");
    if(!trace)
    {
        perror("Failed to open trace");
        return;
    }
    int fd=fileno(trace);
    int flags=fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,flags | O_NONBLOCK);
    
    char line[1024];
    current_depth=0;
    printf("\n--- [KERNEL SHADOW TRACE] ---\n");
    while (fgets(line,sizeof(line),trace))
    {
       scoop_clean_and_print(line);
    } 
    clear_trace();
}