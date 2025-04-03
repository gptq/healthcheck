#!/bin/bash

echo "正在编译C版本的健康检查工具..."

# 确保bin目录存在
mkdir -p ./bin

# 编译AMD64版本
echo "编译 linux/amd64 版本..."
gcc -static -o ./bin/healthcheck-amd64 healthcheck.c

# 编译ARM64版本
echo "编译 linux/arm64 版本..."
aarch64-linux-gnu-gcc -static -o ./bin/healthcheck-arm64 healthcheck.c

# 检查是否有MinGW交叉编译工具用于Windows
if command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "编译 windows/amd64 版本..."
    x86_64-w64-mingw32-gcc -o ./bin/healthcheck-windows-amd64.exe healthcheck.c -lws2_32
    
    # 添加可执行权限
    chmod +x ./bin/healthcheck-windows-amd64.exe
    
    # 检查编译后的Windows二进制文件
    echo "Windows AMD64版本文件类型:"
    file ./bin/healthcheck-windows-amd64.exe
else
    echo "警告: 未找到MinGW交叉编译工具，跳过Windows版本编译"
    echo "可以通过安装 mingw-w64 来启用Windows构建:"
    echo "sudo apt-get install -y mingw-w64"
fi

# 添加可执行权限
chmod +x ./bin/healthcheck-amd64 ./bin/healthcheck-arm64

# 检查编译后的二进制文件类型
echo "AMD64版本文件类型:"
file ./bin/healthcheck-amd64
echo "ARM64版本文件类型:"
file ./bin/healthcheck-arm64

# 测试正负样本
echo ""
echo "====== 测试健康检查工具 ======"
echo ""

# 测试正样本 - 本地通常会有一些服务在监听22端口(SSH)
echo "测试正样本 - 可用服务(端口22):"
./bin/healthcheck-amd64 22
if [ $? -eq 0 ]; then
    echo "✅ 检测结果：服务正常"
else
    echo "❌ 检测结果：服务不可用"
fi

# 测试负样本 - 使用一个不太可能被使用的端口
echo ""
echo "测试负样本 - 不可用服务(端口54321):"
./bin/healthcheck-amd64 54321
if [ $? -eq 0 ]; then
    echo "❌ 检测结果：服务被错误地检测为可用"
else
    echo "✅ 检测结果：正确检测到服务不可用"
fi

echo ""
echo "编译完成！"
echo "AMD64版本: ./bin/healthcheck-amd64"
echo "ARM64版本: ./bin/healthcheck-arm64"
if command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "Windows AMD64版本: ./bin/healthcheck-windows-amd64.exe"
fi
