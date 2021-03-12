#pragma once

#include <aegis.hpp>

struct UserConf {
	UserConf() = default;
	UserConf(UserConf&&);
	// for start only, does not copy buffer!
	void start(const UserConf&);

	std::mutex m;
	unsigned long long pts{};
	std::vector<unsigned long long> conquered_roles;

	nlohmann::json save() const;
	void load(const nlohmann::json&);
};

class ControlUsers {
	std::unordered_map<aegis::snowflake, UserConf> known_users;
	mutable std::mutex access_guilds;

	bool save_nolock(const aegis::snowflake&, const UserConf&);
	UserConf load_nolock(const aegis::snowflake&);
public:
	UserConf& grab_user(const aegis::snowflake&);

	unsigned long long get(const aegis::snowflake&);
	void add(const aegis::snowflake&, const long long);

	size_t users_known_size() const;

	void flush(const aegis::snowflake&);
};

inline ControlUsers global_users;