#include"../include/AIUtil/AIHelper.h"
#include"../include/AIUtil/MQManager.h"
#include <stdexcept>
#include<chrono>

//构造函数
AIHelper::AIHelper(){
    //莫认使用阿里云大模型
    strategy = StrategyFactory::instance().create("1");
}

void AIHelper::setStrategy(std::shared_ptr<AIStrategy> strat){
    strategy = strat;
}

// 设置默认模型
//void AIHelper::setModel(const std::string& modelName) {
  //  model_ = modelName;
//}

// 添加一条用户消息
void AIHelper::addMessage(int userId,const std::string& userName, bool is_user,const std::string& userInput, std::string sessionId) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    messages.push_back({userInput,ms});
    //消息队列异步入库
    pushMessageToMysql(userId, userName, is_user, userInput,  ms,sessionId);
}


void AIHelper::restoreMessage(const std::string& userInput,long long ms){
    messages.push_back({userInput,ms});
}

//发送聊天消息
std::string AIHelper::chat(int userId,std::string userName,std::string sessionId,std::string userQuestion,std::string modelType)
{
    //设置策略
    setStrategy(StrategyFactory::instance().create(modelType));

    if(false == strategy->isMCPModel){
        addMessage(userId, userName, true, userQuestion, sessionId);
        json payload = strategy->buildRequest(this->messages);
        //执行请求
        json response = executeCurl(payload);
        std::string answer = strategy->parseResponse(response);
        addMessage(userId,userName,false,answer,sessionId);
        return answer.empty() ? "[Error] 无法解析响应" : answer;
    }

    // 说明支持MCP
    AIConfig config;
    config.loadFromFile("../AIApps/ChatServer/resource/config.json");
    std::string tempUserQuestion = config.buildPrompt(userQuestion);
    std::cout << "tempUserQuestion is "<< tempUserQuestion << std::endl;
    messages.push_back({tempUserQuestion,0});

    json firstReq = strategy->buildRequest(this->messsages);
    json firstResp = executeCurl(firstReq);
    std::string aiResult = strategy->parseResponse(firstResp);
    // 用完立即移除提示词
    messages.pop_back();

    std::cout << "aiResult is " << aiResult << std::endl;
    //解析AI响应(是否工具调用)
    AIToolCall call = config.parseAIResponse(aiResult);

    // 情况1：AI 不调用工具
    if (!call.isToolCall) {
        addMessage(userId, userName, true, userQuestion, sessionId);
        addMessage(userId, userName, false, aiResult, sessionId);

        std::cout << "No tools required" << std::endl;
        return aiResult;
    }

    // 情况2 ： ai调用工具
    json toolResult;
    AIToolRegistry registry;

    try{
        toolResult = registry.invoke(call.toolName,call.args);
        std::cout << "Tool calll success" << std::endl;
    }catch(const std::exception & e){
        //大多数情况都不会走这里
        
    }
}







