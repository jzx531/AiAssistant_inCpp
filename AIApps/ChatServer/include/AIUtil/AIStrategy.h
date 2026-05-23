#ifndef AI_STRATEGY_H
#define AI_STRATEGY_H

#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <sstream>
#include <memory>

#include "../../../../HttpServer/include/utils/JsonUtil.h"

class AIStrategy{
public:
    virtual ~AIStrategy() = default;

    virtual std::string getApiUrl() const = 0;

    //API key
    virtual std::string getApiKey() const = 0;

    virtual std::string getModel() const = 0;

    virtual json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const = 0;

    virtual std::string parseResponse(const json & response) const = 0;

    bool isMCPModel = false;
};

class AliyunStrategy : public AIStrategy{

public:
    AliyunStrategy(){
        const char * key = std::getenv("DASHSCOPE_API_KEY");
        if(!key) throw std::runtime_error("Aliyun API Key not found!");
        apiKey_ = key;
        isMCPModel = false;
    }
    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    

}

#endif

