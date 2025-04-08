#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#define MAX_PROCESSES 128

typedef struct {
    int pid;
    char name[256];
    float cpu;
} Process


float get_cpu_usage() {
    static long prev_idle = 0, prev_total = 0;
    FILE *fp = fopen("/proc/stat", "r");
    char buf[1024];
    fgets(buf, sizeof(buf), fp);
    fclose(fp);

    long user, nice, system, idle, iowait, irq, softirq, steal;
    sscanf(buf, "cpu  %ld %ld %ld %ld %ld %ld %ld %ld",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);

    long idle_time = idle + iowait;
    long total_time = user + nice + system + idle + iowait + irq + softirq + steal;

    long delta_idle = idle_time - prev_idle;
    long delta_total = total_time - prev_total;

    prev_idle = idle_time;
    prev_total = total_time;

    if (delta_total == 0) {
	    return 0;
    }
	
    return (100.0 * (delta_total - delta_idle)) / delta_total;
}


float memUsage() {

    FILE *fp = fopen("/proc/meminfo", "r");
    long total = 0, free = 0, buffers = 0, cached = 0;
    char label[64];
    long value;
   
    // fscanf reads from file pointers 

    while (fscanf(fp, "%s %ld", label, &value) != EOF) {
        if (strcmp(label, "MemTotal:") == 0) total = value;
        else if (strcmp(label, "MemFree:") == 0) free = value;
        else if (strcmp(label, "Buffers:") == 0) buffers = value;
        else if (strcmp(label, "Cached:") == 0) cached = value;
    }
    fclose(fp);
    long used = total - free - buffers - cached;
    return (100.0 * used) / total;	

}

// float storageUsage(){}
// save for future ideas
//
int processID(Process *procs, int max) {

  Dir *dir = opendir("/proc");
  struct dirent *entry;
  int count = 0;


  while ((entry = readdir(dir)) != NULL && count < max) {
	
	char location[256];

	if (!isdigit(entry->d_name[0])) {
		   continue;
	}


	int pid = atoi(entry -> d_name);
	
	snprintf(location, sizeof(location), "/proc/%d/stat", pid);
        FILE *fp = fopen(path, "r");
        if (!fp) {
		continue;
	}

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

void print_help() {}

void chart(int row, const char *label, float percent) {

    mvprintw(row, 0, "%s [", label);
    int width = COLS - strlen(label) - 4;
    int filled = (int)(width * percent / 100.0);
    for (int i = 0; i < width; i++) {
        if (i < filled) addch('=');
        else addch(' ');
    }
    printw("] %.1f%%", percent);


}

int main() {


}
