import numpy as np
from scipy.stats import uniform, norm, poisson, pareto
import json
import os
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

def generate_symmetric_delay_matrix(n, dist, params, bounds):
    """
    Generate symmetric network delay matrix with uint32 values
    
    Parameters:
        n (int): Number of network nodes (n ≥ 2)
        dist (str): Distribution type, options: "uniform", "normal", "poisson", "long_tail", "mixed_link"
        params (tuple): Distribution parameters (matching dist type)
            - uniform: (loc, scale) → loc=left boundary, scale=interval length
            - normal: (mean, std) → mean value, standard deviation (std > 0)
            - poisson: (lambda,) → rate parameter (lambda > 0)
            - long_tail: (k,) → shape parameter (k > 0)
            - mixed_link: 无实际参数（仅标记类型）
        bounds (tuple): Delay range (left, right) as integers, must satisfy 0 ≤ left < right ≤ 4294967295
    
    Returns:
        np.ndarray: n×n symmetric delay matrix with uint32 values
    
    Raises:
        ValueError: When input parameters are invalid
    """
    # Parameter validation
    if not isinstance(n, int) or n < 2:
        raise ValueError(f"Number of nodes must be integer ≥ 2, current input: {n}")
    
    left_bound, right_bound = bounds
    # Ensure bounds are integers within uint32 range
    if not (isinstance(left_bound, int) and isinstance(right_bound, int)):
        raise ValueError(f"Bounds must be integers for uint32 type, current input: {bounds}")
    if left_bound < 0 or right_bound > 4294967295:
        raise ValueError(f"Bounds must be within uint32 range [0, 4294967295], current input: {bounds}")
    if left_bound >= right_bound:
        raise ValueError(f"Left bound must be less than right bound, current input: {bounds}")
    
    # Distribution validation (新增mixed_link类型支持，无额外参数校验)
    if dist == "uniform":
        if len(params) != 2 or params[1] <= 0:
            raise ValueError(f"Uniform distribution requires (loc, scale) with scale>0, current: {params}")
        loc, scale = params
        dist_func = uniform(loc=loc, scale=scale)
    elif dist == "normal":
        if len(params) != 2 or params[1] <= 0:
            raise ValueError(f"Normal distribution requires (mean, std) with std>0, current: {params}")
        mean, std = params
        dist_func = norm(loc=mean, scale=std)
    elif dist == "poisson":
        if len(params) != 1 or params[0] <= 0:
            raise ValueError(f"Poisson distribution requires (lambda,) with lambda>0, current: {params}")
        lam = params[0]
        dist_func = poisson(mu=lam)
    elif dist == "long_tail":
        if len(params) != 1 or params[0] <= 0:
            raise ValueError(f"Long tail distribution requires (k,) with k>0, current: {params}")
        k = params[0]
    elif dist == "mixed_link":
        # mixed_link类型无需初始化分布函数（后续强制填充INT_MAX）
        dist_func = None
    else:
        raise ValueError(f"Unsupported distribution: {dist}, options: uniform/normal/poisson/long_tail/mixed_link")

    # Initialize delay matrix with uint32 type
    # 强制所有延迟值为uint32最大值（INT_MAX = 4294967295），忽略原有分布逻辑
    INT_MAX = 4294967295
    delay_matrix = np.full((n, n), INT_MAX, dtype=np.uint32)

    return delay_matrix


def calculate_average_delay(matrix):
    """Calculate average delay, ignoring diagonal zeros"""
    n = matrix.shape[0]
    if n < 2:
        return 0.0
    # Sum all elements except diagonal
    total = matrix.sum() - np.trace(matrix)
    count = n * (n - 1)
    return total / count if count != 0 else 0.0


def plot_delay_histogram(matrix, output_file, dist_type, params=None):
    """Plot delay distribution histogram and save to file"""
    n = matrix.shape[0]
    mask = np.triu(np.ones((n, n), dtype=bool), k=1)
    delays = matrix[mask]
    
    avg_delay = calculate_average_delay(matrix)
    
    plt.figure(figsize=(10, 6))
    n_bins, bins, patches = plt.hist(delays, bins=30, alpha=0.7, color='#4CAF50', edgecolor='black')
    
    plt.axvline(avg_delay, color='red', linestyle='dashed', linewidth=2, 
                label=f'Average: {avg_delay:.2f} ms')
    
    title = f'Network Delay Distribution (Type: {dist_type})'
    if dist_type == 'long_tail' and params and len(params) > 0:
        title += f', k={params[0]}'
    plt.title(title, fontsize=14)
    plt.xlabel('Delay (ms)', fontsize=12)
    plt.ylabel('Frequency', fontsize=12)
    
    plt.gca().xaxis.set_major_formatter(FuncFormatter(lambda x, _: f'{int(x)}'))
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.legend()
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    
    print(f"Histogram saved to: {os.path.abspath(output_file)}")


def save_delay_config(n, dist_type, matrix, bounds, output_file, params=None):
    """Save delay configuration to JSON file"""
    left_bound, right_bound = bounds
    
    config = {
        "node_count": n,
        "distribution": dist_type,
        "parameters": {
            "low": left_bound,  # Integer value for uint32
            "high": right_bound,  # Integer value for uint32
            "average_delay": float(calculate_average_delay(matrix))
        },
        "delay_matrix": matrix.tolist()  # Will be saved as integers
    }
    
    # 新增mixed_link类型的参数存储（无额外参数，仅保留结构一致性）
    if dist_type == 'uniform' and params and len(params) == 2:
        config["parameters"]["loc"] = float(params[0])
        config["parameters"]["scale"] = float(params[1])
    elif dist_type == 'normal' and params and len(params) == 2:
        config["parameters"]["mean"] = float(params[0])
        config["parameters"]["std"] = float(params[1])
    elif dist_type == 'poisson' and params and len(params) == 1:
        config["parameters"]["lambda"] = float(params[0])
    elif dist_type == 'long_tail' and params and len(params) == 1:
        config["parameters"]["shape"] = float(params[0])
    elif dist_type == 'mixed_link':
        config["parameters"]["type"] = "full_INT_MAX"  # 标记mixed_link类型特征

    output_dir = os.path.dirname(output_file)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    with open(output_file, 'w') as f:
        json.dump(config, f, indent=4)
    
    print(f"Configuration saved to: {os.path.abspath(output_file)}")


def print_delay_matrix(matrix, delay_unit="ms", max_rows=10):
    """Print delay matrix summary with integer values"""
    n = matrix.shape[0]
    print(f"Network delay matrix (unit: {delay_unit}, showing first {min(n, max_rows)} rows/columns):")
    print(f"{'':<6}" + "".join([f"Node{j:<8}" for j in range(min(n, max_rows))]))
    for i in range(min(n, max_rows)):
        row_str = f"Node{i:<4}"
        for j in range(min(n, max_rows)):
            # Print as integer since we're using uint32
            row_str += f"{matrix[i][j]:<8d}"
        print(row_str)
    if n > max_rows:
        print(f"... omitting remaining {n - max_rows} rows/columns ...")


if __name__ == "__main__":
    # Configuration parameters - 仅调整distribution_type为mixed_link，其余保持不变
    node_count = 128                # Number of nodes
    distribution_type = "mixed_link"# 类型改为mixed_link
    delay_bounds = (2000, 12000)   # Delay range in ms (保持原配置不变)
    
    # Set distribution parameters (mixed_link无需实际参数，传空元组即可)
    if distribution_type == "mixed_link":
        dist_params = ()
    base_filename = "empty"
    # Generate delay matrix with uint32 values (此时矩阵全为INT_MAX)
    delay_matrix = generate_symmetric_delay_matrix(
        n=node_count,
        dist=distribution_type,
        params=dist_params,
        bounds=delay_bounds
    )
    
    # Print matrix summary (可看到所有非对角元素均为4294967295)
    print_delay_matrix(delay_matrix)
    avg_delay = calculate_average_delay(delay_matrix)
    print(f"\nAverage delay: {avg_delay:.2f} ms")  # 平均值也为4294967295.00
    
    # Save to JSON (配置文件中distribution标记为mixed_link)
    json_filename = f"{base_filename}.json"
    save_delay_config(
        n=node_count,
        dist_type=distribution_type,
        matrix=delay_matrix,
        bounds=delay_bounds,
        output_file=json_filename,
        params=dist_params
    )
    