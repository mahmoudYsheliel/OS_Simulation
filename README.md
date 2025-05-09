
## Project Overview

This project simulates how an operating system schedules processes. A **process generator** sends data to a **scheduler** using **shared memory** and **semaphores** to coordinate and share information safely.

## How It Works

### Communication (IPC)
- A 64-byte shared memory buffer is used to send one process at a time.
- A semaphore is used to control access.
- Signals (`SIGINT`, `SIGCHLD`) help notify between generator and scheduler.

### Scheduling
- The scheduler supports different algorithms.
- It stores processes in data structures and decides their order of execution.

## Data Structures
- **Dynamic Array List**
- **Priority Queue**
- **Hash Map**

## Execution Steps
1. Process generator sends data.
2. Scheduler reads and stores the process.
3. Scheduler starts scheduling while receiving more data.
4. When a process ends, scheduler cleans it up.
5. Final stats and logs are printed.

## Output Files
- `scheduler.log` – Scheduler actions
- `scheduler.perf` – Performance info
- `memory.log` – Memory usage

## Work Plan
- Plan and build IPC
- Implement scheduling algorithms
- Test memory management
- Integrate everything and test
- Write final report
"""
