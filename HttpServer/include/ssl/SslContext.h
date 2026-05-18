#ifndef SSLCONTEXT_H
#define SSLCONTEXT_H

#include "SslConfig.h"
#include <openssl/ssl.h>
#include <memory>
#include <muduo/base/noncopyable.h>

namespace ssl
{
    
class SslContext : muduo::noncopyable
{
public:
    explicit SslContext(const SslConfig &config);
    ~SslContext();

    bool initialize();
    SSL_CTX *get() const { return m_sslContext.get(); }

private:
    bool loadCertificates();
    bool setupProtocol();
    void setupSessionCache();
    static void handleSslError(const char *msg);
};

}

#endif // SSLCONTEXT_H
