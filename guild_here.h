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
	std::string alias;
	unsigned long long chat_registry{}, chat_commands{};

	nlohmann::json save() const;
	void load(const nlohmann::json&);
};

class ControlGuilds {
	std::unordered_map<aegis::snowflake, GuildConf> known_guilds;
	mutable std::mutex access_guilds;

	bool save_nolock(const aegis::snowflake&, const GuildConf&);
	GuildConf load_nolock(const aegis::snowflake&);

	void flush_nolock(const aegis::snowflake&);
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
	// copy to this if any
	unsigned long long get_command_chat(const aegis::snowflake&);
	// set registry chat
	void set_command_chat(const aegis::snowflake&, const unsigned long long);
	// get alias
	std::string get_alias(const aegis::snowflake&);
	// set alias
	void set_alias(const aegis::snowflake&, const std::string&);

	
};

inline ControlGuilds global_guilds;