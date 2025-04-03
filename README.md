![GitHub](https://img.shields.io/github/license/samuelba/healthcheck)
[![ci](https://github.com/samuelba/healthcheck/actions/workflows/ci.yml/badge.svg)](https://github.com/samuelba/healthcheck/actions/workflows/ci.yml)
![GitHub tag (latest SemVer)](https://img.shields.io/github/v/tag/samuelba/healthcheck)
![Docker Image Size (latest semver)](https://img.shields.io/docker/image-size/samuelba/healthcheck)

# 健康检查工具

这是一个轻量级的HTTP健康检查工具，用于Docker容器健康状态监控。该工具会发送HTTP请求到指定的端点，并根据HTTP状态码判断服务是否健康。

## 特点

- 超小体积：静态编译，二进制文件约750KB
- 多架构支持：同时提供linux/amd64和linux/arm64版本
- 无依赖：静态链接，不依赖任何外部库
- 符合标准：严格遵循Docker健康检查规范
- 真实检查：验证HTTP状态码而不仅是TCP连接

## 使用方法

### 基本用法

```bash
# 检查默认端口(9000)
./healthcheck-amd64

# 指定端口
./healthcheck-amd64 8080

# 指定端口和路径
./healthcheck-amd64 8080 api/health

# 开启调试输出
DEBUG=1 ./healthcheck-amd64 8080 api/health
```

### 环境变量

- `PORT`：要检查的端口（默认：9000）
- `API_PATH`：要检查的API路径（默认：空）
- `DEBUG`：设置为任意值可开启调试输出

### 在Docker中使用

#### 示例Dockerfile

```dockerfile
FROM alpine:latest

# 复制你的应用程序
COPY ./your_app /app/your_app

# 复制健康检查工具到容器中
COPY ./bin/healthcheck-amd64 /app/healthcheck

WORKDIR /app
EXPOSE 8080

# 设置健康检查配置
ENV PORT=8080
ENV API_PATH=api/health
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
  CMD ["/app/healthcheck"]

CMD ["./your_app"]
```

#### 示例docker-compose.yml

```yaml
version: '3'

services:
  web:
    build: .
    ports:
      - "8080:8080"
    healthcheck:
      test: ["CMD", "/app/healthcheck", "8080", "api/health"]
      interval: 30s
      timeout: 5s
      retries: 3
      start_period: 5s
```

## 退出码

- `0`：健康检查成功（HTTP状态码2xx）
- `1`：健康检查失败（连接失败或HTTP状态码非2xx）

## 输入验证

工具会验证提供的端口和路径参数：

- 端口必须是1-65535之间的数字
- 路径只能包含字母、数字、斜杠(/)、连字符(-)和下划线(_)
- 路径长度限制在100个字符以内

## 编译

使用提供的构建脚本编译：

```bash
chmod +x build-c.sh
./build-c.sh
```

编译结果将生成在`bin`目录下：
- AMD64版本：`./bin/healthcheck-amd64`
- ARM64版本：`./bin/healthcheck-arm64`

## 最佳实践

- 在非调试模式下（不设置DEBUG环境变量），健康检查工具不会输出任何内容，只通过退出码表明健康状态
- 在Docker容器中，确保将健康检查工具复制到容器内
- 如果遇到问题，可以临时启用DEBUG模式进行故障排查
