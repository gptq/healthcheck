#!/bin/bash

# 编译 x86_64 (AMD64) 版本
echo "正在编译 linux/amd64 版本..."
cargo build --release --target x86_64-unknown-linux-musl

# 为 ARM64 版本设置环境变量
echo "正在编译 linux/arm64 版本..."
# 使用gcc-aarch64-linux-gnu作为链接器
export CC_aarch64_unknown_linux_musl=aarch64-linux-gnu-gcc
export CARGO_TARGET_AARCH64_UNKNOWN_LINUX_MUSL_LINKER=aarch64-linux-gnu-gcc
cargo build --release --target aarch64-unknown-linux-musl

# 创建输出目录
mkdir -p ./bin

# 复制编译结果到输出目录
cp ./target/x86_64-unknown-linux-musl/release/healthcheck ./bin/healthcheck-amd64
cp ./target/aarch64-unknown-linux-musl/release/healthcheck ./bin/healthcheck-arm64

echo "编译完成！"
echo "amd64版本: ./bin/healthcheck-amd64"
echo "arm64版本: ./bin/healthcheck-arm64"
