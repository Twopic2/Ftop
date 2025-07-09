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
#include <locale.h>

#include "disk.h"
#include "cpuinfo.h"

#define NAME_MAX_LEN 256
#define INITIAL_SIZE 256

#define PREVENT_OVERFLOW 100000
#define MAX_CORES 128
#define CORES_PER_COLUMN 10
#define BAR_WIDTH 20
#define PROCESS_DISPLAY 25

typedef struct {
    int pid;
    char name[256];
    float cpu;
    float mem;
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

void coreUsage(float *usage) {
    FILE *fp = fopen("/proc/stat", "r");
    char buf[1024];
    int core = 0;

    while (fgets(buf, sizeof(buf), fp)) {
        if (strncmp(buf, "cpu", 3) == 0 && isdigit(buf[3])) {
            long user, nice, system, idle, iowait, irq, softirq, steal;
            sscanf(buf, "cpu%*d %ld %ld %ld %ld %ld %ld %ld %ld", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);

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

    if (!fp) {
        refresh();
        return stats;
    }
    
    long total = 0;
    long free = 0;
    long buffers = 0;
    long cached = 0;
    /* 
    Will used in future implemtnation long swap = 0;
     */
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

    int attempted = 0;
    int nullChecker = 0;

    while ((entry = readdir(dir)) != NULL && count < max) {
        if (!isdigit(entry->d_name[0])) {
            continue;
        }

        attempted++;
        int pid = atoi(entry->d_name);
        
        char cpu_path[PREVENT_OVERFLOW];
        char cmdline_path[PREVENT_OVERFLOW];
        char mem_path[PREVENT_OVERFLOW];
        
        snprintf(cpu_path, sizeof(cpu_path), "/proc/%d/stat", pid);
        snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);
        snprintf(mem_path, sizeof(mem_path), "/proc/%d/statm", pid);
        
        FILE *cpu_fp = fopen(cpu_path, "r");
        FILE *mem_fp = fopen(mem_path, "r");

        if (!cpu_fp) {
            nullChecker++;
            continue;
        }

        if (!mem_fp) {
            nullChecker++;
            continue;
        }

        long total_mem = 0;
        long proc_memusage = 0;
        long utime = 0;
        long stime = 0;

        char stat_line[1024];

        if (!fgets(stat_line, sizeof(stat_line), cpu_fp)) {
            fclose(cpu_fp);
            nullChecker++;
            continue;
        }
        fclose(cpu_fp);
       
        char *name_start = strchr(stat_line, '(');
        char *name_end = strrchr(stat_line, ')');
        if (!name_start || !name_end || name_start >= name_end) {
            nullChecker++;
            continue;
        }
        
        char name[256];
        int name_len = name_end - name_start - 1;

        if (name_len < 0) {
            name_len = 0;
        } else if (name_len >= 256) {
            name_len = 255;
        }

        strncpy(name, name_start + 1, name_len);
        name[name_len] = '\0';
        
        FILE *cmd_fp = fopen(cmdline_path, "r");

        if (cmd_fp) {
            size_t bufsize = INITIAL_SIZE;
            char *cmdline = malloc(bufsize);
            size_t len = 0;
            int c;

            while ((c = fgetc(cmd_fp)) != EOF && c != '\n') {
                if (len + 1 >= bufsize) {
                    bufsize *= 2;
                    char *temp = realloc(cmdline, bufsize);
                    if (!temp) {
                        free(cmdline);
                        perror("realloc failed");
                        fclose(cmd_fp);
                    }
                    cmdline = temp;
                }
                cmdline[len++] = c;
            }

            cmdline[len] = '\0';
            if (len > 0) {
                char name[NAME_MAX_LEN];
                char *base = strrchr(cmdline, '/');
                if (base) {
                    strncpy(name, base + 1, NAME_MAX_LEN - 1);
                } else {
                    strncpy(name, cmdline, NAME_MAX_LEN - 1);
                }
                name[NAME_MAX_LEN - 1] = '\0'; 

                for (char *p = name; *p; p++) {
                    if (*p < ' ') *p = ' ';
                }
            }

            free(cmdline);
            fclose(cmd_fp);            
        }
        
        char *stat_ptr = name_end + 2; 
        for (int i = 0; i < 12; i++) {
            while (*stat_ptr && *stat_ptr != ' ') {
                stat_ptr++;
            }
            while (*stat_ptr && *stat_ptr == ' ') {
                stat_ptr++;
            }

            if (!stat_ptr || *stat_ptr == '\0') {
                break;
            }
        }
        
        if (sscanf(stat_ptr, "%ld %ld", &utime, &stime) != 2) {
            nullChecker++;
            continue;
        }
      
        if (mem_fp) {

            if (fscanf(mem_fp, "%ld %ld", &total_mem, &proc_memusage) != 2) {
                proc_memusage = 0;  
            }
            fclose(mem_fp);
        }
        
        procs[count].pid = pid;
        strncpy(procs[count].name, name, 255);
        procs[count].name[255] = '\0';
        
        long page_size = sysconf(_SC_PAGESIZE);
        float mem_usage = (proc_memusage * page_size) / (1024.0 * 1024.0);
        
        procs[count].cpu = (float)(utime + stime) / sysconf(_SC_CLK_TCK);
        procs[count].mem = mem_usage;
        
        count++;
    }
    
    closedir(dir); 
    return count;
}

int compareCpu(const void *a, const void *b) {
    const Process *p1 = (const Process *)a;
    const Process *p2 = (const Process *)b;
    
    if (p2 -> cpu > p1 -> cpu) {
        return 1;
    } else if (p2 -> cpu < p1 -> cpu) {
        return -1;
    }

    return 0;
}

int compareMem(const void *a, const void *b) {
    const Process *p1 = (const Process *)a;
    const Process *p2 = (const Process *)b;

    if (p2 -> mem > p1 -> mem) {
        return 1;
    } else if (p2 -> mem < p1 -> mem) {
        return -1;
    }

    return 0;
}

void bargraph(int y, int x, const char *label, float percent) {
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
    mvprintw(proc_row, 0, " PID    CPU    MEM    COMMAND"); 

    int display_count = (count < PROCESS_DISPLAY) ? count : PROCESS_DISPLAY;

    for (int i = 0; i < display_count && (i + scroll) < count; i++) {
        int id = i + scroll;
        mvprintw(proc_row + 1 + i, 0, "%5d  %5.1f  %7.1f  %s", procs[id].pid, procs[id].cpu, procs[id].mem, procs[id].name);
    }
}

int main() {    
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    cbreak();
    timeout(1000);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    curs_set(FALSE);

    amountCores();
    Process procs[10000];   
    float core_usages[MAX_CORES]; 

    struct ClockInfo clocks[MAX_CORES];
    int freqCount = 0;

    int scroll = 0;
    int count = 0;

    while (1) {
        clear();
        mvprintw(0, 0, "Welcome to Ftop!");
        coreUsage(core_usages);
        MemoryStats mem_stats = memStats();      
        freqCount = catFrequency(clocks, coreAmount);
//        long uptime = show_uptime();

        float disk = diskUsage("/");
        long total_disk = diskTotal("/");

      
        int base_row = 2; 
         
        for (int i = 0; i < coreAmount; i++) {
            int col = i / CORES_PER_COLUMN;
            int row = i % CORES_PER_COLUMN;
            int x = col * 70; 
            int y = base_row + row;        
            float freq_mhz = 0.0f;
    
            for (int j = 0; j < freqCount; j++) {
                if (clocks[j].core_id == i) {
                    freq_mhz = clocks[j].MHz;
                    break;
                }
            }
            
            char label[32];
            if (freq_mhz >= 1000.0f) {
                snprintf(label, sizeof(label), "CPU %02d %.2fGHz", i, freq_mhz/1000.0f);
            } else if (freq_mhz > 0.0f) {
                snprintf(label, sizeof(label), "CPU %02d %.0fMHz", i, freq_mhz);
            } else {
                snprintf(label, sizeof(label), "CPU %02d", i);
            }
    
            bargraph(y, x, label, core_usages[i]);
        }

        int mem_row = base_row + CORES_PER_COLUMN + 1;
        bargraph(mem_row, 0, "Memory", mem_stats.percentage);
        mvprintw(mem_row, 50 , "%.2f GB / %.2f GB", mem_stats.usedGB, mem_stats.totalGB);

        int disk_row = mem_row + 2;
        bargraph(disk_row, 0, "Disk Usage", disk);

        int diskTotal_row = disk_row + 2; 
        mvprintw(diskTotal_row, 0, "Root %ld GB", total_disk);

        int cache_row = diskTotal_row + 2;


        cacheusage(cache_row, 0);

        count = processID(procs, 10000);
        
        int proc_row = cache_row + 5;
        qsort(procs, count, sizeof(Process), compareMem);
        processDisplay(procs, count, scroll, proc_row);
        
        /* 
        int isa_row = proc_row + PROCESS_DISPLAY + 2;
        displayISAInfo(isa_row, 100); 
        */
        
        refresh();
        
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
                break;
        }
    }

    endwin();
    return 0;
}
