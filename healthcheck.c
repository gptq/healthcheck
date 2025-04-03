#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>  // 用于字符检查
#include <errno.h>  // 用于错误处理

#define BUFFER_SIZE 4096
#define MAX_PATH_LENGTH 100
#define RECV_TIMEOUT_SEC 5
#define CONNECT_TIMEOUT_SEC 5

// 检查端口是否有效
int is_valid_port(const char *port_str) {
    // 检查是否为空
    if (port_str == NULL || *port_str == '\0') {
        return 0;
    }
    
    // 检查是否全是数字
    for (int i = 0; port_str[i] != '\0'; i++) {
        if (!isdigit(port_str[i])) {
            return 0;
        }
    }
    
    // 转换为整数并检查范围
    long port = strtol(port_str, NULL, 10);
    return (port > 0 && port <= 65535);
}

// 检查路径是否有效（只允许字母、数字、下划线、连字符和/）
int is_valid_path(const char *path) {
    // 检查是否为空
    if (path == NULL) {
        return 0;
    }
    
    // 检查路径长度
    size_t path_len = strlen(path);
    if (path_len > MAX_PATH_LENGTH) {
        return 0;
    }
    
    // 检查是否包含危险序列
    if (strstr(path, "..") != NULL) {
        return 0;
    }
    
    // 不允许以 / 开头（防止绝对路径）
    if (path_len > 0 && path[0] == '/') {
        return 0;
    }
    
    // 检查字符有效性
    for (size_t i = 0; i < path_len; i++) {
        if (!isalnum(path[i]) && path[i] != '/' && path[i] != '_' && path[i] != '-') {
            return 0;
        }
    }
    
    return 1;
}

// 安全地接收HTTP响应
int receive_response(int sock, char *buffer, size_t buffer_size, int debug) {
    ssize_t bytes_received = 0;
    size_t total_bytes = 0;
    
    // 确保缓冲区有足够空间
    if (buffer_size < 1) {
        return -1;
    }
    
    // 接收数据，直到接收完成或缓冲区满
    while ((bytes_received = recv(sock, buffer + total_bytes, buffer_size - total_bytes - 1, 0)) > 0) {
        total_bytes += bytes_received;
        
        // 检查是否已接收完HTTP头
        buffer[total_bytes] = '\0';
        if (strstr(buffer, "\r\n\r\n") != NULL) {
            break;
        }
        
        // 防止缓冲区溢出
        if (total_bytes >= buffer_size - 1) {
            break;
        }
    }
    
    if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        if (debug) {
            perror("接收响应失败");
        }
        return -1;
    }
    
    // 确保字符串以null结尾
    buffer[total_bytes] = '\0';
    return total_bytes;
}

// 解析HTTP响应
int parse_http_response(const char *buffer, int debug) {
    // 检查最小响应长度
    if (strlen(buffer) < 12) { // "HTTP/1.1 200" 至少需要12个字符
        if (debug) {
            printf("响应数据不足\n");
        }
        return -1;
    }
    
    // 解析HTTP状态码
    int status_code = 0;
    char http_version[16] = {0};
    
    if (sscanf(buffer, "%15s %d", http_version, &status_code) != 2 ||
        strncmp(http_version, "HTTP/", 5) != 0) {
        if (debug) {
            printf("无法解析HTTP响应格式: %.100s...\n", buffer);
        }
        return -1;
    }
    
    if (debug) {
        printf("HTTP状态码: %d\n", status_code);
    }
    
    return status_code;
}

int main(int argc, char *argv[]) {
    // 默认值
    char *port = "9000";
    char *path = "";
    int debug = 0;
    int result = 1; // 默认失败
    
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
    timeout.tv_sec = RECV_TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout) < 0) {
        if (debug) {
            perror("设置接收超时失败");
        }
    }
    
    timeout.tv_sec = CONNECT_TIMEOUT_SEC;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof timeout) < 0) {
        if (debug) {
            perror("设置发送超时失败");
        }
    }
    
    // 设置服务器地址
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // 安全地转换端口
    long port_num = strtol(port, NULL, 10);
    if (port_num <= 0 || port_num > 65535) {
        if (debug) {
            printf("错误: 端口号超出范围\n");
        }
        close(sock);
        return 1;
    }
    server.sin_port = htons((uint16_t)port_num);
    
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
    
    do {
        // 准备HTTP请求
        char request[BUFFER_SIZE];
        int req_len = snprintf(request, BUFFER_SIZE, 
                            "GET /%s HTTP/1.1\r\nHost: localhost:%s\r\nConnection: close\r\n\r\n", 
                            path, port);
        
        if (req_len < 0 || req_len >= BUFFER_SIZE) {
            if (debug) {
                printf("错误: 请求构建失败或请求过长\n");
            }
            break;
        }
        
        // 发送HTTP请求
        if (send(sock, request, strlen(request), 0) < 0) {
            if (debug) {
                perror("发送请求失败");
            }
            break;
        }
        
        // 接收响应
        char buffer[BUFFER_SIZE];
        int recv_result = receive_response(sock, buffer, BUFFER_SIZE, debug);
        
        if (recv_result < 0) {
            if (debug) {
                printf("接收响应失败\n");
            }
            break;
        }
        
        // 解析HTTP状态码
        int status_code = parse_http_response(buffer, debug);
        
        if (status_code < 0) {
            // 解析失败
            break;
        }
        
        // 2xx 状态码表示成功
        if (status_code >= 200 && status_code < 300) {
            if (debug) {
                printf("成功: 服务可用\n");
            }
            result = 0; // 成功
        } else {
            if (debug) {
                printf("失败: HTTP错误 %d\n", status_code);
            }
        }
    } while (0);
    
    // 关闭套接字
    close(sock);
    return result;
}
