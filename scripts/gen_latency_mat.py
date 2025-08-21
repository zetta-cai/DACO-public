import numpy as np
from scipy.stats import pareto
import json
import os

def generate_delay_matrix(n, dist_type, low=1, high=100, **kwargs):
    """生成uint32_t类型的对称延迟矩阵（单位：ms）"""
    if high <= low or low < 0:
        raise ValueError("上界必须大于下界，且下界不能为负")
    if high > 4294967295:  # uint32_t最大值
        raise ValueError("上界超过uint32_t最大值(4294967295)")
    
    delay_matrix = np.zeros((n, n), dtype=np.uint32)  # 初始化uint32矩阵
    
    for i in range(n):
        for j in range(i + 1, n):
            if dist_type == 'uniform':
                # 均匀分布：整数随机值 [low, high]
                val = np.random.randint(low, high + 1)  # 包含high
            
            elif dist_type == 'long_tail':
                # 长尾分布：生成整数并截断
                k = kwargs.get('k', 1.5)
                if k <= 0:
                    raise ValueError("shape参数k必须>0")
                
                # 生成长尾分布值并转换为整数
                val = low * (pareto.rvs(k) + 1)
                val = int(round(val))  # 取整
                val = max(low, min(val, high))  # 截断到[low, high]
            
            else:
                raise ValueError("分布类型仅支持 'uniform' 或 'long_tail'")
            
            delay_matrix[i][j] = val
            delay_matrix[j][i] = val
    
    return delay_matrix

def save_delay_config(n, dist_type, matrix, low, high, output_file,** kwargs):
    """保存为JSON配置文件"""
    config = {
        "node_count": n,
        "distribution": dist_type,
        "parameters": {
            "low": low,
            "high": high
        },
        "delay_matrix": matrix.tolist()  # 整数列表
    }
    
    if dist_type == 'long_tail':
        config["parameters"]["shape"] = kwargs.get('k', 1.5)
    
    with open(output_file, 'w') as f:
        json.dump(config, f, indent=4)
    
    print(f"配置文件已保存至: {os.path.abspath(output_file)}")

if __name__ == "__main__":
    # 配置参数（单位：ms，uint32_t类型）
    node_count = 128
    distribution_type = "long_tail"  # 可选 "uniform" 或 "long_tail"
    delay_low = 1000      # 最小延迟（ms）
    delay_high = 5000    # 最大延迟（ms）
    output_filename = "delay_config_uint32.json"
    
    if distribution_type == 'long_tail':
        delay_matrix = generate_delay_matrix(
            n=node_count,
            dist_type=distribution_type,
            low=delay_low,
            high=delay_high,
            k=1.2
        )
        save_delay_config(node_count, distribution_type, delay_matrix, 
                         delay_low, delay_high, output_filename, k=1.2)
    else:
        delay_matrix = generate_delay_matrix(
            n=node_count,
            dist_type=distribution_type,
            low=delay_low,
            high=delay_high
        )
        save_delay_config(node_count, distribution_type, delay_matrix, 
                         delay_low, delay_high, output_filename)