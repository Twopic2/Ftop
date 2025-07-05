#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ncurses.h>

#include "memalloc.h"

static ProcessArray *globalProcArray = NULL;
static coreStat *globalCores = NULL;
static char **allocatedBuffers = NULL;
static int bufferCount = 0;
static int bufferCapacity = 0;

void (*memory_cleanup_func)(void) = NULL;

static void trackBuffer(char *buffer) {
    if (bufferCount >= bufferCapacity) {
         if (bufferCapacity == 0) {
            bufferCapacity = 10;
        } else {
            bufferCapacity = bufferCapacity * 2;
        }
        char **new_buffers = realloc(allocatedBuffers, sizeof(char*) * bufferCapacity);
        if (new_buffers) {
            allocatedBuffers = new_buffers;
        } else {
            return;
        }
    }

    if (allocatedBuffers) {
        allocatedBuffers[bufferCount++] = buffer;
    }
}

static void untrack_buffer(char *buffer) {
    if (!allocatedBuffers) {
        return;
    }
    for (int i = 0; i < bufferCount; i++) {
        if (allocatedBuffers[i] == buffer) {
            for (int j = i; j < bufferCount - 1; j++) {
                allocatedBuffers[j] = allocatedBuffers[j + 1];
            }
            bufferCount--;
            break;
        }
    }
}

ProcessArray* initProcessArray(int cap) {
    ProcessArray *arr = malloc(sizeof(ProcessArray));

    arr->processes = malloc(sizeof(Process) * cap);

    arr->count = 0;
    arr->capacity = cap;
    
    globalProcArray = arr;
    return arr;
}

int resizeProcessArray(ProcessArray *arr, int cap) {
    if (!arr || cap <= arr->capacity) {
        return 1;
    }

    Process *procs = realloc(arr->processes, sizeof(Process) * cap);
    arr->processes = procs;
    arr->capacity = cap;

    return 1;
}
