#include "../include/aigame/GomokuAI.h"

#include <stdexcept>
#include <iostream>
#include <regex>

GomokuAI::GomokuAI()
    : url_("http://172.22.48.1:11434/api/generate"),
      model_("qwen2.5:7b")
{
}

size_t GomokuAI::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    const size_t totalSize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<const char*>(contents), totalSize);
    return totalSize;
}

std::string GomokuAI::buildPrompt(const std::vector<std::vector<int>>& board) const
{
    json stones = json::array();
    for (int row = 0; row < static_cast<int>(board.size()); ++row)
    {
        for (int col = 0; col < static_cast<int>(board[row].size()); ++col)
        {
            if (board[row][col] != 0)
            {
                stones.push_back({
                    {"x", col},
                    {"y", row},
                    {"v", board[row][col]}
                });
            }
        }
    }

    return std::string(
        "You are a Gomoku AI playing white stones (2) on a 16x16 board.\n"
        "Cell values: 0 means empty, 1 means black, 2 means white.\n"
        "Coordinates use x for column and y for row, both in range [0,15].\n"
        "Return exactly one JSON object only in this format: {\"x\":col,\"y\":row}\n"
        "Do not return markdown, comments, explanation, code fences, or any extra text.\n"
        "The chosen cell must currently be empty.\n"
        "\n"
        "Follow standard Gomoku strategy carefully. Your priorities are: \n"
        "1. If white has a move that wins immediately, play it now.\n"
        "2. If black has any immediate winning move next turn, block it now.\n"
        "3. If white can create a strong attack, such as an open four, live three, double threat, or a move that forces black to respond, prefer that move.\n"
        "4. If black is building a dangerous horizontal, vertical, diagonal, or anti-diagonal line of 3 or 4, defend near that line and cut the threat.\n"
        "5. If there is no urgent tactical threat, extend white's connected stones, connect separated white groups, and stay near the existing battle area.\n"
        "6. Avoid isolated moves far away from all current stones unless they directly win or block an immediate loss.\n"
        "\n"
        "When evaluating the board, check both players in all four directions: horizontal, vertical, diagonal, and anti-diagonal.\n"
        "Pay attention to five in a row, open four, closed four, open three, broken three, and double threats.\n"
        "Defense is more important than random development when black is close to winning.\n"
        "If several moves are safe, choose the one that best improves white's chance to win soon.\n"
        "\n"
        "Stones is a list of occupied cells in the form {x,y,v}.\n"
        "Stones:\n") + stones.dump();

        // return std::string(
        // "Gomoku move selection. You are white (2). Board size: 16.\n"
        // "Cell values: 0 empty, 1 black, 2 white.\n"
        // "Return exactly one JSON object only: {\"x\":col,\"y\":row}\n"
        // "No markdown. No explanation. No extra text.\n"
        // "x and y must be integers in [0,15]. The cell must be empty 0.\n"
        // "Stones is a list of occupied cells in the form {x,y,v}.\n"
        // "Choose a reasonable next move near existing stones.\n"
        // "Stones:\n") + stones.dump();
}

std::string GomokuAI::postJson(const json& payload) const
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        throw std::runtime_error("Failed to initialize curl");
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    const std::string requestBody = payload.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "undefined");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

    const CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cout << "curl_easy_perform() failed: " << std::string(curl_easy_strerror(res)) << std::endl; // Debugging output
        throw std::runtime_error("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
    }

    return response;
}

std::pair<int, int> GomokuAI::parseMove(const std::string& responseText) const
{
    const json outer = json::parse(responseText);
    const std::string modelText = outer.at("response").get<std::string>();

    const std::size_t jsonStart = modelText.find('{');
    const std::size_t jsonEnd = modelText.rfind('}');
    if (jsonStart == std::string::npos || jsonEnd == std::string::npos || jsonEnd < jsonStart)
    {
        throw std::runtime_error("Model response does not contain a JSON object: " + modelText);
    }

    const std::string candidate = modelText.substr(jsonStart, jsonEnd - jsonStart + 1);

    try {
        const json move = json::parse(candidate);
        const int x = move.at("x").get<int>();
        const int y = move.at("y").get<int>();
        return {x, y};
    } catch (const std::exception&) {
        std::smatch xMatch;
        std::smatch yMatch;
        const std::regex xPattern("\"x\"\\s*:\\s*(-?\\d+)");
        const std::regex yPattern("\"y\"\\s*:\\s*(-?\\d+)");

        if (!std::regex_search(candidate, xMatch, xPattern) || !std::regex_search(candidate, yMatch, yPattern)) {
            throw std::runtime_error("Failed to parse AI move from response: " + modelText);
        }

        const int x = std::stoi(xMatch[1].str());
        const int y = std::stoi(yMatch[1].str());
        return {x, y};
    }
}

std::pair<int, int> GomokuAI::makeMove(const std::vector<std::vector<int>>& board)
{
    const json payload = {
        {"model", model_},
        {"prompt", buildPrompt(board)},
        {"stream", false}
    };

    const std::string responseText = postJson(payload);
    std::cout << "Response: " << responseText << std::endl; // Debugging output
    return parseMove(responseText);
}
