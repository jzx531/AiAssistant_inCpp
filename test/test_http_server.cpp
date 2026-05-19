#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <muduo/base/Timestamp.h>
#include <muduo/net/Buffer.h>

#include "../HttpServer/include/http/HttpContext.h"
#include "../HttpServer/include/http/HttpResponse.h"
#include "../HttpServer/include/router/Router.h"
#include "../HttpServer/include/router/RouterHandler.h"
#include "../HttpServer/include/middleware/MiddlewareChain.h"

namespace
{

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

class StaticHandler : public http::router::RouterHandler
{
public:
    void handle(const http::HttpRequest &req, http::HttpResponse *resp) override
    {
        resp->setStatusCode(http::HttpResponse::Ok200);
        resp->setStatusMessage("OK");
        resp->setBody("static:" + req.path());
    }
};

class TraceMiddleware : public http::middleware::Middleware
{
public:
    void before(http::HttpRequest &request) override
    {
        const char *body = "before-processed";
        request.setBody(body, body + 16);
    }

    void after(http::HttpResponse &response) override
    {
        response.addHeader("X-Test-After", "done");
    }
};

void testHttpContextParsesGetRequest()
{
    muduo::net::Buffer buffer;
    buffer.append("GET /chat?id=42&name=bob HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");

    http::HttpContext context;
    const bool ok = context.parseRequest(&buffer, muduo::Timestamp::now());

    require(ok, "HttpContext should parse a valid GET request");
    require(context.gotAll(), "HttpContext should finish parsing GET request");
    require(context.request().method() == http::HttpRequest::Get, "Method should be GET");
    require(context.request().path() == "/chat", "Path should be /chat");
    require(context.request().getQueryParameter("id") == "42", "Query parameter id should be 42");
    require(context.request().getQueryParameter("name") == "bob", "Query parameter name should be bob");
    require(context.request().getHeader("Host") == "localhost", "Host header should be parsed");
    require(context.request().getVersion() == "HTTP/1.1", "Version should be HTTP/1.1");
}

void testHttpContextParsesPostBody()
{
    muduo::net::Buffer buffer;
    buffer.append("POST /login HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11\r\n\r\nhello=world");

    http::HttpContext context;
    const bool ok = context.parseRequest(&buffer, muduo::Timestamp::now());

    require(ok, "HttpContext should parse a valid POST request");
    require(context.gotAll(), "HttpContext should finish parsing POST request");
    require(context.request().method() == http::HttpRequest::Post, "Method should be POST");
    require(context.request().getBody() == "hello=world", "Body should match Content-Length");
    require(context.request().contentLength() == 11, "Content-Length should be stored");
}

void testHttpResponseSerializesMessage()
{
    http::HttpResponse response(false);
    response.setVersion("HTTP/1.1");
    response.setStatusCode(http::HttpResponse::Ok200);
    response.setStatusMessage("OK");
    response.addHeader("Content-Type", "text/plain");
    response.setBody("pong");

    muduo::net::Buffer output;
    response.appendToBuffer(&output);
    const std::string raw = output.toStringPiece().as_string();

    require(raw.find("HTTP/1.1 200 OK\r\n") == 0, "Status line should be serialized");
    require(raw.find("Connection: keep-alive\r\n") != std::string::npos, "Keep-alive header should exist");
    require(raw.find("Content-Type: text/plain\r\n") != std::string::npos, "Content-Type header should exist");
    require(raw.size() >= 4 && raw.substr(raw.size() - 4) == "pong", "Body should be appended");
}

void testRouterSupportsStaticAndRegexRoutes()
{
    http::router::Router router;
    router.registerCallback(http::HttpRequest::Get, "/ping",
        [](const http::HttpRequest &, http::HttpResponse *resp)
        {
            resp->setStatusCode(http::HttpResponse::Ok200);
            resp->setStatusMessage("OK");
            resp->setBody("pong");
        });

    router.registerHandler(http::HttpRequest::Get, "/users", std::make_shared<StaticHandler>());

    router.addRegexCallback(http::HttpRequest::Get, "/users/:id",
        [](const http::HttpRequest &req, http::HttpResponse *resp)
        {
            resp->setStatusCode(http::HttpResponse::Ok200);
            resp->setStatusMessage("OK");
            resp->setBody(req.getPathParameter("param1"));
        });

    http::HttpRequest pingRequest;
    pingRequest.setMethod("GET", "GET" + 3);
    pingRequest.setPath("/ping", "/ping" + 5);

    http::HttpResponse pingResponse;
    require(router.route(pingRequest, &pingResponse), "Static callback route should match");

    muduo::net::Buffer pingOutput;
    pingResponse.setVersion("HTTP/1.1");
    pingResponse.appendToBuffer(&pingOutput);
    require(pingOutput.toStringPiece().as_string().find("pong") != std::string::npos, "Static callback should write pong");

    http::HttpRequest regexRequest;
    regexRequest.setMethod("GET", "GET" + 3);
    regexRequest.setPath("/users/100", "/users/100" + 10);

    http::HttpResponse regexResponse;
    require(router.route(regexRequest, &regexResponse), "Regex callback route should match");

    muduo::net::Buffer regexOutput;
    regexResponse.setVersion("HTTP/1.1");
    regexResponse.appendToBuffer(&regexOutput);
    require(regexOutput.toStringPiece().as_string().find("100") != std::string::npos, "Regex route should expose path parameter");
}

void testMiddlewareChainRunsBeforeAndAfter()
{
    http::middleware::MiddlewareChain chain;
    chain.addMiddleware(std::make_shared<TraceMiddleware>());

    http::HttpRequest request;
    request.setMethod("GET", "GET" + 3);
    request.setPath("/middleware", "/middleware" + 11);
    chain.processBefore(request);

    require(request.getBody() == "before-processed", "Before middleware should mutate request state");

    http::HttpResponse response;
    chain.processAfter(response);

    response.setVersion("HTTP/1.1");
    response.setStatusCode(http::HttpResponse::Ok200);
    response.setStatusMessage("OK");
    muduo::net::Buffer output;
    response.appendToBuffer(&output);
    require(output.toStringPiece().as_string().find("X-Test-After: done\r\n") != std::string::npos,
            "After middleware should add response header");
}

} // namespace

int main()
{
    try
    {
        testHttpContextParsesGetRequest();
        testHttpContextParsesPostBody();
        testHttpResponseSerializesMessage();
        testRouterSupportsStaticAndRegexRoutes();
        testMiddlewareChainRunsBeforeAndAfter();
        std::cout << "httpServer tests passed" << std::endl;
        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "httpServer tests failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
