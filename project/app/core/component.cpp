#include "component.h"

namespace beryl
{
    /// <summary>
    /// 查询群是否在白名单里
    /// </summary>
    /// <param name="group_id">群 ID</param>
    /// <returns>如果在，则返回 true</returns>
    bool Component::passed(const mirai::gid_t group_id) const
    {
        return std::find(group_whitelist_.begin(), group_whitelist_.end(), group_id) != 
            group_whitelist_.end();
    }

    /// <summary>
    /// 查询消息发送者是否在黑名单里
    /// </summary>
    /// <param name="user_id">用户 ID</param>
    /// <returns>如果不在，则返回 true</returns>
    bool Component::passed(const mirai::uid_t user_id) const
    {
        return std::find(user_blacklist_.begin(), user_blacklist_.end(), user_id) ==
            user_blacklist_.end();
    }

    std::optional<Component::TextMessage> Component::event_trigger(const mirai::uid_t bot_id,
        const mirai::Event& event)
    {
        std::optional<TextMessage> opt;
        event.dispatch([&](const mirai::FriendMessage& e)
            {
                if (!passed(e.sender.id)) return;
                if (e.message.quote) return;
                const auto segments = e.message.content.match_types<mirai::msg::Plain>();
                if (!segments) return;
                const auto& [plain] = *segments;
                opt = {
                    e.sender.id, {}, e.message.source.id,
                    mirai::utils::trim_whitespace(plain.view())
                };
            });
        event.dispatch([&, bot_id](const mirai::GroupMessage& e)
            {
                if (!passed(e.sender.group.id) || !passed(e.sender.id)) return;
                if (e.message.quote) return;
                const auto segments = e.message.content.match_types<mirai::msg::At, mirai::msg::Plain>();
                if (!segments) return; // Not "At + Plain"
                const auto& [at, plain] = *segments;
                if (at.target != bot_id) return; // Not at bot
                opt = {
                    e.sender.id, e.sender.group.id, e.message.source.id,
                    mirai::utils::trim_whitespace(plain.view())
                };
            });
        return opt;
    }

    void PolymorphicComponentWrapper::on_event(mirai::Session& sess, const mirai::Event& event)
    {
        try
        {
            ptr_->do_on_event(sess, event);
        }
        catch (const std::exception& e)
        {
            std::cout << e.what();
        }
        catch (...)
        {
            
        }
    }
}
