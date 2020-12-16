#pragma once
#include "../core/component.h"
#include "../function/angel.h"
namespace msg = mirai::msg;
namespace ut = mirai::utils;

namespace beryl
{
    class MasterCommands final : public Component
    {
    public:
        inline static const std::string_view name = "Master Commands";
    protected:
        void do_on_event(mirai::Session& sess, const mirai::Event& event) override;
    private:
        mirai::uid_t master_id_ = mirai::uid_t(562231326);

        [[nodiscard]] bool check_permission(const mirai::Member& member) const
        {
            return member.id == master_id_;
        }
        void activate_component_in_group(mirai::Session sess, const mirai::Event& event)
        {
            event.dispatch([&](const mirai::GroupMessage& e)
                {
                    if (!check_permission(e.sender)) return;
                    //if (e.message.content == "/activate" + Angel::name)
                    //{
                    //    Angel::group_whitelist_.emplace_back(e.sender.group.id);
                    //}
                });

        }
    };
}
