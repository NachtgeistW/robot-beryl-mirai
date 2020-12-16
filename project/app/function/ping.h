#pragma once
#include "../core/component.h"

namespace beryl
{
    class Ping final : public Component
    {
    public:
        inline static const std::string_view name = "Ping";
        inline static const std::string_view command = "/ping";

    protected:
        void do_on_event(mirai::Session& sess, const mirai::Event& event) override;
    };
}
