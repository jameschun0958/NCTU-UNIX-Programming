#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h> 
#include<fstream>
#include<sstream>
#include<iostream>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include<vector>
#include <algorithm>
#include <regex>

using namespace std;

struct file_descriptor
{
    bool opendir;
    bool readdir;
    string fd;
    string type;
    string node;
    string name;
};

struct pid_info
{
    string pid;
    string uid;
    string username;
    string comm;
    vector<file_descriptor> file_desc;
    string del;
};

int DigitFilter(const struct dirent *pDir)
{
  if (isdigit(pDir->d_name[0]))
  {
    return 1;
  }
  return 0;
}

bool is_number(string str)
{
    string::const_iterator it = str.begin();
    while (it != str.end() && std::isdigit(*it)) ++it;
    return !str.empty() && it == str.end();
}

string exec_cmd(const char* cmd)
{
    char buffer[128];
    string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }

    return result;
}

void print_prefix()
{
    cout<< "COMMAND\tPID\tUSER\tFD\tTYPE\tNODE\tNAME\n";
}

void print_all(vector<struct pid_info> &lsof)
{
    //int pid_count = sizeof(lsof)/sizeof(lsof[0]);

    for (int i = 0; i < lsof.size(); i++)
    {
        for(int j=0; j<lsof[i].file_desc.size(); j++)
        {
            cout<<lsof[i].comm<<"\t"<<lsof[i].pid<<"\t"<<lsof[i].username<<"\t";
            cout<<lsof[i].file_desc[j].fd<<"\t"<<lsof[i].file_desc[j].type<<"\t"<<lsof[i].file_desc[j].node<<"\t"<<lsof[i].file_desc[j].name<<endl;
        }
        
    }
}

void search_comm(vector<struct pid_info> &lsof, string comm)
{
    if(comm != "")
    {
        regex str(comm);
        for (int i = 0; i < lsof.size();)
        {
            if(!regex_search(lsof[i].comm, str))
            {
                lsof.erase(lsof.begin() + i);
            }
            else
            {
                i++;
            }
        }
    }
}


void search_type(vector<struct pid_info> &lsof, string type)
{
    if(type != "")
    {
        regex str(type);
        for (int i = 0; i < lsof.size(); i++)
        {
            for(int j=0; j < lsof[i].file_desc.size();)
            {
                if(!regex_search(lsof[i].file_desc[j].type, str))
                {
                    lsof[i].file_desc.erase(lsof[i].file_desc.begin() + j);
                }
                else
                {
                    j++;
                }
            }
        }
    }
}

void search_file(vector<struct pid_info> &lsof, string file)
{
    if(file != "")
    {
        regex str(file);
        for (int i = 0; i < lsof.size(); i++)
        {
            for(int j=0; j < lsof[i].file_desc.size();)
            {
                if(!regex_search(lsof[i].file_desc[j].name, str))
                {
                    lsof[i].file_desc.erase(lsof[i].file_desc.begin() + j);
                }
                else
                {
                    j++;
                }
            }
        }
    }
}

string get_comm(string pid)
{
    string comm_path = "/proc/" + pid + "/comm";
    string result;
    ifstream pFile;
    pFile.open(comm_path);
    if (pFile.is_open())
    {
        getline(pFile, result);
        //cout<<"comm: "<<result<<endl;
    }
    else{
        cout<<comm_path<<" get_comm error!"<<endl;
    }
    pFile.close();

    return result;
}

string get_uid(string pid)
{
    string result;
    ifstream pFile;
    string status_path = "/proc/" + pid + "/status";
    pFile.open(status_path);
    if (pFile.is_open())
    {
        string str;
        while(getline(pFile, str))
        {
            istringstream ss(str);
            string word;

            while(ss >> word)
            {
                if(word == "Uid:")
                {
                    //cout<<str<<endl;
                    istringstream ss2(str);
                    string name,uid;
                    ss2>>name>>uid;
                    result = uid;
                    break;
                }
            }
        }
    }
    else{
        cout<<status_path<<" get_uid error!"<<endl;
    }
    pFile.close();

    return result;
}

string get_username(string uid)
{
    struct passwd *user;
    string result;
    user = getpwuid(atoi(uid.c_str()));
    result = user->pw_name;
    //cout<<"username: "<<result<<endl;
    return result;
}

string get_type(struct stat sb)
{
    string type;
    switch (sb.st_mode & S_IFMT) {
        case S_IFBLK:  type="Blok";
            break;
        case S_IFCHR:  type="CHR";        
            break;
        case S_IFDIR:  type="DIR";        
            break;
        case S_IFIFO:  type="FIFO";
            break;
        case S_IFLNK:  type="LINK";                 
            break;
        case S_IFREG:  type="REG";            
            break;
        case S_IFSOCK: type="SOCK";                  
            break;
        default:       type="unknow";                
            break;
    }

    return type;
}

file_descriptor get_cwd(string pid)
{
    char buf[PATH_MAX];
    file_descriptor file_desc;
    struct stat sb;
    string cwd_path = "/proc/" + pid + "/cwd";
    if(access(cwd_path.c_str(), R_OK) == 0)
    {
        file_desc.fd = "cwd";
        int name_size = readlink(cwd_path.c_str(), buf, sizeof(buf)-1);
        buf[name_size] = '\0';
        file_desc.name = buf;
        stat(cwd_path.c_str(), &sb);
        file_desc.node = to_string(sb.st_ino);
        file_desc.type = get_type(sb);
    }
    else
    {
        file_desc.fd = "cwd";
        file_desc.type = "unknown";
        file_desc.node = "\t";
        file_desc.name = cwd_path + " (readlink: Permission denied)";
    }

    return file_desc;
}

file_descriptor get_root(string pid)
{
    char buf[PATH_MAX];
    file_descriptor file_desc;
    struct stat sb;
    string root_path = "/proc/" + pid + "/root";
    if(access(root_path.c_str(), R_OK) == 0)
    {
        file_desc.fd = "root";
        int name_size = readlink(root_path.c_str(), buf, sizeof(buf)-1);
        buf[name_size] = '\0';
        file_desc.name = buf;

        stat(root_path.c_str(), &sb);
        file_desc.node = to_string(sb.st_ino);
        file_desc.type = get_type(sb);
    }
    else
    {
        file_desc.fd = "root";
        file_desc.type = "unknown";
        file_desc.name = root_path + " (readlink: Permission denied)";
        file_desc.node = "\t";
    }

    return file_desc;
}

file_descriptor get_exe(string pid)
{
    char buf[PATH_MAX];
    file_descriptor file_desc;
    struct stat sb;
    string exe_path = "/proc/" + pid + "/exe";
    if(access(exe_path.c_str(), R_OK) == 0)
    {
        file_desc.fd = "exe";
        int name_size = readlink(exe_path.c_str(), buf, sizeof(buf)-1);
        buf[name_size] = '\0';
        file_desc.name = buf;
        stat(exe_path.c_str(), &sb);
        file_desc.node = to_string(sb.st_ino);
        file_desc.type = get_type(sb);
    }
    else
    {
        file_desc.fd = "exe";
        file_desc.type = "unknown";
        file_desc.name = exe_path + " (readlink: Permission denied)";
        file_desc.node = "\t";
    }

    return file_desc;
}


vector<file_descriptor> get_mem(string pid)
{
    vector<file_descriptor> result;
    string mem_path = "/proc/" + pid + "/maps";
    ifstream pFile;

    if(access(mem_path.c_str(), R_OK) == 0)
    {
        pFile.open(mem_path);
        if (pFile.is_open())
        {
            string str;
            while(getline(pFile, str))
            {
                istringstream ss(str);
                string word;
                while(ss >> word)
                {
                    if(word.find("/") != string::npos)
                    {
                        // if element in vector or not

                        if(result.size()==0)
                        {
                            file_descriptor file_desc;
                            struct stat sb;
                            file_desc.fd = "mem";
                            file_desc.name = word;
                            stat(mem_path.c_str(), &sb);
                            file_desc.node = to_string(sb.st_ino);
                            file_desc.type = get_type(sb);
                            result.push_back(file_desc);
                        }
                            
                        for(int i=0;i<result.size();i++)
                        {
                            if(result[i].name == word)
                                break;
                            else if(result[i].name != word && i == result.size()-1)
                            {
                                file_descriptor file_desc;
                                struct stat sb;
                                file_desc.fd = "mem";
                                //file_desc.type = "REG";
                                file_desc.name = word;
                                stat(mem_path.c_str(), &sb);
                                file_desc.node = to_string(sb.st_ino);
                                file_desc.type = get_type(sb);
                                //cout<<word<<endl;
                                result.push_back(file_desc);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            cout<<mem_path<<" get_mem error"<<endl;
        }
        pFile.close();
    }

    return result;
}

string get_fd_type(string pos, string fd_dir_path)
{
    ifstream pFile;
    string result = pos;
    pFile.open(fd_dir_path);
    if (pFile.is_open())
    {
        string str;
        while(getline(pFile, str))
        {
            istringstream ss(str);
            string word;
            bool check=false;
            while(ss >> word)
            {
                if(check)
                {
                    char last_char = word.back();
                    //cout<<"last char"<<last_char<<endl;
                    if(last_char == '0')
                        result = result + "r";
                    else if(last_char == '1')
                        result = result + "w";
                    else if(last_char == '2')
                        result = result + "u";
                    else
                        result = result + "N";

                }
                
                if(word.find("flags:")!= string::npos)
                    check = true;
            }           
        }
    }

    return result;
}

vector<file_descriptor> get_fd(string pid)
{
    string fd_path = "/proc/" + pid + "/fd";
    string fdinfo_path = "/proc/" + pid + "/fdinfo";
    struct stat sb;
    vector<file_descriptor> result;
    
    if(access(fd_path.c_str(), R_OK) == 0)
    {   
        DIR *dir;
        struct dirent *dir_ptr;
        
        dir = opendir(fd_path.c_str());
        while((dir_ptr = readdir(dir)) != NULL)
        {
            //cout<<"dir_ptr: "<<dir_ptr->d_name<<endl;
            string fd_pos = dir_ptr->d_name;
            if(is_number(fd_pos))
            {
                file_descriptor file_desc;
                char buf[4096];
                string fdinfo_path = "/proc/" + pid + "/fdinfo/" + fd_pos;
                string fd_dir_path = fd_path + "/" + fd_pos;

                stat(fd_dir_path.c_str(), &sb);
                int name_size = readlink(fd_dir_path.c_str(), buf, sizeof(buf)-1);
                buf[name_size] = '\0';
                file_desc.name = buf;
                file_desc.type = get_type(sb);
                file_desc.node = to_string(sb.st_ino);
                file_desc.fd = get_fd_type(fd_pos, fdinfo_path);
                result.push_back(file_desc);
            }
        }
        
        //cout<<"fd_path: "<<fd_path<<endl;
    }
    else
    {
        file_descriptor file_desc;
        file_desc.fd = "NOFD";
        file_desc.type = get_type(sb);
        file_desc.node = "\t";
        file_desc.name = fd_path + "(opendir: Permission denied)";
        result.push_back(file_desc);
    }

    return result;
}

int main(int argc, char *argv[]) {
    
    struct dirent **pid_list;

    int pid_count = scandir("/proc", &pid_list, DigitFilter, alphasort);
    vector<struct pid_info> lsof(pid_count);


    print_prefix();

    for (int i = 0; i < pid_count; i++) 
    {
        // get pid
        lsof[i].pid = pid_list[i]->d_name;
        //cout<<"pid: "<<lsof[i].pid<<endl;

        // get comm
        lsof[i].comm = get_comm(lsof[i].pid);

        // get uid
        lsof[i].uid = get_uid(lsof[i].pid);

        // get username
        lsof[i].username = get_username(lsof[i].uid);

        // get FD
        lsof[i].file_desc.push_back(get_cwd(lsof[i].pid));
        lsof[i].file_desc.push_back(get_root(lsof[i].pid));
        lsof[i].file_desc.push_back(get_exe(lsof[i].pid));
        vector<file_descriptor> mem = get_mem(lsof[i].pid);
        if(mem.size()!=0)
            lsof[i].file_desc.insert(end(lsof[i].file_desc), begin(mem), end(mem));

        vector<file_descriptor> fd = get_fd(lsof[i].pid);
        if(fd.size()!=0)
            lsof[i].file_desc.insert(end(lsof[i].file_desc), begin(fd), end(fd));
        
    }

    int cmd_opt = 0;
    string arg_comm, arg_type, arg_file;
    while((cmd_opt = getopt(argc, argv, "c:t:f:")) != -1)
    {
        switch(cmd_opt){
            case 'c':
                arg_comm = optarg;
                //cout<<"arg_comm: "<<arg_comm<<endl;
                break;
            case 't':
                arg_type = optarg;
                //cout<<"arg_type: "<<arg_type<<endl;
                break;
            case 'f':
                arg_file = optarg;
                //cout<<"arg_file: "<<arg_file<<endl;
                break;
            default:
                cout<<"input error!\n";
                break;
        }
    }

    // regex comm(arg_comm);
    // print_by_comm(lsof, comm);
    if(cmd_opt == 0)
        print_all(lsof);
    else
    {
        search_comm(lsof, arg_comm);
        search_type(lsof, arg_type);
        search_file(lsof, arg_file);
        print_all(lsof);
    }

    return 0;
}
