#include "ping.h"

void beryl::Ping::do_on_event(mirai::Session& sess, const mirai::Event& event)
{
    event.dispatch([&](const mirai::GroupMessage& e)
        {
            if (e.message.content == command)
            {
                sess.send_message(e.sender.group.id, "%pong!");
            }
        });
}
