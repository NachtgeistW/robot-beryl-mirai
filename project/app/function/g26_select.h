#pragma once

#include <regex>

#include "../header.hpp"

// 一些频繁用到的命名空间
namespace msg = mirai::msg;
namespace ut = mirai::utils;
using namespace mirai::literals;

using permission = mirai::Permission;
using std::string;
using std::to_string;
using json = nlohmann::json;

void g26_init();
void help(mirai::Session& session, const mirai::Member& e, const string& command);
void g26_select(mirai::Session& session, const mirai::Event& event);
void show_select_info(mirai::Session& session, const mirai::GroupMessage& e, const string& command);
void show_select_info(mirai::Session& session, const mirai::TempMessage& e, const string& command);
void change_size(const string& command);
void change_limited(mirai::Session& session, const mirai::Member& e, const string& command);
void allow_select(mirai::Session& session, const mirai::Member& e, const string& command);
void select_song(mirai::Session& session, const mirai::Member& e, const string& command);
void save_select_res();
void debug(const string& command);
