#ifndef DELAY_CONFIG_READER_H
#define DELAY_CONFIG_READER_H

#include <vector>
#include <string>
#include <map>
#include <cstdint>  // 包含uint32_t

struct DelayConfig {
    int node_count;
    std::string distribution;
    std::map<std::string, uint32_t> parameters;  // 参数为uint32_t
    std::vector<std::vector<uint32_t>> delay_matrix;  // 延迟矩阵（vector存储）
};

class DelayConfigReader {
public:
    // 从文件读取配置
    static DelayConfig readFromFile(const std::string& filename);
    // 验证配置有效性
    static bool validateConfig(const DelayConfig& config);
    // 获取节点间延迟
    static uint32_t getDelay(const DelayConfig& config, int node1, int node2);
};

#endif // DELAY_CONFIG_READER_H