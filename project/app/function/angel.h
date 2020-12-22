#pragma once

#include "../core/component.h"

namespace beryl
{
    class Angel final : public Component
    {
    public:
        inline static const std::string name = "Angel";
        inline static const std::string_view command = "/Angel";

    protected:
        void do_on_event(mirai::Session& sess, const mirai::Event& event) override;
    };
}
