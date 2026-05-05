#include "get_syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *syscall_map[512] = {0};

void load_syscall_map()
{  
     FILE *f=fopen("/usr/include/x86_64-linux-gnu/asm/unistd_64.h", "r");
    if(f==NULL)
    {
        return;
    }
    char line[256];
    while(fgets(line,sizeof(line),f))
    {
      char name[128]; 
      int n;
      if(sscanf(line,"#define __NR_%s %d",name,&n)==2)
      {
        if(n>=0 && n<512)
         syscall_map[n]=strdup(name);
      }

    }
    fclose(f);

}
const char* get_syscall(int sc_no)
{
    if(sc_no>=0 && sc_no <512 && syscall_map[sc_no])
    {
        return syscall_map[sc_no];
    }
    return "could not get syscall name";
    
}