#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

float memUsage() {
    FILE *fp = fopen("/proc/diskstats", "r");
   
    while (fscanf(fp, "%s %ld", label, &value) != EOF) {
        
    
    }
    fclose(fp);
    return total;
}
