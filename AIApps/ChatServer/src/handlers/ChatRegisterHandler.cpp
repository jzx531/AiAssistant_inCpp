#include "../include/handlers/ChatRegisterHandler.h"


void ChatRegisterHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    json parsed = json::parse(req.getBody());
    std::string username = parsed["username"];
    std::string password = parsed["password"];

    int userId = insertUser(username, password);
    if (userId != -1)
    {
        json successResp;
        successResp["status"] = "success";
        successResp["message"] = "register successful";
        successResp["userId"] = userId;
        std::string successBody = successResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::Ok200, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
    }
    else
    {
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = "username already exists";
        std::string failureBody = failureResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::Conflict409, "Conflict");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}

int ChatRegisterHandler::insertUser(const std::string &username, const std::string &password)
{
    if(!isUserExist(username))
    {
        std::string sql = "INSERT INTO users (username, password) VALUES (?, ?)";
        mysqlUtil_.executeUpdate(sql, username, password);
        std::string sql2 = "SELECT id FROM users WHERE username = ?";
        auto res = mysqlUtil_.executeQuery(sql2, username);
        if (res->next())
        {
            return res->getInt("id");
        }
    }
    return -1;
}

bool ChatRegisterHandler::isUserExist(const std::string &username)
{
    std::string sql = "SELECT id FROM users WHERE username = ?";
    auto res = mysqlUtil_.executeQuery(sql, username);
    if(res->next())
    {
        return true;
    }
    return false;
}

