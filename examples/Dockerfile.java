FROM maven:3.8-openjdk-11 AS build
WORKDIR /app
COPY pom.xml .
COPY src ./src
RUN mvn package -DskipTests

FROM openjdk:11-jre-slim

WORKDIR /app
# 从构建阶段复制JAR文件
COPY --from=build /app/target/*.jar app.jar

# 下载健康检查工具（根据容器架构自动选择适当的版本）
RUN apt-get update && apt-get install -y curl \
    && ARCH=$(uname -m) \
    && if [ "$ARCH" = "x86_64" ]; then \
         BINARY_NAME="healthcheck-amd64"; \
       elif [ "$ARCH" = "aarch64" ]; then \
         BINARY_NAME="healthcheck-arm64"; \
       else \
         echo "不支持的架构: $ARCH" && exit 1; \
       fi \
    && curl -L "https://github.com/gptq/healthcheck/releases/latest/download/$BINARY_NAME" -o /usr/local/bin/healthcheck \
    && chmod +x /usr/local/bin/healthcheck \
    && apt-get purge -y curl \
    && apt-get autoremove -y \
    && rm -rf /var/lib/apt/lists/*

# 配置应用服务
EXPOSE 8080
ENV PORT=8080
ENV API_PATH=actuator/health

# 配置健康检查
HEALTHCHECK --interval=30s --timeout=5s --start-period=30s --retries=3 \
  CMD ["healthcheck", "8080", "actuator/health"]

# 启动应用
CMD ["java", "-jar", "app.jar"]
