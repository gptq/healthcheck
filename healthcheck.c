#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>  // 用于字符检查

#define BUFFER_SIZE 1024
#define MAX_PATH_LENGTH 100

// 检查端口是否有效
int is_valid_port(const char *port_str) {
    // 检查是否全是数字
    for (int i = 0; port_str[i] != '\0'; i++) {
        if (!isdigit(port_str[i])) {
            return 0;
        }
    }
    
    // 转换为整数并检查范围
    int port = atoi(port_str);
    return (port > 0 && port <= 65535);
}

// 检查路径是否有效（简单路径，只允许字母、数字、下划线、连字符和/）
int is_valid_path(const char *path) {
    // 检查路径长度
    if (strlen(path) > MAX_PATH_LENGTH) {
        return 0;
    }
    
    // 检查字符有效性
    for (int i = 0; path[i] != '\0'; i++) {
        if (!isalnum(path[i]) && path[i] != '/' && path[i] != '_' && path[i] != '-') {
            return 0;
        }
    }
    
    return 1;
}

int main(int argc, char *argv[]) {
    // 默认值
    char *port = "9000";
    char *path = "";
    int debug = 0;
    
    // 解析命令行参数
    if (argc > 1) {
        port = argv[1];
    } else {
        // 尝试从环境变量获取
        char *env_port = getenv("PORT");
        if (env_port != NULL) {
            port = env_port;
        }
    }
    
    if (argc > 2) {
        path = argv[2];
    } else {
        // 尝试从环境变量获取
        char *env_path = getenv("API_PATH");
        if (env_path != NULL) {
            path = env_path;
        }
    }
    
    // 检查是否开启调试
    if (getenv("DEBUG") != NULL) {
        debug = 1;
    }
    
    // 验证端口有效性
    if (!is_valid_port(port)) {
        if (debug) {
            printf("错误: 无效的端口号 '%s'\n", port);
        }
        return 1;
    }
    
    // 验证路径有效性
    if (!is_valid_path(path)) {
        if (debug) {
            printf("错误: 无效的路径 '%s'\n", path);
        }
        return 1;
    }
    
    // 创建socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        if (debug) {
            perror("套接字创建失败");
        }
        return 1;
    }
    
    // 设置连接超时
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout) < 0) {
        if (debug) {
            perror("设置接收超时失败");
        }
    }
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof timeout) < 0) {
        if (debug) {
            perror("设置发送超时失败");
        }
    }
    
    // 设置服务器地址
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(atoi(port));
    
    // 调试输出
    if (debug) {
        printf("健康检查: http://localhost:%s/%s\n", port, path);
    }
    
    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        if (debug) {
            perror("连接失败");
        }
        close(sock);
        return 1;
    }
    
    // 发送HTTP请求以验证路径是否存在
    char request[BUFFER_SIZE];
    sprintf(request, "GET /%s HTTP/1.1\r\nHost: localhost:%s\r\nConnection: close\r\n\r\n", 
            path, port);
    
    if (send(sock, request, strlen(request), 0) < 0) {
        if (debug) {
            perror("发送请求失败");
        }
        close(sock);
        return 1;
    }
    
    // 接收响应
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received < 0) {
        if (debug) {
            perror("接收响应失败");
        }
        close(sock);
        return 1;
    }
    
    // 确保字符串以null结尾
    buffer[bytes_received] = '\0';
    
    // 解析HTTP状态码
    int status_code = 0;
    if (sscanf(buffer, "HTTP/1.%*d %d", &status_code) == 1 || 
        sscanf(buffer, "HTTP/%*d.%*d %d", &status_code) == 1) {
        
        if (debug) {
            printf("HTTP状态码: %d\n", status_code);
        }
        
        // 2xx 状态码表示成功
        if (status_code >= 200 && status_code < 300) {
            if (debug) {
                printf("成功: 服务可用\n");
            }
            close(sock);
            return 0;
        } else {
            if (debug) {
                printf("失败: HTTP错误 %d\n", status_code);
            }
            close(sock);
            return 1;
        }
    } else {
        if (debug) {
            printf("无法解析HTTP响应: %.100s...\n", buffer);
        }
        close(sock);
        return 1;
    }
}
