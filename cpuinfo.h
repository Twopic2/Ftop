#ifndef CPUINFO_H
#define CPUINFO_H

#include <sys/sysinfo.h>


struct cacheinfo {
    int level;
    int sizeKB;
    int id;
    char type[32]; 
};
struct isaInfo {

    char isaSet[32]; 
    char description[];

};

extern long show_uptime();

int catCache(struct cacheinfo *cacheArray, int max_entries);

extern void cacheusage(int row, int col);

int catISA(struct isaInfo *isaArray, int max_entries);

extern void displayISAInfo(int row, int col);

#endif
