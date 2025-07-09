#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <ncurses.h>

struct sysinfo info; 

struct ClockInfo {
    float MHz;
    char speed[32];
    int core_id;
};

struct cacheinfo {
    int level;
    int sizeKB;
    int id; 
    char type[32];   
};

struct isaInfo {
    char isaSet[32]; 
};

long show_uptime() {
    sysinfo(&info);
    return info.uptime;
}

int catCache(struct cacheinfo *cacheArray, int max_entries) {
    struct dirent *entry;

    const char *cache_files = "/sys/devices/system/cpu/cpu0/cache/";
    DIR *dir = opendir(cache_files);

    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        char level_path[512];
        char size_path[512];

        snprintf(level_path, sizeof(level_path), "%s%s/level", cache_files, entry->d_name);
        snprintf(size_path, sizeof(size_path), "%s%s/size", cache_files, entry->d_name);

        FILE *flevel = fopen(level_path, "r");
        FILE *fsize = fopen(size_path, "r");
        
        if (!flevel || !fsize) {
            if (flevel) {
                fclose(flevel);
            }
            if (fsize) {
                fclose(fsize);
            }
            continue;
        }
                
        int level = 0;
        int sizeKB = 0;
        char size_str[256] = {0};

        if (fscanf(flevel, "%d", &level) != 1) {
            fclose(flevel);
            fclose(fsize);
            continue;
        }

        if (fgets(size_str, sizeof(size_str), fsize) == NULL) {
            fclose(flevel);
            fclose(fsize);
            continue;
        }
        
        size_str[strcspn(size_str, "\n")] = 0;

        if (strchr(size_str, 'K')) {
            sscanf(size_str, "%dK", &sizeKB);
        } else if (strchr(size_str, 'M')) {
            int sizeMB = 0;
            sscanf(size_str, "%dM", &sizeMB);
            sizeKB = sizeMB * 1024;
        }

        cacheArray[count].level = level;
        cacheArray[count].sizeKB = sizeKB;
        cacheArray[count].id = count;
        count++;

        fclose(flevel);
        fclose(fsize);
    }

    closedir(dir);
    return count;
}

void cacheusage(int row, int col) {
    struct cacheinfo cache[10];
    int count = catCache(cache, 10);

    if (count == 0) {
        mvprintw(row, col, "Cache info isn't working");
        return;
    }

    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (cache[i].level > cache[j].level) {
                struct cacheinfo temp = cache[i];
                cache[i] = cache[j];
                cache[j] = temp;
            }
        }
    }

    mvprintw(row, col, "CPU Cache Information:");    
    for (int i = 0; i < count; i++) {
        mvprintw(row + i + 1, col, "L%d Cache: %d KB", cache[i].level, cache[i].sizeKB);
    }
}

int catISA(struct isaInfo *isaArray, int max_entries) {
    FILE *fp = fopen("/proc/cpuinfo", "r");

    char line[512];
    int count = 0;

    while(fgets(line, sizeof(line), fp)) {

        if (strncmp(line, "flags", 5) == 0) {
            char *flags = strchr(line, ':');
             if (flags && *(flags + 1)) {
                flags += 2; 
                flags[strcspn(flags, "\n")] = 0;

                char *token = strtok(flags, " ");
                while (token && count < max_entries) {
                    strncpy(isaArray[count].isaSet, token, sizeof(isaArray[count].isaSet) - 1);
                    isaArray[count].isaSet[sizeof(isaArray[count].isaSet) - 1] = '\0';
                    count++;
                    token = strtok(NULL, " ");
                }
            }
                break;
          }

       }

        fclose(fp);
        return count;        
 }

void displayISAInfo(int row, int col) {
    struct isaInfo instructionSet[256];
    int count = catISA(instructionSet, 256);
    
    if (count == 0) {
        mvprintw(row, col, "ISA info isn't working");
        return;
    }

    mvprintw(row, col, "ISA Extensions:");
    int colums = 4;

     for (int i = 0; i < count; i++) {
        mvprintw(row + 1 + i / colums, col + (i % colums) * 15, "%s", instructionSet[i].isaSet);
    }
}

int catFrequency(struct ClockInfo *clocks, int max) {
    int count = 0; 
    FILE *id = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");

    for (int i = 0; i < max; i++) {
        char freqPath[256];
        snprintf(freqPath, sizeof(freqPath), 
                    "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
        FILE *fp = fopen(freqPath, "r");
        if (fp != NULL) {
            long freqs = 0;
            if (fscanf(fp, "%ld", &freqs) == 1) {
                clocks = malloc(sizeof(struct ClockInfo));
                clocks[count].MHz = freqs / 1000.0; 
                clocks[count].core_id = i;
                snprintf(clocks[count].speed, sizeof(clocks[count].speed), 
                            "%.0f MHz", clocks[count].MHz);
                count++;
                free(clocks);
            }
            fclose(fp);
        }
    }
        
    if (!id) {
        FILE *fp2 = fopen("/proc/cpuinfo", "r");
        if (fp2 != NULL) {
            char line[512];
            
            while (fgets(line, sizeof(line), fp2) && count < max) {
                if (strncmp(line, "cpu MHz", 7) == 0) {
                    char *colon = strchr(line, ':');
                    if (colon) {
                        float cpu_mhz = 0.0f;
                        sscanf(colon + 1, "%f", &cpu_mhz);
                        
                        clocks[count].MHz = cpu_mhz;
                        clocks[count].core_id = count; 
                        snprintf(clocks[count].speed, sizeof(clocks[count].speed), 
                                "%.0f MHz", cpu_mhz);
                        count++;
                    }
                }
            }
            fclose(fp2);
        }
    }
    return count;
}

