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
#include <cstdarg>
#include <fcntl.h>


using namespace std;

#define LIBC "libc.so.6"
extern "C"
{
    void checkbuf(const void* buf, char* retchar)
    {
       const char* charptr = static_cast<const char*>(buf);

       int i = 0;
       while(1)
       {
            if(charptr[i]=='\0' || i>31)
                break;
            
            if(isprint(charptr[i]))
                retchar[i] = charptr[i];
            else
                retchar[i] = '.';

            i++;
       }
    }

    int chmod(const char *pathname, mode_t mode)
    {
        int (*old_chmod)(const char *, mode_t);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_chmod) = dlsym(handle, "chmod");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        int retval = (*old_chmod)(pathname, mode);
        char resolved_path[1024];

        char* retptr = realpath(pathname, resolved_path);
        if(retptr == NULL)
            strcpy(resolved_path, pathname);

        char* fd = getenv("OUTPUT_FD");
        dprintf(atoi(fd), "[logger] chmod(\"%s\", %o) = %d\n", resolved_path, mode, retval);

        dlclose(handle);
        return retval;
    }

    int creat(const char *pathname, mode_t mode)
    {
        int (*old_creat)(const char *, mode_t);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;
        char resolved_path[200];

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_creat) = dlsym(handle, "creat");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        int retval = (*old_creat)(pathname, mode);

        char* retptr = realpath(pathname, resolved_path);
        if(retptr == NULL)
            strcpy(resolved_path, pathname);

        char* fd = getenv("OUTPUT_FD");
        dprintf(atoi(fd),"[logger] creat(\"%s\", %o) = %d\n", resolved_path, mode, retval);

        dlclose(handle);
        return retval;
    }

    int creat64(const char *pathname, mode_t mode)
    {
        int (*old_creat)(const char *, mode_t);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;
        char resolved_path[200];

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_creat) = dlsym(handle, "creat64");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        int retval = (*old_creat)(pathname, mode);

        char* retptr = realpath(pathname, resolved_path);
        if(retptr == NULL)
            strcpy(resolved_path, pathname);

        char* fd = getenv("OUTPUT_FD");
        dprintf(atoi(fd),"[logger] creat64(\"%s\", %o) = %d\n", resolved_path, mode, retval);

        dlclose(handle);
        return retval;
    }

    int chown(const char *pathname, uid_t owner, gid_t group)
    {
        int (*old_chown)(const char *, uid_t, gid_t);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;
        char resolved_path[200];

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_chown) = dlsym(handle, "chown");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        int retval = (*old_chown)(pathname, owner, group);

        char* retptr = realpath(pathname, resolved_path);
        if(retptr == NULL)
            strcpy(resolved_path, pathname);

        char* fd = getenv("OUTPUT_FD");
        dprintf(atoi(fd),"[logger] chown(\"%s\", %o, %o) = %d\n", resolved_path, owner, group, retval);

        dlclose(handle);
        return retval;
    }

    int remove(const char *pathname)
    {
        int (*old_remove)(const char *);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;
        char resolved_path[200];

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_remove) = dlsym(handle, "remove");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        char* retptr = realpath(pathname, resolved_path);
        if(retptr == NULL)
            strcpy(resolved_path, pathname);
        
        int retval = (*old_remove)(pathname);
        char* fd = getenv("OUTPUT_FD");
        dprintf(atoi(fd),"[logger] remove(\"%s\") = %d\n", resolved_path, retval);
        
        dlclose(handle);
        return retval;
    }

    int open(const char *path, int flags,...)
    {
        mode_t mode = 0;
        if (__OPEN_NEEDS_MODE(flags))
        {
            va_list arg;
            va_start(arg, flags);
            mode = va_arg(arg, mode_t);
            va_end (arg);
        }

        int (*old_open)(const char *, int, mode_t);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;
        struct stat sb;
        char resolved_path[200];

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_open) = dlsym(handle, "open");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        char* retptr = realpath(path, resolved_path);
        if(retptr == NULL)
            strcpy(resolved_path, path);

        int retval = (*old_open)(path, flags, mode);

        char* fd = getenv("OUTPUT_FD");
        dprintf(atoi(fd),"[logger] open(\"%s,\" %o, %o) = %d\n", resolved_path, flags, mode, retval);
        
        dlclose(handle);

        return retval;
    }

    int open64(const char *path, int flags,...)
    {
        mode_t mode = 0;
        if (__OPEN_NEEDS_MODE(flags))
        {
            va_list arg;
            va_start(arg, flags);
            mode = va_arg(arg, mode_t);
            va_end (arg);
        }

        int (*old_open)(const char *, int, mode_t);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;
        struct stat sb;
        char resolved_path[200];

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_open) = dlsym(handle, "open64");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        char* retptr = realpath(path, resolved_path);
        if(retptr == NULL)
            strcpy(resolved_path, path);

        int retval = (*old_open)(path, flags, mode);

        char* fd = getenv("OUTPUT_FD");
        dprintf(atoi(fd),"[logger] open64(\"%s,\" %o, %o) = %d\n", resolved_path, flags, mode, retval);
        
        dlclose(handle);

        return retval;
    }
    
    int rename(const char *oldpath, const char *newpath)
    {
        int (*old_rename)(const char *, const char *);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;
        char resolved_oldpath[200];
        char resolved_newpath[200];

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_rename) = dlsym(handle, "rename");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        char* retptr1 = realpath(oldpath, resolved_oldpath);
        if(retptr1 == NULL)
            strcpy(resolved_oldpath, oldpath);

        int retval = (*old_rename)(oldpath, newpath);
            
        char* retptr2 = realpath(newpath, resolved_newpath);
        if(retptr2 == NULL)
            strcpy(resolved_newpath, newpath);
        char* fd = getenv("OUTPUT_FD");
        dprintf(atoi(fd), "[logger] rename(\"%s\",  \"%s\") = %d\n", resolved_oldpath, resolved_newpath, retval);

        dlclose(handle);
        return retval;
    }

    ssize_t write(int fd, const void *buf, size_t count)
    {
        ssize_t (*old_write)(int, const void*, size_t);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;
        char resolved_path[200];

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_write) = dlsym(handle, "write");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        char str_buf[4096]={'\0'};
        string fd_path = "/proc/self/fd/" + to_string(fd);
        readlink(fd_path.c_str(), str_buf, sizeof(str_buf)-1);
        
        int retval = (*old_write)(fd, buf, count);
        char checked_buf[32]={'\0'};
        checkbuf(buf, checked_buf);

        char* out_fd = getenv("OUTPUT_FD");
        dprintf(atoi(out_fd), "[logger] write(\"%s \", \"%s\", %d) = %d\n", str_buf, checked_buf, count, retval);

        dlclose(handle);
        
        return retval;
    }   

    ssize_t read(int fd, void *buf, size_t count)
    {
        int (*old_read)(int, void*, size_t);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_read) = dlsym(handle, "read");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        int retval = (*old_read)(fd, buf, count);

        char str_buf[4096]={'\0'};
        string fd_path = "/proc/self/fd/" + to_string(fd);
        readlink(fd_path.c_str(), str_buf, sizeof(str_buf)-1);
        
        char checked_buf[32]={'\0'};
        checkbuf(buf, checked_buf);
        
        char* out_fd = getenv("OUTPUT_FD");
        dprintf(atoi(out_fd), "[logger] read(\"%s\", \"%s\", %d) = %d\n", str_buf, checked_buf, count, retval);

        dlclose(handle);

        return retval;
    }
    
    int close(int fd)
    {
        int (*old_close)(int);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_close) = dlsym(handle, "close");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);
        
        char str_buf[4096]={'\0'};
        string fd_path = "/proc/self/fd/" + to_string(fd);
        readlink(fd_path.c_str(), str_buf, sizeof(str_buf)-1);
        
        int retval = (*old_close)(fd);
        char* out_fd = getenv("OUTPUT_FD");
        dprintf(atoi(out_fd), "[logger] close(\"%s\") = %d\n", str_buf, retval);
        
        dlclose(handle);

        return retval;
    }

    size_t fread(void * ptr, size_t size, size_t nmemb, FILE * stream)
    {
        size_t (*old_fread)(void *, size_t, size_t, FILE *);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_fread) = dlsym(handle, "fread");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        char str_buf[4096]={'\0'};
        int fno;
        fno = fileno(stream);
        string fd_path = "/proc/self/fd/" + to_string(fno);
        readlink(fd_path.c_str(), str_buf, sizeof(str_buf)-1);
        
        size_t retval = (*old_fread)(ptr, size, nmemb, stream);
        char checked_buf[32] = {'\0'};
        checkbuf(ptr, checked_buf);

        char* out_fd = getenv("OUTPUT_FD");
        dprintf(atoi(out_fd), "[logger] fread(\"%s\", %d, %d, \"%s\") = %d\n",  checked_buf, size, nmemb, str_buf, retval);

        dlclose(handle);

        return retval;
    }

    FILE* fopen(const char * pathname, const char * mode)
    {
        FILE* (*old_fopen)(const char *,  const char *);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;
        char resolved_path[200];

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_fopen) = dlsym(handle, "fopen");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        char* retptr = realpath(pathname, resolved_path);
        if(retptr == NULL)
            strcpy(resolved_path, pathname);
        
        FILE* retval = (*old_fopen)(pathname, mode);
        
        char* out_fd = getenv("OUTPUT_FD");
        dprintf(atoi(out_fd), "[logger] fopen(\"%s\", \"%s\") = %p\n",  resolved_path, mode, retval);

        dlclose(handle);

        return retval;
    }

    FILE* fopen64(const char * pathname, const char * mode)
    {
        FILE* (*old_fopen)(const char *,  const char *);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;
        char resolved_path[200];

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_fopen) = dlsym(handle, "fopen64");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        char* retptr = realpath(pathname, resolved_path);
        if(retptr == NULL)
            strcpy(resolved_path, pathname);
        
        FILE* retval = (*old_fopen)(pathname, mode);
        
        char* out_fd = getenv("OUTPUT_FD");
        dprintf(atoi(out_fd), "[logger] fopen64(\"%s\", \"%s\") = %p\n",  resolved_path, mode, retval);

        dlclose(handle);

        return retval;
    }

    size_t fwrite(const void * ptr, size_t size, size_t nitems, FILE * stream)
    {
        int (*old_fread)(const void * ptr, size_t size, size_t nitems, FILE * stream);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_fread) = dlsym(handle, "fwrite");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        char str_buf[4096]={'\0'};
        int fno;
        fno = fileno(stream);
        string fd_path = "/proc/self/fd/" + to_string(fno);
        readlink(fd_path.c_str(), str_buf, sizeof(str_buf)-1);
        
        int retval = (*old_fread)(ptr, size, nitems, stream);
        char checked_buf[32] = {'\0'};
        checkbuf(ptr, checked_buf);

        char* out_fd = getenv("OUTPUT_FD");
        dprintf(atoi(out_fd),"[logger] fwrite(\"%s\", %d, %d, \"%s\") = %d\n",  checked_buf, size, nitems, str_buf, retval);

        dlclose(handle);

        return retval;
    }

    int fclose(FILE *stream)
    {
        int (*old_fclose)(FILE *);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_fclose) = dlsym(handle, "fclose");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);

        char str_buf[1024]={'\0'};
        int fno;
        fno = fileno(stream);
        string fd_path = "/proc/self/fd/" + to_string(fno);
        readlink(fd_path.c_str(), str_buf, sizeof(str_buf)-1);
        
        int retval = (*old_fclose)(stream);

        char* out_fd = getenv("OUTPUT_FD");
        dprintf(atoi(out_fd), "[logger] fclose(\"%s\") = %d\n",  str_buf, retval);

        dlclose(handle);

        return retval;
    }

    FILE *tmpfile(void)
    {
        FILE* (*old_tmpfile)(void);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_tmpfile) = dlsym(handle, "tmpfile");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);
        
        FILE* retval = (*old_tmpfile)();

        char* out_fd = getenv("OUTPUT_FD");
        dprintf(atoi(out_fd),"[logger] tmpfile() = %p\n",  retval);

        dlclose(handle);

        return retval;
    }

    FILE *tmpfile64(void)
    {
        FILE* (*old_tmpfile)(void);
        void *handle = dlopen(LIBC, RTLD_LAZY);
        char *error;

        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        else
            *(void **)(&old_tmpfile) = dlsym(handle, "tmpfile64");

        if ((error = dlerror()) != NULL)
            fprintf(stderr, "%s\n", error);
        
        FILE* retval = (*old_tmpfile)();

        char* out_fd = getenv("OUTPUT_FD");
        dprintf(atoi(out_fd),"[logger] tmpfile64() = %p\n",  retval);

        dlclose(handle);

        return retval;
    }
}