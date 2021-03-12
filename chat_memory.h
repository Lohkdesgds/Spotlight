#pragma once

#include <aegis.hpp>

const std::chrono::seconds delta_for_point = std::chrono::seconds(45);
const std::chrono::seconds delta_max_for_point = std::chrono::seconds(60 * 10);

struct ChatTrigger {
	aegis::snowflake chatid{}, lastuserid{};
	std::chrono::system_clock::time_point last_message{};
	std::mutex m;

	ChatTrigger() = default;
	ChatTrigger(const aegis::snowflake&);

	// also update time if true. userid
	bool check_point_can_get(const aegis::snowflake&);
};

class ControlChats {
	std::mutex m;
	std::unordered_map<aegis::snowflake, ChatTrigger> chats;
public:
	// chat, user
	bool points_can(const aegis::snowflake&, const aegis::snowflake&);
};


inline ControlChats global_chats;