#include "../include/handlers/GomokuEntryHandler.h"

void GomokuEntryHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    std::string reqFile;
    // reqFile.append("C:/Users/Jiang ZiXian/Desktop/github/AiAssistant_inCpp/AIApps/GomokuServer/resource/gomoku.html");
    reqFile.append("../AIApps/GomokuServer/resource/gomoku.html");
    FileUtil fileOperater(reqFile);
    if (!fileOperater.isValid())
    {
        LOG_WARN << reqFile << " not exist";
        fileOperater.resetDefaultFile(); // 404 NOT FOUND
    }

    std::vector<char> buffer(fileOperater.size());
    fileOperater.readFile(buffer); 
    std::string bufStr = std::string(buffer.data(), buffer.size());

    resp->setStatusLine(req.getVersion(), http::HttpResponse::Ok200, "OK");
    resp->setCloseConnection(false);
    resp->setContentType("text/html");
    resp->setContentLength(bufStr.size());
    resp->setBody(bufStr);
}

