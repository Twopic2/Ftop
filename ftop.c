#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <sys/statvfs.h>
#include <signal.h>

#include "disk.h"
#include "cpuinfo.h"

#define MAX_CORES 128
#define CORES_PER_COLUMN 10
#define BAR_WIDTH 20
#define PROCESS_DISPLAY 25

typedef struct {
    int pid;
    char name[256];
    float cpu;
} Process;

typedef struct {
    float prevIdle;
    float prevTotal;
} coreStat;

typedef struct {
    float totalGB;
    float usedGB;
    float percentage;
} MemoryStats;

int coreAmount = 0;
coreStat cores[MAX_CORES];

void amountCores() {
    FILE *fp = fopen("/proc/stat", "r");
    char buf[999];
    coreAmount = 0;

    while (fgets(buf, sizeof(buf), fp)) {
        if (strncmp(buf, "cpu", 3) == 0 && isdigit(buf[3])) {
            coreAmount++;
        } else if (strncmp(buf, "intr", 4) == 0) {
            break;
        } 
    }
    fclose(fp);
}

void coreUsagefunc(float *usage) {
    FILE *fp = fopen("/proc/stat", "r");
    char buf[999];
    int core = 0;

    while (fgets(buf, sizeof(buf), fp)) {
        if (strncmp(buf, "cpu", 3) == 0 && isdigit(buf[3])) {
            long user, nice, system, idle, iowait, irq, softirq, steal;
            sscanf(buf, "cpu%*d %ld %ld %ld %ld %ld %ld %ld %ld",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);

            long idle_time = idle + iowait;
            long total_time = user + nice + system + idle + iowait + irq + softirq + steal;

            long delta_idle = idle_time - cores[core].prevIdle;
            long delta_total = total_time - cores[core].prevTotal;

            cores[core].prevIdle = idle_time;
            cores[core].prevTotal = total_time;

            usage[core] = (delta_total == 0) ? 0 : 100.0f * (delta_total - delta_idle) / delta_total;
            core++;
        }
    }
    fclose(fp);
}

MemoryStats memStats() {
    MemoryStats stats = {0.0f, 0.0f, 0.0f};
    FILE *fp = fopen("/proc/meminfo", "r");
    
    long total = 0, free = 0, buffers = 0, cached = 0, swap = 0;
    char label[64];
    long value;

    while (fscanf(fp, "%s %ld", label, &value) != EOF) {
        if (strcmp(label, "MemTotal:") == 0) {
            total = value;
        } else if (strcmp(label, "MemFree:") == 0) {
            free = value;
        } else if (strcmp(label, "Buffers:") == 0) {
            buffers = value;
        } else if (strcmp(label, "Cached:") == 0) {
            cached = value;
        }
    }
    fclose(fp); 

    long used = total - free - buffers - cached;
    
    stats.totalGB = (float)total / 1048576.0;
    stats.usedGB = (float)used / 1048576.0;

    stats.percentage = (100.f * used) / total;
 
    return stats;
}

int processID(Process *procs, int max) {
    DIR *dir = opendir("/proc");
    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < max) {
        if (!isdigit(entry->d_name[0])) continue;

        int pid = atoi(entry->d_name);
        char location[256];
        snprintf(location, sizeof(location), "/proc/%d/stat", pid);
        FILE *fp = fopen(location, "r");

        int dummy;
        char name[256];
        char state;
        long utime, stime;
        
        fscanf(fp, "%d (%[^)]s) %c", &dummy, name, &state);
        
        for (int i = 0; i < 11; i++) {
            fscanf(fp, "%*s");
        }
        fscanf(fp, "%ld %ld", &utime, &stime);
        fclose(fp);

        procs[count].pid = pid;
        strncpy(procs[count].name, name, 255);
        procs[count].cpu = (float)(utime + stime) / sysconf(_SC_CLK_TCK);
        count++;
    }

    closedir(dir);
    return count;
}

int compare_cpu(const void *a, const void *b) {
    float diff = ((Process *)b)->cpu - ((Process *)a)->cpu;
    return (diff > 0) - (diff < 0);
}

// percentage of usage
void chart(int y, int x, const char *label, float percent) {
    int filled = (int)(BAR_WIDTH * percent / 100.0);
    mvprintw(y, x, "%s [", label);
    for (int i = 0; i < BAR_WIDTH; i++) {
        if (i < filled) {
            addch('=');
        } else {
            addch(' ');
        }
    }   
    printw("] %5.1f%%", percent);
}

void processDisplay(Process *procs, int count, int scroll, int proc_row) {

    mvprintw(proc_row, 0, " PID   CPU  NAME");

    int display_count;

    if (count < PROCESS_DISPLAY) {
        display_count = count;
    }

    if (count >= PROCESS_DISPLAY) {
        display_count = PROCESS_DISPLAY;

    }

    for (int i = 0; i < display_count && (i + scroll) < count; i++) {
        int id = i + scroll;
        mvprintw(proc_row + 1 + i, 0, "%5d  %5.1f  %s", procs[id].pid, procs[id].cpu, procs[id].name);
    }
}

int main() {
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    cbreak();
    timeout(1000);

    curs_set(FALSE);

    amountCores();
    Process procs[256];   
    float core_usages[MAX_CORES]; 

    int scroll = 0;
    int count = 0;

    while (1) {
        clear();

        mvprintw(0, 0, "Welcome to Ftop");
    
        coreUsagefunc(core_usages);

        MemoryStats mem_stats = memStats();

        long uptime = show_uptime();

        float disk = diskUsage("/");
        
        long total_disk = diskTotal("/");

        int base_row = 2;

        for (int i = 0; i < coreAmount; i++) {
            int col = i / CORES_PER_COLUMN;
            int row = i % CORES_PER_COLUMN;
            int x = col * 50;
            int y = base_row + row;

            char label[16];
            snprintf(label, sizeof(label), "CPU %02d", i);
            chart(y, x, label, core_usages[i]);
        }

        int mem_row = base_row + CORES_PER_COLUMN + 1;
        chart(mem_row, 0, "Memory", mem_stats.percentage);
        mvprintw(mem_row, 50 , "%.2f GB / %.2f GB", mem_stats.usedGB, mem_stats.totalGB);

        int disk_row = mem_row + 2;
        chart(disk_row, 0, "Disk Usage", disk);

        int diskTotal_row = disk_row + 2; 
        mvprintw(diskTotal_row, 0, "Root %ld GB", total_disk);

        int cache_row = diskTotal_row + 2;
        cacheusage(cache_row, 0);

        int count = processID(procs, 256);
        qsort(procs, count, sizeof(Process), compare_cpu);

        int proc_row = cache_row + 5;
        processDisplay(procs, count, scroll, proc_row);

        int isa_row = proc_row ;
        displayISAInfo(isa_row, 100);

        int ch = getch();

        switch(ch) {
            case KEY_UP:
                if (scroll > 0) {
                    scroll--;
                }
            break;
            case KEY_DOWN:
                if (scroll + PROCESS_DISPLAY < count) {
                    scroll++;
                }
        }

        int uptime_row = proc_row;
        mvprintw(uptime_row, 25, "System uptime: %ld seconds", uptime);

        refresh();
        usleep(1000000);
    }

    endwin();
    //End Ftop
    return 0;
}
