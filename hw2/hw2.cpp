#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <vector>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>

using namespace std;

#define FILE_A "aaa"
#define FILE_B "/tmp/bbb"
#define FILE_NULL "/dev/null"

void usage()
{
    printf("usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]\n \\
            -p: set the path to logger.so, default = ./logger.so\n \\
            -o: print output to file, print to \"stderr\" if no file specified\n \\
            --: separate the arguments for logger and for the command\n");
}

int main(int argc, char *argv[])
{
    int cmd_opt = 0;
    string arg_file;
    string arg_sopath="./logger.so";

    while(1)
    {
        cmd_opt = getopt(argc, argv, "o:p:");
        
        if(cmd_opt == -1)
        {
            break;
        }

        switch(cmd_opt){
            case 'o':
                arg_file = optarg;
                //cout<<"arg_comm: "<<arg_comm<<endl;
                break;
            case 'p':
                arg_sopath = optarg;
                //cout<<"arg_type: "<<arg_type<<endl;
                break;
            case '?':
                usage();
                return -1;      
        }
    }

    if(argc <= optind)
    {
       fprintf(stderr, "no command given.\n");
       return -1;
    }

    int fd = 2;
    if(!arg_file.empty())
    {
        fd = open(arg_file.c_str(), O_WRONLY|O_TRUNC|O_CREAT, 0644);
        if(fd < 0) 
            perror("open");
        
        // if(dup2(fd, 2) < 0)
        //     perror("dup2");

        setenv("OUTPUT_FD", to_string(fd).c_str(), true);
    }
    else{
        dup2(2, 3);
        setenv("OUTPUT_FD", to_string(3).c_str(), true);
    }

    vector<char*> execarg;
    int iter = optind;
    while(argv[iter]!= NULL)
    {
        execarg.push_back(argv[iter]);
        iter++;
    }
    //execarg.push_back(strdup(redirect.c_str()));
    execarg.push_back(NULL);

    setenv("LD_PRELOAD", arg_sopath.c_str(), true);
    

    int ret = execvp(argv[optind], &execarg[0]);

    return ret;
}
