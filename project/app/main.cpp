#include "core/app.h"
#include "function/ping.h"
#include "function/sync_twitter_user_timeline.h"

//#include <boost/asio.hpp>

int main()
{
    using namespace std;
    using namespace std::literals;
    using namespace boost;

    try
    {
        beryl::App app;
        //app.add_component<beryl::Repeat>();
        //app.add_component<beryl::Angel>();
        app.add_component<beryl::SyncTwitterTimeline>();
        app.add_component<beryl::Ping>();
        std::cin.get();
        std::cout << "Stopping...\n";
    }
    catch (...)
    {
        mirai::error_logger();
    }
    return 0;
}
