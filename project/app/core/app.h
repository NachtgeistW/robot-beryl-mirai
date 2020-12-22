#pragma once

#include <fstream>
#include <sstream>
#include <filesystem>

#include "component.h"
#include "config.h"
#include "singleton.h"

namespace beryl
{
    class App final : public singleton<App>
    {
    private:
        mirai::Session session_;
        std::vector<PolymorphicComponentWrapper> components_;
        Config config_;
    public:
        App();
        ~App() noexcept;

        template<typename T, typename... Args>
        void add_component(Args&&... args)
        {
            components_.emplace_back(PolymorphicComponentWrapper::create<T>(args...));
        }

        void dispatch_event(const mirai::Event& event)
        {
            for (auto& comp : components_)
                comp.on_event(session_, event);
        }

        void load_data();


    };
}
