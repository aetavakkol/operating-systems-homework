# BBM 342: Operating Systems (Spring 2017) - Experiment 2

**Subject:** Multi-Thread Programming, Inter Process Communication, File System Calls

_Disclaimer:_ If you have similar homework/project, do not copy paste directly.
Try to solve yourself.

## Description
A parallel file search system to practice fork&exec, pipe, POSIX threads, and semaphores/mutexes.

## Compile & Run
```bash
make
cd build/
./main <minion_count> <buffer_size> <search_query> <search_path>
```

### Parameters
- `<minion_count>` number of the minion processes

- `<buffer_size>` size (number of elements) of the file buffer

- `<search_query>` search query

- `<search_path>` search path

## Clean up
```bash
make clean
```

## Test

Tested on;
- gcc 5.4.0 20160609 (Ubuntu 5.4.0-6ubuntu1~16.04.4) [4 cores / 8 threads]
- gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-11) [4 cores / 4 threads]
