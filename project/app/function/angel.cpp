#include "angel.h"

namespace beryl
{
    void Angel::do_on_event(mirai::Session& sess, const mirai::Event& event)
    {
        static std::mt19937 gen(std::random_device{}());
        static const std::bernoulli_distribution d(0.05);
        event.dispatch([&](const mirai::GroupMessage& e)
            {
                if (!passed(e.sender.group.id))
                {
                    std::cout << "Unavailable in this group\n";
                    return;
                }
                if (d(gen))
                {
                    sess.send_message(e.sender.group.id, "夜轮大大简直是天使！啊啊啊啊激动到跑圈！！！");
                    std::cout << "Running in circle.\n";
                }
            });
    }
}
