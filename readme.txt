该项目使用线程池和epoll(ET)实现的并发linuxweb服务器，语言采用c++，使用reactor模式以及B/S模型。
使用有限状态自动机解析http报文，支持get请求，可以访问服务器的文件，实现文件的下载。

各部分组件
http_client 客户端
http_server 服务器
http_conn 处理http以及读写的逻辑
threadpool 线程池
lock 封装的锁
文件夹 dirfile 可以传输的文件所在目录

