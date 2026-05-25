#include"../include/AIUtil/MQManager.h"

// -----MQManager--------

MQManager::MQManager(size_t poolSize):poolSize_(poolSize),counter_(0)
{
    for(size_t i = 0; i<poolSize_; ++i)
    {
        auto conn = std::make_shared<MQConn>();
        //Create
        conn->channel = AmqpClient::Channel::ptr_t channel = AmqpClient::Channel::Create(
            "localhost",  // RabbitMQ 服务器主机名或 IP
            5672,         // AMQP 协议默认端口
            "guest",      // 用户名
            "guest",      // 密码
            "/"           // 虚拟主机 (Virtual Host)
        );
        pool_.push_back(conn);
    }
}

void MQManager::publish(const std::string &queue,const std::string &msg)
{
    size_t index = counter_.fetch_add(1) % poolSize_;
    auto & conn = pool_[index];

    std::lock_guard<std::mutex> lock(conn->mtx);
    auto message = AmqpClient::BasicMessage::Create(msg);
    // 发布消息到默认的交换机 ("")，路由键直接指定为队列名
    conn->channel->BasicPublish("",queue,message);
}

//---------RabbitMQThreadPool-----------

void RabbitMQThreadPool::start(){
    for (int i = 0; i < thread_num_; ++i) {
        workers_.emplace_back(&RabbitMQThreadPool::worker, this, i);
    }
}


void RabbitMQThreadPool::shutdown()
{
    stop_=true;
    for(auto & t : workers_){
        if(t.joinable()) t.join();
    }
}

// RabbitMQ 多线程消费者的工作线程（Worker Thread）
void RabbitMQThreadPool::worker(int id){
    try{
        //Each thread has its own independent channel
        auto channel = AmqpClient::Channel::Create(
            rabbitmq_host_,  // RabbitMQ 服务器主机名或 IP
            5672,         // AMQP 协议默认端口
            "guest",      // 用户名
            "guest",      // 密码
            "/"           // 虚拟主机 (Virtual Host)
        );

        channel->DeclareQueue(queue_name_,false,true,false,false);

        // BasicConsume 将该线程注册为队列的消费者。这里传入的第一个 true 代表开启了自动 ACK（自动确认）模式。
        std::string consumer_tag = channel->BasicConsume(queue_name_, "", true, false, false);

        // 公平分发机制（QoS 限流）,处理完当前消息之前不推送新消息给我
        channel->BasicQos(consumer_tag,1);

        while(!stop_){
            AmqpClient::Envelope::ptr_t env;
            // 带超时检测的跳出阻塞状态
            bool ok = channel->BasicConsumeMessage(consumer_tag,env,500);
            if(ok && env){
                std::string msg = env->Message()->Body();
                handler_(msg);
                channel->BasicAck(env);
            }
        }
        channel->BasicCancel(consumer_tag); // Cancel the consumer
    }
    catch(const std::exception & e){
        std::cerr << "Thread " << id << " exception: " << e.what() << std::endl;
    }
}









