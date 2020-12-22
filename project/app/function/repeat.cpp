#include "repeat.h"

namespace beryl
{
    void Repeat::do_on_event(mirai::Session& sess, const mirai::Event& event)
    {
        using namespace mirai::literals; // 拉入 _uid _gid 等字面量运算符
        event.dispatch([&](const mirai::GroupMessage& e) // 当收到的消息是群消息时分发到此函数
            {
                if (e.sender.group.id == 279023542)
                {
                    sess.send_message(e.sender.group.id, e.message.content); // 向消息源的群发送一模一样的消息（复读）
                    std::cout << "Repeated.";
                }
            });
    }
}
