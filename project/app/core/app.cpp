#include "app.h"

namespace beryl
{
    App::App()
    {
        {
            std::ifstream fs(R"(config.json)");
            std::stringstream ss;
            ss << fs.rdbuf();
            const auto j = json::parse(ss.str());
            config_.from_json(j);
        }

        session_ = mirai::Session(config_.auth_key, config_.bot_id);
        session_.config({}, true);
        session_.subscribe_all_events(
            [&](const mirai::Event& event) { dispatch_event(event); },
            mirai::error_logger,
            mirai::ExecutionPolicy::thread_pool);

    }

    App::~App() noexcept
    {
        try
        {

        }
        catch (...) {}
    }

    /// <summary>
    /// 加载component的数据
    /// </summary>
    void App::load_data()
    {
        std::ifstream fs("config.json");
        std::stringstream ss;
        ss << fs.rdbuf();
        const auto config = json::parse(ss.str());
    }

}
