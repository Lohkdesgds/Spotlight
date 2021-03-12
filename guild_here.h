#pragma once

#include <aegis.hpp>

#include "user_here.h"

struct GuildConf {
	GuildConf() = default;
	GuildConf(GuildConf&&);
	// for start only, does not copy buffer!
	void start(const GuildConf&);

	std::mutex m;
	// role, pts
	std::vector<std::pair<unsigned long long, unsigned long long>> points_to_role;
	unsigned long long chat_registry{};

	nlohmann::json save() const;
	void load(const nlohmann::json&);
};

class ControlGuilds {
	std::unordered_map<aegis::snowflake, GuildConf> known_guilds;
	mutable std::mutex access_guilds;

	bool save_nolock(const aegis::snowflake&, const GuildConf&);
	GuildConf load_nolock(const aegis::snowflake&);
public:
	GuildConf& grab_guild(const aegis::snowflake&);

	// get list of roles of this guild and reapply roles that user has. Guild, guildid, userid, User
	void reapply_user_on_guild(aegis::guild&, const aegis::snowflake&, const aegis::snowflake&, UserConf&);
	// first == role
	std::pair<unsigned long long, unsigned long long> next_role_for(const aegis::snowflake&, UserConf&);
	// last from this guild
	unsigned long long last_role_from(const aegis::snowflake&, UserConf&);
	// true if server has no config
	bool server_has_no_role_system(const aegis::snowflake&);
	// add new reg: guildid, roleid, points
	void reg_add(const aegis::snowflake&, const unsigned long long, const unsigned long long);
	// remove reg: guildid, roleid
	void reg_remove(const aegis::snowflake&, const unsigned long long);
	// remove reg: guildid (all)
	void reg_remove_all(const aegis::snowflake&);
	// list in order
	std::string list(const aegis::snowflake&);
	// copy to this if any
	unsigned long long get_registry_chat(const aegis::snowflake&);
	// set registry chat
	void set_registry_chat(const aegis::snowflake&, const unsigned long long);
	
	void flush(const aegis::snowflake&);
};

inline ControlGuilds global_guilds;