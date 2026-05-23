#ifndef AIFACTORY_H
#define AIFACTORY_H

#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <sstream>
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>


#include"AIStrategy.h"

class StrategyFactory {
public:
    using Creator = std::function<std::shared_ptr<AIStrategy>()>;

    static StrategyFactory& instance();

    void registerStrategy(const std::string &name, Creator creator);

    std::shared_ptr<AIStrategy> create(const std::string &name);

private:

    StrategyFactory() = default;
    std::unordered_map<std::string,Creator> creators;
};

/*
一行代码完成注册（通常在 .cpp 文件中）
你不需要去修改工厂类的代码，只需要在 AStarStrategy.cpp 里加一行：
//当程序启动时，这个全局变量会自动把 "AStar" 和 AStarStrategy 绑定到工厂里
static StrategyRegister<AStarStrategy> reg("AStar"); 
*/

template<typename T>
struct StrategyRegister {
    StrategyRegister(const std::string& name) {
        StrategyFactory::instance().registerStrategy(name, [] {
            std::shared_ptr<AIStrategy> instance = std::make_shared<T>();
            return instance;
            });
    }
};

#endif // AIFACTORY_H
