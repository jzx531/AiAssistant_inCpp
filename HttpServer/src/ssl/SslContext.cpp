#include "../../include/ssl/SslContext.h"
#include <muduo/base/Logging.h>
#include <openssl/err.h>

namespace ssl
{

SslContext::SslContext(const SslConfig& config):ctx_(nullptr)
, config_(config)
{

}
}