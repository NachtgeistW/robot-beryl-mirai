#pragma once

#include "../core/component.h"

namespace beryl
{
    class Repeat final : public Component
    {
    public:
        void do_on_event(mirai::Session& sess, const mirai::Event& event) override;
    };
}
