
//2018/7/2
#include<netinet/in.h>   // sockaddr_in
#include<sys/types.h>    // socket
#include<sys/socket.h>   // socket
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/select.h>   // select
#include<sys/ioctl.h>
#include<sys/time.h>
#include<iostream>
#include<vector>
#include<map>
#include<string>
#include<cstdlib>
#include<cstdio>
#include<cstring>
using namespace std;
#define BUFFER_SIZE 1024

//用于包头 表示包头类型 HEART：表示是心跳包，OTHER：表示数据包
enum Type {HEART, OTHER, END};
//用于包头 表示成员类型 PI：树莓派 CLIENT：客户端
enum Member {PI,CLIENT};
//用于包头 表示数据类型 KEY：关键字 LIST：列表 FILENAME：文件名 FILES：文件内容
enum DataType {KEY,LIST,FILENAME,FILES};

enum File_Type {CAJ,PDF};

int currrnt_client_id; //当前与树莓派通信的设备号，用于公网服务器与改客户端通信时用

/*
包头
type:包头类型
menber:成员类型
length:数据长度
id:用于存储目标客户端在公网服务中的id
Dtype:数据类型
*/
struct PACKET_HEAD
{
    Type type;
    Member member;
    int length;
    int id;
    int file_size;
    DataType Dtype;
    File_Type Ftype;
};

void* heart_handler(void* arg);

class Server
{
private:
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
    int listen_fd;    // 监听的fd
    int max_fd;       // 最大的fd
    int pi_fd;
    fd_set master_set;   // 所有fd集合，包括监听fd和客户端fd
    fd_set working_set;  // 工作集合
    struct timeval timeout;
    map<int, pair<string, int> > mmap;   // 记录连接的客户端fd--><ip, count>
public:
    Server(int port);
    ~Server();
    void Bind();
    void Listen(int queue_len = 20);
    void Accept();
    void Run();
    void Recv(int nums);
    friend void* heart_handler(void* arg);
};

Server::Server(int port)
{
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(port);
    // create socket to listen
    listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    pi_fd = -1;
    if(listen_fd < 0)
    {
        cout << "Create Socket Failed!";
        exit(1);
    }
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

Server::~Server()
{
    for(int fd=0; fd<=max_fd; ++fd)
    {
        if(FD_ISSET(fd, &master_set))
        {
            close(fd);
        }
    }
}

void Server::Bind()
{
    if(-1 == (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))))
    {
        cout << "Server Bind Failed!";
        exit(1);
    }
    cout << "Bind Successfully.\n";
}

void Server::Listen(int queue_len)
{
    if(-1 == listen(listen_fd, queue_len))
    {
        cout << "Server Listen Failed!";
        exit(1);
    }
    cout << "Listen Successfully.\n";
}

void Server::Accept()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int new_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    send(new_fd,&new_fd,sizeof(new_fd),0);
    if(new_fd < 0)
    {
        cout << "Server Accept Failed!";
        exit(1);
    }

    string ip(inet_ntoa(client_addr.sin_addr));    // 获取客户端IP

    cout << ip << " new connection was accepted.\n";

    mmap.insert(make_pair(new_fd, make_pair(ip, 0)));

    // 将新建立的连接的fd加入master_set
    FD_SET(new_fd, &master_set);
    if(new_fd > max_fd)
    {
        max_fd = new_fd;
    }
}

void Server::Recv(int nums)
{
    char *buffer = new char[BUFFER_SIZE+1];
    int flag = 0;
    FILE *fp;
    for(int fd=0; fd<=max_fd; ++fd){
        if(FD_ISSET(fd, &working_set)){
            bool close_conn = false;  // 标记当前连接是否断开了

            PACKET_HEAD head;
            recv(fd, &head, sizeof(head), 0);   // 先接受包头

            if(head.type == HEART){
                //cout << "Received heart-beat\n";
            }else if(head.type == END){

              close(fd);
              FD_CLR(fd,&working_set);
            }
            else if(head.member == PI){
                //数据包，通过head.length确认数据包长度
                if(pi_fd == -1)
                   pi_fd = fd;
                else{
                      int rev_length = 0;
                      int rev_length_sum = 0;
                      int send_length = 0;
                      int current_head_length = head.length;
                      int current_need_recv = head.length;
                      char *FileBuffer = new char[current_need_recv];
                      //cout << "current_head_length" << current_head_length << endl;
                      while(rev_length_sum < current_head_length){
                            bzero(buffer, BUFFER_SIZE+1);
                            //cout << "current_need_recv" << current_need_recv << endl;
                            if((rev_length_sum < BUFFER_SIZE)&&(rev_length_sum >= 0)){

                              rev_length = recv(pi_fd,buffer,current_need_recv,0); //接收数据
                              //cout << "rev_length" << rev_length << endl;
                              memcpy(FileBuffer+rev_length_sum,buffer,rev_length);
                              rev_length_sum += rev_length;
                              current_need_recv = current_head_length - rev_length_sum;
                              //cout << "rev_length_sum" << rev_length_sum << endl;
                            }
                            else{
                                //cout << "error" <<endl;
                            }
                  }
                  head.length = rev_length_sum;
                  send(head.id, &head, sizeof(head), 0); //发送包头
                  send_length = send(head.id, FileBuffer, head.length, 0); //发送数据
                  //cout << "REV: " << rev_length_sum <<" SEND: "<< send_length << endl;

                  rev_length_sum = 0;
                  delete[] FileBuffer;
                  usleep(200);
                }
            }
            else if(head.member == CLIENT)
            {


                bzero(buffer, BUFFER_SIZE+1);
                recv(fd,buffer,head.length,0); //接收数据
                //cout << buffer << endl;
                head.id = fd;
                send(pi_fd, &head, sizeof(head), 0); //发送包头
                send(pi_fd, buffer, head.length, 0); //发送数据

            }
        }
    }
}

void Server::Run()
{
    max_fd = listen_fd;   // 初始化max_fd
    FD_ZERO(&master_set);
    FD_SET(listen_fd, &master_set);  // 添加监听fd

    while(1)
    {
        FD_ZERO(&working_set);
        memcpy(&working_set, &master_set, sizeof(master_set));

        timeout.tv_sec = 30;
        timeout.tv_usec = 0;

        int nums = select(max_fd+1, &working_set, NULL, NULL, &timeout);
        if(nums < 0)
        {
            cout << "select() error!";
            exit(1);
        }

        if(nums == 0)
        {
            //cout << "select() is timeout!";
            continue;
        }

        if(FD_ISSET(listen_fd, &working_set))
            Accept();   // 有新的客户端请求
        else
            Recv(nums); // 接收客户端的消息
    }
}

int main()
{
    Server server(8080);
    server.Bind();
    server.Listen();
    server.Run();
    return 0;
}
