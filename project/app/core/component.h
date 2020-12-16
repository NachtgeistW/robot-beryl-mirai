#pragma once
#include <vector>
#include <mirai/mirai.h>

namespace beryl
{
    /// <summary>
    /// 组件基类
    /// </summary>
    class Component
    {
        friend class PolymorphicComponentWrapper;
        std::vector<mirai::uid_t> user_blacklist_;
        std::vector<mirai::gid_t> group_whitelist_;

    protected:
        virtual void do_on_event(mirai::Session& sess, const mirai::Event& event) = 0;

    public:
        struct TextMessage final
        {
            mirai::uid_t user;
            mirai::gid_t group;
            mirai::msgid_t msg_id;
            std::string_view text;
        };

        [[nodiscard]] auto& group_whitelist() { return group_whitelist_; }
        [[nodiscard]] auto& user_blacklist() { return user_blacklist_; }
        [[nodiscard]] auto& group_whitelist() const { return group_whitelist_; }
        [[nodiscard]] auto& user_blacklist() const { return user_blacklist_; }

        [[nodiscard]] bool passed(mirai::gid_t) const;
        [[nodiscard]] bool passed(mirai::uid_t) const;

        std::optional<TextMessage> event_trigger(mirai::uid_t bot_id, const mirai::Event& event);
        virtual ~Component() = default;
    };

    /// <summary>
    /// 多态组件封装器
    /// </summary>
    class PolymorphicComponentWrapper final
    {
        std::unique_ptr<Component> ptr_;

    public:
        explicit PolymorphicComponentWrapper(std::unique_ptr<Component>&& ptr) noexcept : 
            ptr_(std::move(ptr)) { }

        template <typename T, typename... Args>
        static PolymorphicComponentWrapper create(Args&&... args) noexcept
        {
            return PolymorphicComponentWrapper(std::unique_ptr<Component>(new T(args...)));
        }

        void on_event(mirai::Session& sess, const mirai::Event& event);
    };
}
