# 🍨 Scoop Tracer

Ever wondered what the Linux kernel is *actually* doing when you run a program? 

Scoop is a fun little tool that peeks under the hood for you. It pauses your program right before it talks to the kernel, records exactly what the kernel does in the background, and prints it out as a beautiful, easy-to-read tree.

## ⚙️ How it works

```mermaid
sequenceDiagram
    participant P as Target Program
    participant S as Scoop
    participant K as Linux Kernel

    Note over P,K: 1. Syscall Entry
    P->>S: Program triggers a System Call
    S->>K: Scoop enables kernel tracing

    Note over K: 2. Kernel Execution
    K-->>K: Kernel performs the requested operation
    K-->>K: Kernel records its internal function graph
    
    Note over P,K: 3. Syscall Exit
    P->>S: System Call finishes
    S->>K: Scoop disables kernel tracing
    S->>S: Scoop prints the formatted trace graph
```

## 🚀 Try it out

**1. Build it**
```bash
make
sudo make install
```

**2. Trace something** (You need `sudo` because we are looking at real kernel memory!)
```bash
# Trace a standard command
sudo scoop ls

# Or trace your own code
sudo scoop ./my_program
```

## 🎥 Demo

*[Insert your awesome demo video here!]*
