# 🍨 Scoop Tracer

**Scoop** is a powerful hybrid system call tracer that helps you understand the under-the-hood flow of any Linux program. It is built natively on top of the `ptrace()` system call and the Linux kernel's internal `ftrace` framework.

---


---

## ⚙️ How It Works (The Workflow)
Scoop bridges the gap between user-space and kernel-space execution by seamlessly orchestrating two powerful Linux tracing facilities:

1. **`ptrace()` Interception:** Scoop uses `ptrace` to pause the target program exactly at the boundary of a system call (entry and exit).
2. **`ftrace` Injection:** When a system call begins, Scoop reads the syscall type, dynamically writes it into the kernel's `set_graph_function` filter, and toggles `tracing_on`.
3. **Execution & Capture:** The kernel executes the system call while the `function_graph` tracer records the deep, nested execution path. 
4. **Clean Print:** Once the syscall exits, Scoop turns off tracing, reads the raw trace buffer, filters out internal kernel scheduling noise, and prints a beautiful, indented execution tree directly to your terminal.

---

## ✨ Features
- **Precise Synchronization:** Identifies genuine system calls using `PTRACE_O_TRACESYSGOOD` to eliminate background noise (like context switches and generic signal stops).
- **Self-Healing:** Automatically wipes dirty kernel trace buffers and resets filters on startup or graceful exit (`Ctrl+C`).
- **Robust Argument Passing:** Easily traces target commands with multiple arguments (e.g., `scoop ls -la`).
- **Deep Kernel Visualization:** Shows you exactly which drivers (like `ext4`) and sub-systems are triggered by your code.

---

## 🛠️ Installation

Building and installing Scoop is simple:

```bash
# Compile the tool
make

# Install globally to /usr/bin/
sudo make install
```

---

## 🚀 Usage

Scoop configures kernel memory via `/sys/kernel/debug/tracing`, so it **must be run as root**.

```bash
# View the help menu
scoop -h

# Trace a standard system utility
sudo scoop ls 

# Trace your own custom compiled binary
sudo scoop ./my_program
```


