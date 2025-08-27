#include "delay_config_reader.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstdint>

using json = nlohmann::json;
using namespace std;

DelayConfig DelayConfigReader::readFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("无法打开文件: " + filename);
    }

    // 将json对象变量名从j改为json_data，避免与循环变量j冲突
    json json_data;
    try {
        file >> json_data;
    } catch (const exception& e) {
        throw runtime_error("JSON解析失败: " + string(e.what()));
    }

    DelayConfig config;
    try {
        config.node_count = json_data["node_count"];
        config.distribution = json_data["distribution"];

        // 读取参数（转换为uint32_t）
        config.parameters["low"] = json_data["parameters"]["low"].get<uint32_t>();
        config.parameters["high"] = json_data["parameters"]["high"].get<uint32_t>();
        if (config.distribution == "long_tail") {
            // 形状参数可能是浮点数，用double存储
            config.parameters["shape"] = static_cast<uint32_t>(json_data["parameters"]["shape"].get<double>());
        }

        // 读取延迟矩阵（vector<vector<uint32_t>>）
        config.delay_matrix.resize(config.node_count);
        for (int i = 0; i < config.node_count; ++i) {
            config.delay_matrix[i].resize(config.node_count);
            for (int j = 0; j < config.node_count; ++j) {
                // 使用json_data访问JSON数据，避免与循环变量j冲突
                config.delay_matrix[i][j] = json_data["delay_matrix"][i][j].get<uint32_t>();
            }
        }
    } catch (const exception& e) {
        throw runtime_error("配置解析失败: " + string(e.what()));
    }

    if (!validateConfig(config)) {
        throw runtime_error("配置无效");
    }

    return config;
}

bool DelayConfigReader::validateConfig(const DelayConfig& config) {
    // 验证节点数量
    if (config.node_count <= 0) {
        cerr << "无效节点数量: " << config.node_count << endl;
        return false;
    }

    // // 验证分布类型
    // if (config.distribution != "uniform" && config.distribution != "long_tail") {
    //     cerr << "不支持的分布类型: " << config.distribution << endl;
    //     return false;
    // }

    // 验证参数范围
    auto low = config.parameters.at("low");
    auto high = config.parameters.at("high");
    if (low >= high) {
        cerr << "无效范围: low(" << low << ") >= high(" << high << ")" << endl;
        return false;
    }

    // 验证矩阵尺寸
    if (config.delay_matrix.size() != (size_t)config.node_count) {
        cerr << "矩阵行数不符" << endl;
        return false;
    }
    for (const auto& row : config.delay_matrix) {
        if (row.size() != (size_t)config.node_count) {
            cerr << "矩阵列数不符" << endl;
            return false;
        }
    }

    // 验证矩阵对称性和对角线（uint32_t特性）
    for (int i = 0; i < config.node_count; ++i) {
        if (config.delay_matrix[i][i] != 0) {  // 自身延迟为0
            cerr << "对角线不为0（节点" << i << "）" << endl;
            return false;
        }
        for (int j = i + 1; j < config.node_count; ++j) {
            if (config.delay_matrix[i][j] != config.delay_matrix[j][i]) {  // 对称性
                cerr << "矩阵不对称（节点" << i << "与" << j << "）" << endl;
                return false;
            }
            if (config.delay_matrix[i][j] < low || config.delay_matrix[i][j] > high) {  // 范围验证
                cerr << "延迟值超出范围（节点" << i << "与" << j << "：" << config.delay_matrix[i][j] << "）" << endl;
                return false;
            }
        }
    }

    return true;
}

uint32_t DelayConfigReader::getDelay(const DelayConfig& config, int node1, int node2) {
    if (node1 < 0 || node1 >= config.node_count || node2 < 0 || node2 >= config.node_count) {
        throw out_of_range("节点索引超出范围");
    }
    return config.delay_matrix[node1][node2];
}
    