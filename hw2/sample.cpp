#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include<iostream>

#define FILE_A "aaa"
#define FILE_B "bbb"
#define FILE_NULL "/dev/null"

int main()
{
    struct stat st;
    char *argv[] = {FILE_A, NULL};
    char buf[128];
    close(2);
    creat(FILE_A, 0600);
    chmod(FILE_A, 0666);
    chown(FILE_A, 065534, 065534);
    rename(FILE_A, FILE_B);
    int fd = open(FILE_B, 1101, 0666);
    write(fd, "cccc", 5);
    close(fd);
    fd = open(FILE_B, 000);
    read(fd, buf, 100);
    close(fd);
    FILE* stream = tmpfile();
    fwrite("cccc", 1, 5, stream);
    fclose(stream);
    FILE* fp = fopen(FILE_B, "r");
    fread(buf, 1, 100, fp);
    fclose(fp);
    remove(FILE_B);

    std::cout<<"sample done.\n";

    return 0;
}