#include "../header.hpp"
#include "g26_select.h"
#include <regex>
#include <map>
#include <vector>
#include <set>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

auto select_res = std::map<unsigned int, std::vector<mirai::uid_t> >();
auto select_res_name = std::map<unsigned int, std::vector<string> >();
auto allow_size = std::map<size_t, size_t>();
auto selected_member = std::set<mirai::uid_t>();
bool is_allowed_to_select = false;
int default_size = 5;
unsigned limited = 7;
json j;

void g26_select(mirai::Session& session, const mirai::Event& event)
{
	// 拉入 _uid _gid 等字面量运算符
	using namespace mirai::literals;
	event.dispatch([&](const mirai::TempMessage& e)
		{
			auto msg = e.message;
			if (const auto opt = msg.content.match_types<msg::Plain>())
			{
				const auto [plain] = *opt;
				help(session, e.sender, plain.text);
				select_song(session, e.sender, plain.text);
				show_select_info(session, e, plain.text);
			}
		});

	event.dispatch([&](const mirai::GroupMessage& e)
		{
			const auto group = e.sender.group;
			auto msg = e.message;
			if (group.id == 105892544 || group.id == 279023542)
			{
				if (const auto opt = e.message.content.match_types<msg::At, msg::Plain>())
				{
					// 切分为at和文本部分，然后拼成一个tuple
					const auto [at, plain] = *opt;
					if (at.target != session.qq()) return;

					show_select_info(session, e, plain.text);
					help(session, e.sender, plain.text);

					// 管理员和主人命令
					if (e.sender.permission == permission::member) return;

					allow_select(session, e.sender, plain.text);
					change_size(plain.text);
					change_limited(session, e.sender, plain.text);

					if (e.sender.permission == permission::owner)
					{
						debug(plain.text);
					}
				}
			}
		});
}

void help(mirai::Session& session, const mirai::Member& e, const string& command)
{
	if (command == " /help" || command == " ")
	{
		const mirai::uid_t qq_id = e.id;
		const mirai::gid_t group_id = 105892544_gid;
		session.send_message(qq_id, group_id, "欢迎参加本次 G26 比赛！以下是本次参赛用到的指令：\n"
			"私聊命令：\n"
			"选择 + 序号：用于选歌的指令。只能私聊使用。注意，选好之后就不能更改了，所以想好之后再发噢。\n"
			"选歌情况：用于查看选歌情况。\n"
			"群聊命令（请在输入指令前艾特机绿）：\n"
			"选歌情况：用于查看选歌情况。\n"
			"（管理员）开始选歌/结束选歌\n"
			"如果出现任何异常请立刻联系夜轮。祝比赛愉快");
	}
}
// 参赛选手选歌
void select_song(mirai::Session& session, const mirai::Member& e, const string& command)
{
	const mirai::uid_t qq_id = e.id;
	const mirai::gid_t group_id = 105892544_gid;
	const permission p = e.permission;
	std::regex reg(R"re(\s*(select|选择|選擇)\s([1-9]\d*$))re");
	std::smatch res;

	std::cout << qq_id << ' ' << group_id << '\n';

	if (!std::regex_match(command, res, reg)) return;
	try
	{
		// 暂不允许选歌
		if (!is_allowed_to_select)
		{
			session.send_message(group_id, "提醒各位，主催未宣布比赛开始，请耐心等待");
			session.send_message(qq_id, group_id, "主催未宣布比赛开始，请耐心等待");
			return;
		}

		// 判断权限
		if (p != permission::member)
		{
			session.send_message(qq_id, group_id, "主催不能选歌，不许捣乱。");
			return;
		}

		// 选手还没选歌
		if (selected_member.find(qq_id) == selected_member.end())
		{
			const unsigned int order = std::stoul(res.str(2));
			if (order > limited)
			{
				session.send_message(qq_id, group_id, "超过曲目编号上限。当前的上限是：" + to_string(limited));
				return;
			}

			if (allow_size[order] == 0)
				allow_size[order] = default_size;
			// 如果还没被选满
			if (select_res[order].size() < allow_size[order])
			{
				select_res[order].push_back(qq_id);
				select_res_name[order].push_back(e.member_name);
				selected_member.insert(qq_id);

				session.send_message(qq_id, group_id, "选歌成功，记住选好了就不能再反悔了");
				string msg = "第" + res.str(2) + "首曲子已被选择" + to_string(select_res[order].size()) + "次。";
				if (select_res[order].size() == allow_size[order])
					msg += "本曲的名额已用尽。";
				session.send_message(group_id, msg);

				save_select_res();
			}
			else
				session.send_message(qq_id, group_id, "这首曲子已经没有名额了");
		}
		else
			session.send_message(qq_id, group_id, "你已经选过歌了");
	}
	catch (const std::exception& exc)
	{
		const string what = exc.what();
		std::cout << command + ' ' + what << '\n';
		session.send_message(562231326_uid, command + ' ' + what);
	}
}

// 初始化
void g26_init()
{
	if (!fs::exists("select_result.json"))
	{
		j["allow_select"] = false;
		j["limited"] = limited;

		for (size_t i = 1; i <= limited; i++)
		{
			string order = to_string(i);
			j[order]["size"] = default_size;
			j[order]["member_id"] = std::vector<mirai::uid_t>();
			j[order]["member_name"] = std::vector<string>();
		}
		std::ofstream o("select_result.json");
		o << std::setw(4) << j << std::endl;
	}
	else
	{
		std::ifstream i("select_result.json");
		i >> j;
		is_allowed_to_select = j["allow_select"];
		limited = j["limited"];
		for (auto& it : j.items())
		{
			if (!isdigit(it.key().at(0)))
				break;
			const int key = stoi(it.key());

			std::vector<mirai::uid_t> member_id = it.value().at("member_id");
			select_res[key] = member_id;

			// 把选歌人的信息导入到 selected_member 中
			for (auto& k : member_id)
				selected_member.insert(k);

			std::vector<string> member_name = it.value().at("member_name");
			select_res_name[key] = member_name;
		}
	}
}

// 展示当前选歌情况（群聊）
void show_select_info(mirai::Session& session, const mirai::GroupMessage& e, const string& command)
{
	if (command == " 选歌情况" || command == " 選歌情況")
	{
		if (select_res.empty())
			session.send_message(e.sender.group.id, "列表是空的");
		else
		{
			string msg = "当前选歌情况\n";
			for (auto& it : select_res)
			{
				msg += to_string(it.first) + ":\n";
				for (auto jt = it.second.begin(); jt != it.second.end(); ++jt)
					msg += "***\n";
				msg += "---\n";
			}
			session.send_message(e.sender.group.id, msg);
		}
	}
}

// 展示当前选歌情况（私聊）
void show_select_info(mirai::Session& session, const mirai::TempMessage& e, const string& command)
{
	if (command == "选歌情况" || command == "選歌情況")
	{
		if (select_res.empty())
			session.send_message(e.sender.group.id, "列表是空的");
		else
		{
			string msg = "当前选歌情况\n";
			for (auto& it : select_res)
			{
				msg += to_string(it.first) + ":\n";
				for (auto jt = it.second.begin(); jt != it.second.end(); ++jt)
				{
					if (e.sender.id == (*jt).id)
						msg += "你自己\n";
					else
						msg += "***\n";
				}
				msg += "---\n";
			}
			session.send_message(e.sender.id, e.sender.group.id, msg);
		}
	}
}

// 保存结果
void save_select_res()
{
	// 保存 size
	for (auto& it : allow_size)
		j[to_string(it.first)]["size"] = it.second;

	// 保存选歌情况
	for (auto& it : select_res)
	{
		auto i = to_string(it.first);
		j[i]["member_id"] = it.second;
	}
	for (auto& it : select_res_name)
	{
		auto i = to_string(it.first);
		j[i]["member_name"] = it.second;
	}

	j["allow_select"] = is_allowed_to_select;
	j["limited"] = limited;

	std::ofstream o("select_result.json");
	o << std::setw(4) << j << std::endl;
}

// （管理员）允许选歌的指令
void allow_select(mirai::Session& session, const mirai::Member& e, const string& command)
{
	if (command == " 开始选歌")
	{
		is_allowed_to_select = true;
		session.send_message(e.group.id, mirai::Message(msg::AtAll{}) + " 选歌开始！");
	}

	if (command == " 停止选歌")
	{
		is_allowed_to_select = false;
		session.send_message(e.group.id, mirai::Message(msg::AtAll{}) + " 选歌已停止。");
	}

	j["allow_select"] = is_allowed_to_select;
	std::ofstream o("select_result.json");
	o << std::setw(4) << j << std::endl;
}

// （管理员）改变组的大小（全部或部分）
void change_size(const string& command)
{
	const std::regex reg(R"re(\s*(修改上限)\s*([1-9]\d*|全部)\s*([1-9]\d*))re");
	std::smatch res;
	if (!std::regex_match(command, res, reg))	return;

	if (res.str(2) == "全部")
	{
		const int new_size = std::stoi(res.str(3));
		for (auto& i : select_res)
			i.second.resize(new_size);
		for (auto& i : allow_size)
			i.second = new_size;
	}
	else
	{
		const unsigned int order = std::stoul(res.str(2));
		const unsigned int size = std::stoul(res.str(3));
		select_res[order].resize(size);
		allow_size[order] = size;
	}
}

// （管理员）改变曲目数
void change_limited(mirai::Session& session, const mirai::Member& e, const string& command)
{
	const std::regex reg(R"re(\s*(修改曲数)\s*([1-9]\d*)re");
	std::smatch res;
	if (!std::regex_match(command, res, reg))	return;
	limited = stoi(res.str(2));
	session.send_message(e.group.id, mirai::Message(msg::AtAll{}) + " 曲目上限已修改为：" + res.str(2));

	j["limited"] = limited;
	std::ofstream o("select_result.json");
	o << std::setw(4) << j << std::endl;
}

void debug(const string& command)
{
	if (command == " /debug")
	{
		std::cout << "selected_member\n";
		for (const auto& it : selected_member)
			std::cout << it << '\n';
		std::cout << "---\nselect_res\n";
		for (const auto& it : select_res)
		{
			std::cout << it.first << '\n';
			for (const auto& jt : it.second)
				std::cout << jt.id << '\n';
		}
		std::cout << "---\nselect_res_name\n";
		for (const auto& it : select_res_name)
		{
			std::cout << it.first << '\n';
			for (const auto& jt : it.second)
				std::cout << jt << '\n';
		}
		std::cout << "---\nallow_size\n";
		for (const auto& it : allow_size)
			std::cout << it.first << ": " << it.second << '\n';
		std::cout << "---\n";
		std::cout << "is_allowed_to_select: " << (is_allowed_to_select ? "Yes" : "No") << '\n';
		std::cout << "default_size: " << default_size << '\n';
		std::cout << "limited: " << limited << '\n';
	}
}