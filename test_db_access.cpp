#include "ConceptDatabase.hpp"
#include <iostream>

using namespace std;

int main() {
    cout << "测试数据库访问程序" << endl;

    // 创建数据库实例
    ConceptDatabase db;

    // 尝试连接到现有数据库
    if (!db.initialize("concepts-db")) {
        cerr << "无法连接到数据库" << endl;
        return 1;
    }

    // 查看数据库统计
    db.printStatistics();

    // 获取所有概念
    auto concepts = db.getAllConcepts();
    cout << "找到 " << concepts.size() << " 个概念:" << endl;

    for (auto& concept : concepts) {
        cout << "概念ID: " << concept->id << " - 特征数: " << concept->feature_keys.size() << endl;
        for (size_t i = 0; i < concept->feature_keys.size(); i++) {
            cout << "  " << concept->feature_keys[i] << ":" << concept->feature_values[i] << endl;
        }
    }

    return 0;
}