use std::env;
use std::process::exit;

#[tokio::main(flavor = "current_thread")]
async fn main() {
    // 获取命令行参数
    let args: Vec<String> = env::args().collect();
    
    // 解析端口：优先使用命令行参数，其次使用环境变量，最后使用默认值
    let port = if args.len() > 1 {
        args[1].clone()
    } else {
        match env::var("PORT") {
            Ok(val) => val,
            Err(_) => "9000".to_string(),
        }
    };
    
    // 解析API路径：优先使用命令行参数，其次使用环境变量，最后使用默认值
    let path = if args.len() > 2 {
        args[2].clone()
    } else {
        match env::var("API_PATH") {
            Ok(val) => val,
            Err(_) => "".to_string(),
        }
    };

    // Docker健康检查不需要详细输出，仅在调试模式下输出
    let debug = env::var("DEBUG").is_ok();
    if debug {
        println!("健康检查: http://localhost:{}/{}", port, path);
    }

    let url = format!("http://localhost:{}/{}", port, path);
    let client = reqwest::Client::new();
    let res = client.get(url).send().await;
    match res {
        Ok(res) => {
            let status = res.status();
            if !status.is_success() {
                if debug {
                    println!("失败: {}", status);
                }
                exit(1)
            }
            if debug {
                println!("成功: {}", status);
            }
            exit(0)
        }
        Err(e) => {
            if debug {
                println!("错误: {}", e);
            }
            exit(1)
        },
    }
}
