#include<fstream>
#include<string>
#include<unistd.h>
#include<assert.h>
using std::string;
string _ip;
char*ip;
int port;
bool isdamon;
void readconf(const char* filename) {
    std::fstream f_sm(filename);
    std::string str;
    while(getline(f_sm, str)) {
        if(str.substr(0, 7) == "isdamon") {
             assert(str.size() >= 9);
            isdamon = str[8] - '0';
        }
        else if(str.substr(0, 2) == "ip") {
            assert(str.size() >= 10);
            _ip = str.substr(3);
        }
        else if(str.substr(0, 4) == "port") {
            assert(str.size() >= 6);
            port = stol(str.substr(5));
        }
    }
       
}
void loadFromFile(int argc, char*argv[]) {
    assert(argc > 1);
    readconf(argv[1]);
}

void parse_arg(int argc, char*argv[]){
    int opt;
    const char *str = "f:p:i:d";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {

        case 'f':
        {
            loadFromFile(argc, argv); 
            return;
        }

        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        case 'i':
        {
            _ip = string(optarg);
            break;
        }
        case 'd':
        {
            isdamon = atoi(optarg);
        }
        
        default:
            break;
        }
    }
}

extern void initconfig(int argc, char*argv[]) {
    parse_arg(argc, argv);
    ip = const_cast<char*>(_ip.c_str());

    if(isdamon) {
       auto pid = fork();
       if(pid < 0) {
           exit(1);
       }
       else if(pid > 0) {
           exit(0);
       }
       else { // 子进程 成为了守护进程
           setsid(); // 组id
           //chdir("."); // 暂时不改工作目录
           int des = getdtablesize(); //关闭文件描述符
           for(int i=0; i<des; i++) {
                close(i);
            }
      }
    }
}
