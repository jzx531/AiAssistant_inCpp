#include "../../../include/utils/db/DbConnectionPool.h"
#include "../../../include/utils/db/DbException.h"
#include <muduo/base/Logging.h>

namespace http 
{
namespace db 
{

void DbConnectionPool::init(
        const std::string &host, 
        const std::string &user, 
        const std::string &password, 
        const std::string &database, 
        size_t poolSize
    )
{
    //连接池会被多个线程访问,所以操作需要加锁
    std::unique_lock<std::mutex> lock(mutex_);
    //如果连接池已经初始化过,则不再初始化
    if (initialized_)
    {
        return;
    }

    host_ = host;
    user_ = user;
    password_ = password;
    database_ = database;

    //创建连接
    for (size_t i = 0; i < poolSize; ++i)
    {
        connections_.push(createConnection());
    }

    initialized_ = true;
    LOG_INFO << "Database connection pool initialized with " << poolSize << " connections";
}

DbConnectionPool::DbConnectionPool()
{
    checkThread_ = std::thread(&DbConnectionPool::checkConnections, this);
    checkThread_.detach();
}

DbConnectionPool::~DbConnectionPool() 
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) 
    {
        connections_.pop();
    }
    LOG_INFO << "Database connection pool destroyed";
}

//修改获取连接的函数
std::shared_ptr<DbConnection> DbConnectionPool::getConnection()
{
    std::shared_ptr<DbConnection> conn;
    {
        std::unique_lock<std::mutex> lock(mutex_);

        while(connections_.empty())
        {
            if(!initialized_)
            {
                throw DbException("Database connection pool not initialized");
            }
            LOG_INFO << "Waiting for database connection...";
            cv_.wait(lock);
        }
        conn = connections_.front();
        connections_.pop();
    }//释放锁
    try
    {
        //在锁外检查连接
        if(!conn->ping())
        {
            LOG_WARN << "Connection lost, attempting to reconnect...";
            conn = createConnection();
        }
        return std::shared_ptr<DbConnection>(conn.get(),
            [this,conn](DbConnection* ){
                std::lock_guard<std::mutex> lock(mutex_);
                connections_.push(conn);
                cv_.notify_one();
            }
        );
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR << "Failed to get connection: " << e.what();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            connections_.push(conn);
            cv_.notify_one();
        }
        throw;
    }
}

std::shared_ptr<DbConnection> DbConnectionPool::createConnection()
{
    return std::make_shared<DbConnection>(host_, user_, password_, database_);
}
    
//修改检查连接的函数
void DbConnectionPool::checkConnections()
{
    while(true)
    {
        try
        {
            std::vector<std::shared_ptr<DbConnection>> connsToCheck;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if(connections_.empty())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }

                auto tmp = connections_;
                while(!tmp.empty())
                {
                    connsToCheck.push_back(tmp.front());
                    tmp.pop();
                }
                
            }

            //在锁外检查链接
            for(auto& conn : connsToCheck)
            {
                if(!conn->ping())
                {
                    try{
                        conn->reconnect();
                    }
                    catch(const std::exception& e)
                    {
                        LOG_ERROR << "Failed to reconnect: " << e.what();
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
        catch(const std::exception& e)
        {
            LOG_ERROR << "Error in check thread: " << e.what();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        
    }
}

}
}