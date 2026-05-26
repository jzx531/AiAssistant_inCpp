#ifndef GOMOKUAI_H
#define GOMOKUAI_H

#include <utility>
#include <string>
#include <vector>

#include <curl/curl.h>

#include "../../../../HttpServer/include/utils/JsonUtil.h"

class GomokuAI {
public:
    GomokuAI();
    ~GomokuAI() = default;

    std::pair<int, int> makeMove(const std::vector<std::vector<int>>& board);

private:
    std::string buildPrompt(const std::vector<std::vector<int>>& board) const;
    std::string postJson(const json& payload) const;
    std::pair<int, int> parseMove(const std::string& responseText) const;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

private:
    std::string url_;
    std::string model_;
};

#endif


