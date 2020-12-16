#pragma once
#include <mirai/mirai.h>

namespace beryl
{
	using nlohmann::json;
	struct Config final
	{
		std::string auth_key;
		mirai::uid_t bot_id;
		mirai::uid_t dev_id;

		struct ComponentInfo final
		{
			std::string help_string;
			std::string description;
			std::vector<mirai::gid_t> group_whitelist;
			std::vector<mirai::uid_t> user_blacklist;
		};
	public:
		void from_json(const json& j)
		{
			j["auth_key"].get_to(auth_key);
			j["bot_id"].get_to(bot_id);
		}
		void to_json(json& j, const Config& config)
		{
			j["auth_key"] = config.auth_key;
			j["bot_id"] = config.bot_id;
		}
	};
}