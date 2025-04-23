#ifndef CPUINFO_H
#define CPUINFO_H

#include <sys/sysinfo.h>


struct cacheinfo {
    int level;
    int sizeKB;
    int id;
    char type[32]; 
};

long show_uptime();

int catCache(struct cacheinfo *cacheArray, int max_entries);

void cacheusage(int row, int col);

#endif
