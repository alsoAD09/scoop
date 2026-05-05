#ifndef GET_SYSCALL_H
#define GET_SYSCALL_H

void load_syscall_map();
const char* get_syscall(int sc_no);
#endif
