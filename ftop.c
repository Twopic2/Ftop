#include<stdio.h> 
#include<stdlib.h> 
#include<errno.h> 
#include<sys/utsname.h> 

// Templete Code

struct utsname buffer;


void test() {
    errno = 0;

    if (uname(&buffer) != 0) {
        exit(EXIT_FAILURE);
    }

    printf("Sys name = %s", buffer.sysname);

}


int main() { 
   
    // The system info
    // int r=system("cat /proc/1/status"); 

    int i = system("cat /proc/cpuinfo | grep MHz");

} 





