#include<fstream>
#include<string>
#include<assert.h>
namespace server_config {
    bool isdamon = 0;
    std::string ip = "127.0.0.1";
    int port = 3389;
    int listenfd = 0;
    std::string conf_path = "../conf/";
    std::string default_conf_name = "server.conf";
    
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
                ip = str.substr(3);
            }
            else if(str.substr(0, 4) == "port") {
                assert(str.size() >= 6);
                port = stol(str.substr(5));
            }
        }
       
    }
    
   void setdaemon() {
       auto pid = fork();
       if(pid < 0) {
           exit(1);
       }
       else if(pid > 0) {
           exit(0);
       }
       else { // 子进程 成为了守护进程
           setsid(); // 组id
           chdir("."); // 暂时不改工作目录
           int des = getdtablesize(); //关闭文件描述符
           for(int i=0; i<des; i++) {
                close(i);
            }
      }
   }
   void setsocket_parameter() {
       struct linger tmp = {1, 0}; //发送RST, 将避免TIME_WAIT状态，将缓冲区残留数据丢弃
       setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof tmp);
   }
   void bindsocket() {
        listenfd = socket(PF_INET, SOCK_STREAM, 0);
        assert(listenfd >= 0);
        setsocket_parameter(); // 有关socket的参数设置
        int ret = 0;
        sockaddr_in address;
        bzero(&address, sizeof address);
        address.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str() , &address.sin_addr);
        address.sin_port = htons(port);
        ret = bind(listenfd, (sockaddr*)& address, sizeof address);
        assert(ret >= 0);
        ret = listen(listenfd, 5);
        assert(ret >= 0);
   }
   void init_server(int argc, char* argv[]) {
       /*
        读取配置文件
        设置地址等信息
       */
      std::string conf_file = conf_path;
      if(argc < 2)
        conf_file += default_conf_name;
      else
        conf_file += argv[1];

      readconf(conf_file.c_str());
      bindsocket(); 
      printf("服务器已启动\nip: %s\nport: %d\n\n", ip.c_str(), port);
      if(isdamon)
        setdaemon(); 
              
   }  
}