import os

# 定义根目录路径（可根据实际路径调整）
root_dir = "/home/jzcai/covered-private/output/log/exp_simulation_intercache_latency_v2_p2p"
# 定义要遍历的round序号（0到4）
rounds = range(0, 5)

def extract_avg_latency(file_path):
    """从单个文件中提取Avg Latency (ms)数据"""
    avg_latency = None
    # 标记是否找到目标统计块
    in_stats_block = False
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                # 1. 找到Final Stresstest Statistics块的开始
                if line == "[Final Stresstest Statistics]":
                    in_stats_block = True
                    continue
                # 2. 进入块后，找到包含Avg Latency的表格行
                if in_stats_block and "Avg Latency (ms)" in line:
                    # 3. 读取下一行（表格数据行）
                    data_line = next(f).strip()
                    # 4. 按|分割数据，提取第二列（Avg Latency值）
                    # 分割后去掉空字符串和空格，取索引1的元素
                    columns = [col.strip() for col in data_line.split('|') if col.strip()]
                    if len(columns) >= 2:
                        avg_latency = columns[1]
                    # 找到后退出循环，避免重复处理
                    break
    except Exception as e:
        print(f"处理文件 {file_path} 时出错：{e}")
    return avg_latency

# 主逻辑：遍历所有round目录和文件
if __name__ == "__main__":
    print("文件名 | Avg Latency (ms)")
    print("-" * 50)
    
    for round_num in rounds:
        # 构建每个round的目录路径
        round_dir = os.path.join(root_dir, f"round{round_num}")
        # 检查目录是否存在
        if not os.path.exists(round_dir):
            print(f"目录 {round_dir} 不存在，跳过")
            continue
        print(round_dir)
        # 遍历目录下的所有文件
        for filename in os.listdir(round_dir):
            file_path = os.path.join(round_dir, filename)
            # 只处理文件（跳过子目录）
            if os.path.isfile(file_path):
                latency = extract_avg_latency(file_path)
                # 去掉目录只留下文件名
                filename_only = os.path.basename(file_path)
                if latency:
                    print(f"{filename_only} | {latency}")
                else:
                    print(f"{filename_only} | 未找到Avg Latency数据")