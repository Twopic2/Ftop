#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>

float diskUsage(char *path) {
    struct statvfs stat;

    if (statvfs(path, &stat) != 0) {
        perror("statvfs failed");
    }

    unsigned long total = stat.f_blocks * stat.f_frsize;
    unsigned long free = stat.f_bfree * stat.f_frsize;
    unsigned long used = total - free;

    double used_percent = (double)used / total * 100.0;

    printf("Disk usage for %s:\n", path);
    printf("  Total: %.2f GB\n", total / 1e9);
    printf("  Used:  %.2f GB\n", used / 1e9);
    printf("  Free:  %.2f GB\n", free / 1e9);
    printf("  Usage: %.2f%%\n", used_percent);

    return (100.0 * used) / total;
}

