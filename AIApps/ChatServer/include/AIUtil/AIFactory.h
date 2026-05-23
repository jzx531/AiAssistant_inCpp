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
    std::unordered_map<std::string,Creator> creators_;
};

#endif // AIFACTORY_H
