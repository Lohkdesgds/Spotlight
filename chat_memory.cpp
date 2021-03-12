#include "chat_memory.h"

ChatTrigger::ChatTrigger(const aegis::snowflake& chid)
{
	chatid = chid;
	last_message = std::chrono::system_clock::now();
}

bool ChatTrigger::check_point_can_get(const aegis::snowflake& uid)
{
	std::lock_guard<std::mutex> l(m);

	const auto now = std::chrono::system_clock::now();
	if ((now - last_message) > delta_for_point && lastuserid != uid || (now - last_message) > delta_max_for_point) {
		last_message = now;
		lastuserid = uid;
		return true;
	}
	return false;
}

bool ControlChats::points_can(const aegis::snowflake& c, const aegis::snowflake& u)
{
	std::lock_guard<std::mutex> l(m);
	return chats[c].check_point_can_get(u);
}