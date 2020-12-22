#pragma once
#include <string>
#include <stack>

#include "../core/component.h"

using std::string;
using std::vector;
namespace beryl
{
    class Tweet;
    class TwitterConfig;
    class SyncTwitterTimeline;

    class Tweet
    {
        string created_at_;
        unsigned long id_ = 0;
        string id_str_;
        string text_;
        bool truncated_ = false;
        vector<string> media_;
    public:
        Tweet() = default;
        Tweet(string created_at, const unsigned long id, string id_str, string text, const bool truncated) :
            created_at_(std::move(created_at)), id_(id), id_str_(std::move(id_str)), text_(std::move(text)), truncated_(truncated) {}
        Tweet(string created_at, const unsigned long id, string id_str, string text, const bool truncated, const vector<string>& media) :
            created_at_(std::move(created_at)), id_(id), id_str_(std::move(id_str)), text_(std::move(text)), truncated_(truncated), media_(media) {}

        [[nodiscard]] string get_created_at() const { return created_at_; }
        [[nodiscard]] unsigned long get_id() const { return id_; }
        [[nodiscard]] string get_id_str() const { return id_str_; }
        [[nodiscard]] string get_text() const { return text_; }
        [[nodiscard]] vector<string> get_media() const { return media_; }
    };

    class TwitterConfig final
    {
        friend class SyncTwitterTimeline;
        string oauth_consumer_key_, oauth_consumer_secret_, oauth_token_,
            oauth_token_secret_, url_, latest_tweet_id_str_;
        void from_json(const nlohmann::json& j);
        void to_json(nlohmann::json& j);
    };

    class SyncTwitterTimeline final : public Component
    {
        friend class TwitterConfig;

        TwitterConfig config_;
        asio::io_context io_;

        //一个base64 digit是6bit，一个char是8bit，32/3*4是你要的数组长度
        const size_t nonce_len_ = 45;
        string oauth_nonce_, oauth_timestamp_;
        const string oauth_version_ = "1.0", oauth_signature_method_ = "HMAC-SHA1";

        string get_n_once();
        string get_timestamp();
        string hmac_sha1_sign(const string& key, const string& plain);
        string collecting_oauth_parameters(const string& screen_name);
        string create_signature(const string& origin_oauth_str);
        string prepare_oauth_header(const string& screen_name);
        std::pair<long, string> oauth_get(const string& screen_name);
        std::stack<Tweet> prepare_sending_stack(const std::pair<long, string>& response);
        void send_tweets_to_group(mirai::gid_t group_id, mirai::Session& sess, const string& screen_name);
    public:
        SyncTwitterTimeline();
        ~SyncTwitterTimeline() override;
        void do_on_event(mirai::Session& sess, const mirai::Event&) override;
    };

    template <typename T>
    class Timer
    {
    private:
        using duration = asio::steady_timer::duration;

        asio::io_context& io_;
        duration duration_;
        asio::steady_timer t_{ io_ };
        T func_;

        void do_work()
        {
            func_();

            t_.expires_after(duration_);
            invoke();
        }

        void invoke() { t_.async_wait([&](const asio::error_code&) { do_work(); }); }

    public:
        Timer() = default;
        Timer(asio::io_context& io, T func, const duration d) : io_(io), duration_(d), func_(std::move(func))
        {
            invoke();
            io_.run();
        }
        ~Timer() = default;
        [[nodiscard]]auto& get_duration() const { return duration_; }
        void set_duration(const duration d) { duration_ = d; }
    };

    template <typename T>
    Timer(asio::io_context&, T, asio::steady_timer::duration)->Timer<T>;

    size_t write_string(void* ptr, size_t size, size_t nmemb, std::string* data);
}
