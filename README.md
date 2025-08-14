# Mini Filesystem (MFS) in C

A tiny teaching filesystem with block I/O plus a demo shell (`fsshell`) that exercises common file and directory operations.

# Author: Ranjiv Jithendran

# Email: rjithendran@sfsu.edu / ranjiv.jithendran@gmail.com

- **Block size:** 512 bytes  
- **Typical volume size (for demos/tests):** 500,000 – 10,000,000 bytes  
- Public API is declared in `mfs.h` (you may adapt it for your design, especially `fdDir`).

---

## ✨ Features

- **Block I/O** via `LBAread` / `LBAwrite`
- Basic **file** and **directory** operations
- Interactive shell (`fsshell`) with commands like `ls`, `cp`, `mv`, `md`, `rm`, `touch`, `cat`, `cd`, `pwd`, `cp2l`, `cp2fs`, `history`, `help`
- Pluggable on-disk layout (implement your own metadata structures)

---

## 🧱 Block I/O API

```c
#include <stdint.h>

uint64_t LBAwrite(void *buffer, uint64_t lbaCount, uint64_t lbaPosition);
uint64_t LBAread (void *buffer, uint64_t lbaCount, uint64_t lbaPosition);
