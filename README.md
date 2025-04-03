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

在Dockerfile中：

```Dockerfile
# 复制健康检查工具到容器中
COPY --from=health-check-image --chown=nonroot:nonroot /bin/healthcheck-amd64 /app/healthcheck

# 设置健康检查配置
ENV PORT=8080
ENV API_PATH=api/health
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
  CMD ["/app/healthcheck"]
```

在docker-compose.yml中：

```yaml
services:
  your-service:
    build: .
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

## 编译

使用提供的构建脚本编译：

```bash
chmod +x build-c.sh
./build-c.sh
```

编译结果将生成在`bin`目录下：
- AMD64版本：`./bin/healthcheck-amd64`
- ARM64版本：`./bin/healthcheck-arm64`

# Health Check

A minimal health check. Calls the defined API and exits either with 0 (success) or 1 (failure).

## Usage

Example `Dockerfile` file

```Dockerfile
FROM rust:1.68 as builder

# Make use of cache for dependencies.
RUN USER=root cargo new --bin your_app
WORKDIR ./your_app
COPY ./Cargo.lock ./Cargo.lock
COPY ./Cargo.toml ./Cargo.toml
RUN cargo build --release && \
    rm src/*.rs

# Build the app.
COPY . ./
RUN rm ./target/release/deps/your_app*
RUN cargo build --release

# Use distroless as minimal base image to package the app.
FROM gcr.io/distroless/cc-debian11:nonroot

COPY --from=builder --chown=nonroot:nonroot /your_app/target/release/your_app /app/your_app
COPY --from=samuelba/healthcheck:latest --chown=nonroot:nonroot /app/healthcheck /app/healthcheck
USER nonroot
WORKDIR /app
EXPOSE 9000

# Define the port and API path for the healthcheck.
# The health check will call http://localhost:PORT/API_PATH.
ENV PORT=9000
ENV API_PATH=api/v1/health
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s CMD ["/app/healthcheck"]

CMD ["./your_app"]
