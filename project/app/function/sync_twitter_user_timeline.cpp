// TwitterTimeline++.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define NOMINMAX

#include <chrono>
#include <iostream>
#include <stack>
#include <string>
#include <utility>

#include <curl/curl.h>

#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>

#include <nlohmann/json.hpp>

#include <sodium.h>

#include <mirai/core/events.h>
#include <mirai/core/session.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <asio.hpp>

#include "sync_twitter_user_timeline.h"

#include <fstream>

using std::string;
using std::vector;
using json = nlohmann::json;

namespace beryl
{
    /// <summary>
    /// 获取 Twitter OAuth 的 parameter nonce
    /// </summary>
    /// <returns>nonce</returns>
    string SyncTwitterTimeline::get_n_once()
    {
        // myString will be a string of 32 random bytes
        // Use sodium to generate
        char random_buf[32];
        randombytes_buf(random_buf, 32);

        string str;

        CryptoPP::StringSource ss(
            reinterpret_cast<const byte*>(random_buf),
            32,
            true,
            new CryptoPP::Base64Encoder(new CryptoPP::StringSink(str), false)
        );

        // remove +, /, = in string
        str.erase(remove(str.begin(), str.end(), '+'), str.end());
        str.erase(remove(str.begin(), str.end(), '/'), str.end());
        str.erase(remove(str.begin(), str.end(), '='), str.end());

        return str;
    }

    /// <summary>
    /// 获取 Twitter OAuth 的 parameter timestamp
    /// </summary>
    /// <returns>timestamp</returns>
    string SyncTwitterTimeline::get_timestamp()
    {
        const auto ms = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().
            time_since_epoch());
        return std::to_string(ms.count());
    }

    string SyncTwitterTimeline::hmac_sha1_sign(const string& key, const string& plain)
    {
        const auto* key_buf = reinterpret_cast<const byte*>(key.c_str());
        string      base64_encoded;

        try
        {
            CryptoPP::HMAC<CryptoPP::SHA1> hmac(key_buf, key.size());

            CryptoPP::StringSource ss(
                plain, true,
                new CryptoPP::HashFilter(
                    hmac,
                    new CryptoPP::Base64Encoder(
                        new CryptoPP::StringSink(
                            base64_encoded),
                        false)
                )
            );
        }
        catch (const CryptoPP::Exception& e)
        {
            std::cerr << e.what() << std::endl;
        }

        return base64_encoded;
    }

    /// <summary>
    /// 将准备好的 OAuth 参数拼接为字符串
    /// </summary>
    /// <param name="screen_name">Twitter 用户名</param>
    /// <returns></returns>
    string SyncTwitterTimeline::collecting_oauth_parameters(const string& screen_name)
    {
        string auth;
        oauth_nonce_ = get_n_once();
        oauth_timestamp_ = get_timestamp();
        auth = auth + "oauth_consumer_key=" + config_.oauth_consumer_key_;
        auth = auth + "&oauth_nonce=" + oauth_nonce_;
        auth = auth + "&oauth_signature_method=" + oauth_signature_method_;
        auth = auth + "&oauth_timestamp=" + oauth_timestamp_;
        auth = auth + "&oauth_token=" + config_.oauth_token_;
        auth = auth + "&oauth_version=" + oauth_version_;
        auth = auth + "&screen_name=" + screen_name;

        return auth;
    }

    /// <summary>
    /// 创建 signature base string
    /// </summary>
    /// <param name="origin_oauth_str">原始的 OAuth 字符串</param>
    /// <returns>创建好的 signature base string</returns>
    string SyncTwitterTimeline::create_signature(const string& origin_oauth_str)
    {
        string oauth_signature;
        // https://curl.haxx.se/libcurl/c/curl_easy_escape.html
        // prepare method that use to convert 
        auto* curl = curl_easy_init();

        // prepare percent encoding string
        const string pe_url = { curl_easy_escape(curl, config_.url_.c_str(), static_cast<int>(config_.url_.length())) };
        const string pe_oauth = { curl_easy_escape(curl, origin_oauth_str.c_str(), static_cast<int>(origin_oauth_str.length())) };
        const string pe_oauth_consumer_secret = {
            curl_easy_escape(curl, config_.oauth_consumer_secret_.c_str(), static_cast<int>(config_.oauth_consumer_secret_.length()))
        };
        const string pe_oauth_token_secret = {
            curl_easy_escape(curl, config_.oauth_token_secret_.c_str(), static_cast<int>(config_.oauth_token_secret_.length()))
        };

        // creating a signature
        const auto base = string("GET") + "&" + pe_url + "&" + pe_oauth;
        const auto key = pe_oauth_consumer_secret + "&" + pe_oauth_token_secret;
        oauth_signature = hmac_sha1_sign(key, base);
        oauth_signature = curl_easy_escape(curl, oauth_signature.c_str(), static_cast<int>(oauth_signature.length()));

        return oauth_signature;
    }

    /// <summary>
    /// 将准备好的 OAuth 字符串转换为 http request header
    /// </summary>
    /// <returns></returns>
    string SyncTwitterTimeline::prepare_oauth_header(const string& screen_name)
    {
        const auto oauth_signature =
            create_signature(collecting_oauth_parameters(screen_name));
        // joint header
        string auth = "OAuth ";
        auth = auth + "oauth_consumer_key=\"" + config_.oauth_consumer_key_ + "\", ";
        auth = auth + "oauth_nonce=\"" + oauth_nonce_ + "\", ";
        auth = auth + "oauth_signature_method=\"" + oauth_signature_method_ + "\", ";
        auth = auth + "oauth_timestamp=\"" + oauth_timestamp_ + "\", ";
        auth = auth + "oauth_token=\"" + config_.oauth_token_ + "\", ";
        auth = auth + "oauth_version=\"" + oauth_version_ + "\", ";
        auth = auth + "oauth_signature=\"" + oauth_signature + "\"";

        return auth;

        // https://stackoverflow.com/questions/33523286/converting-from-c-string-to-unsigned-char-and-back
        // https://stackoverflow.com/questions/1673445/how-to-convert-unsigned-char-to-stdstring-in-c
    }

    /// <summary>
    /// 向 Twitter 发送带 OAuth 的请求，并返回 screen_user 的最近 20 条推文
    /// </summary>
    /// <returns></returns>
    std::pair<long, string> SyncTwitterTimeline::oauth_get(const string& screen_name)
    {
        auto* const curl = curl_easy_init();

        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: " + prepare_oauth_header(screen_name)).c_str());
        headers = curl_slist_append(headers, "Connection: close");

        long   response_code;
        string response;

        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, (config_.url_ + "?screen_name=" + screen_name).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
#ifdef DEBUG
            curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:1080");
#endif
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        return std::make_pair(response_code, response);
    }

    /// <summary>
    /// 将获取到的推文加入待发送栈
    /// </summary>
    /// <param name="response">Twitter 返回的</param>
    /// <returns></returns>
    std::stack<Tweet> SyncTwitterTimeline::prepare_sending_stack(const std::pair<long, string>& response)
    {
        auto js = json::parse(response.second);
        std::stack<Tweet> tweet_stk;
        for (auto& j : js)
        {
            if (j["id_str"] > config_.latest_tweet_id_str_)
            {
                if (j.contains("extended_entities"))
                {
                    vector<string> media;
                    for (auto& i : j["extended_entities"]["media"])
                        media.push_back(i["media_url"]);
                    tweet_stk.push(Tweet(j["created_at"], j["id"], j["id_str"], j["text"], j["truncated"], media));
                }
                else
                {
                    tweet_stk.push(Tweet(j["created_at"], j["id"], j["id_str"], j["text"], j["truncated"]));
                }
            }
        }
        return tweet_stk;
    }

    void SyncTwitterTimeline::send_tweets_to_group(const mirai::gid_t group_id, mirai::Session& sess, const string& screen_name)
    {
        const auto response = oauth_get(screen_name);
        auto tweet_stk = prepare_sending_stack(response);

        using namespace mirai::literals; // 拉入 _uid _gid 等字面量运算符
#ifdef DEBUG
        system("set http_proxy=127.0.0.1:1080");
        system("set https_proxy=127.0.0.1:1080");
#endif
        while (!tweet_stk.empty())
        {
            if (tweet_stk.size() == 1)
            {
                config_.latest_tweet_id_str_ = tweet_stk.top().get_id_str();
            }
            auto plain = "https://twitter.com/" + screen_name + "/status/" + tweet_stk.top().get_id_str() + '\n' + tweet_stk.top().get_created_at() + '\n' + tweet_stk.top().get_text();
            if (!tweet_stk.top().get_media().empty())
            {
                sess.send_message(group_id, mirai::Message{
                    mirai::msg::Plain{plain},
                    mirai::msg::Image{{}, tweet_stk.top().get_media()[0], {}}
                    });
            }
            else
                sess.send_message(group_id, plain);

            tweet_stk.pop();
        }
    }

    SyncTwitterTimeline::SyncTwitterTimeline()
    {
        {
            const std::ifstream fs(R"(config_twitter.json)");
            std::stringstream ss;
            ss << fs.rdbuf();
            const auto j = json::parse(ss.str());
            config_.from_json(j);
        }
    }

    SyncTwitterTimeline::~SyncTwitterTimeline()
    {
        try
        {
            json j;
            std::ofstream ofs("config_twitter.json");
            {
                config_.to_json(j);
                ofs << j.dump(2);
            }
        }
        catch (...) {}
    }

    void TwitterConfig::from_json(const nlohmann::json& j)
    {
        j["oauth_consumer_key"].get_to(oauth_consumer_key_);
        j["oauth_consumer_secret"].get_to(oauth_consumer_secret_);
        j["oauth_token"].get_to(oauth_token_);
        j["oauth_token_secret"].get_to(oauth_token_secret_);
        j["url"].get_to(url_);
        j["latest_tweet_id_str"].get_to(latest_tweet_id_str_);
    }

    void TwitterConfig::to_json(nlohmann::json& j)
    {
        j["oauth_consumer_key"] = oauth_consumer_key_;
        j["oauth_consumer_secret"] = oauth_consumer_secret_;
        j["oauth_token"] = oauth_token_;
        j["oauth_token_secret"] = oauth_token_secret_;
        j["url"] = url_;
        j["latest_tweet_id_str"] = latest_tweet_id_str_;
    }

    void SyncTwitterTimeline::do_on_event(mirai::Session& sess, const mirai::Event&)
    {
        using namespace std::literals;
        try
        {
#ifdef DEBUG
            static auto _ = Timer(
                io_,
                [&]() { send_tweets_to_group(mirai::gid_t(279023542), sess, "Nightwheel_C"); },
                1min
            );
#else
            static auto _ = Timer(
                io_,
                [&]() { send_tweets_to_group(mirai::gid_t(1051608425), sess, "pj_sekai"); },
                1min
            );
#endif
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        catch (...)
        {

        }
    }

    size_t write_string(void* ptr, const size_t size, const size_t nmemb, std::string* data)
    {
        data->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }
}
