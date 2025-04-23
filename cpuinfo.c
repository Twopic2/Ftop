#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <ncurses.h>

struct sysinfo info; 

struct cacheinfo {

    int level;
    int sizeKB;
    int id; 
    char type[32];   

};

long show_uptime() {

    sysinfo(&info);

    return info.uptime;

}

int catCache(struct cacheinfo *cacheArray, int max_entries) {

    struct dirent *entry;

    // I didn't know this but linux organizes cache by index such index0->Intruction. 
    const char *cache_files = "/sys/devices/system/cpu/cpu0/cache/";

    DIR *dir = opendir(cache_files);

    int count = 0;

    while ((entry = readdir(dir)) != NULL) {

        char level_path[512];
        char size_path[512];

        FILE *flevel = fopen(level_path, "r");
        FILE *fsize = fopen(size_path, "r");
      

        snprintf(level_path, sizeof(level_path), "%s%s/level", cache_files, entry->d_name);
        snprintf(size_path, sizeof(size_path), "%s%s/size", cache_files, entry->d_name);
        
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
        char type_str[256] = {0};

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

