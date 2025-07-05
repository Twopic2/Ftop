#ifndef MEMALLOC_H
#define MEMALLOC_H

#include <stdlib.h>

typedef struct {
    int pid;
    char name[256];
    float cpu;
    float mem;
} Process;

typedef struct {
    Process *processes;
    int count;
    int capacity;
} ProcessArray;

typedef struct {
    float prevIdle;
    float prevTotal;
} coreStat;

ProcessArray* initProcessArray(int cap);
int resizeProcessArray(ProcessArray *arr, int cap);
void freeProcessArray(ProcessArray *arr);

coreStat* initCoreStats(int core);
void freeCoreStats(coreStat *cores);

char* safeMallocString(size_t size);
void safeFreeString(char *str);

char* createBuffer(size_t size);
void freeBuffer(char *buffer);

void registerCleanup_handler(void);
void cleanupAllMemory(void);

extern void (*memoryCleanupFunc)(void);

#endif