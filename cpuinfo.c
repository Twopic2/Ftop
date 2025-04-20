#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <linux/unistd.h>       
#include <linux/kernel.h>

struct sysinfo info; 

typedef struct {


} ISA;


long show_uptime() {

    sysinfo(&info);

    return info.uptime;

}