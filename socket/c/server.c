#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BACKLOG 10
#define BUFFER_SIZE 1024

/**
 * 清空缓冲区
 */
void cache_flush() {
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}

/**
 * 回声服务器
 */
void echo_server(const int port) {
  // 创建套接字
  int serv_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  // ip4地址结构体
  // 与sockaddr区别：sockaddr为通用结构，并且sockaddr.sa_data无相关函数转换为有效的char给其赋值。sockadd_in与sockaddr长度一致，可以无损强转
  // ip6地址结构体为 sockaddr_in6
  struct sockaddr_in serve_addr;
  memset(&serve_addr, 0, sizeof(serve_addr));
  serve_addr.sin_family = PF_INET;
  serve_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serve_addr.sin_port = htons(port);
  // 绑定地址族、IP与端口
  bind(serv_socket, (struct sockaddr*)&serve_addr, sizeof(serve_addr));

  // 监听
  // backlog 缓冲区request.queue长度 (可默认为[SOMAXCONN]由系统调配)
  listen(serv_socket, BACKLOG);

  // 接收客户端请求
  struct sockaddr_in client_addr;
  socklen_t client_addr_size = sizeof(client_addr);
  char buffer[BUFFER_SIZE] = {0};
  while (1) {
    int client_socket =
        accept(serv_socket, (struct sockaddr*)&client_addr, &client_addr_size);

    // 回声
    read(client_socket, buffer, sizeof(buffer) - 1);
    write(client_socket, buffer, sizeof(buffer));

    // 清理工作
    close(client_socket);            // 关闭客户端套接字
    memset(buffer, 0, BUFFER_SIZE);  // 重置数据缓冲区
  }

  // 关闭服务端套接字
  close(serv_socket);
}

/**
 * 服务器文件下载
 */
void file_download(const int port, const char* file) {
  FILE* fp = fopen(file, "rb");
  if (fp == NULL) {
    perror("打开文件失败");
    exit(0);
  }

  // 创建套接字
  int serv_socket = socket(PF_INET, SOCK_STREAM, 0);
  // 绑定
  // struct in_addr ip = {s_addr : inet_addr("127.0.0.1")};
  struct sockaddr_in serv_addr = {
      .sin_family = PF_INET,
      .sin_port = port,
      .sin_addr = {.s_addr = inet_addr("127.0.0.1")},
  };
  bind(serv_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
  // 监听
  listen(serv_socket, BACKLOG);
  // 接收请求
  struct sockaddr clnt_addr;
  socklen_t clnt_addr_size = sizeof(clnt_addr);
  int clnt_socket = accept(serv_socket, &clnt_addr, &clnt_addr_size);

  // 下载
  char buffer[BUFFER_SIZE] = {0};
  int nCount;
  while ((nCount = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
    write(clnt_socket, buffer, nCount);
  }
  shutdown(serv_socket, SHUT_WR);  // 断开输出流
  read(serv_socket, buffer, sizeof(buffer));

  // 清理工作
  fclose(fp);
  close(serv_socket);
  close(clnt_socket);
}

int main() {
  // 回声
  echo_server(9501);

  // 下载音频
  file_download(9502, "./test.amr");
  return 0;
}