#include "user_here.h"

UserConf::UserConf(UserConf&& c)
{
	std::lock_guard<std::mutex> l(m);

	pts = std::move(c.pts);
	conquered_roles = std::move(c.conquered_roles);
}

void UserConf::start(const UserConf& c)
{
	pts = c.pts;
	conquered_roles = c.conquered_roles;
}

nlohmann::json UserConf::save() const
{
	// ALWAYS SEE start() AND COPY CONSTRUCTOR!!!

	nlohmann::json j;
	j["points"] = pts;
	for (const auto& i : conquered_roles)
		j["roles"].push_back(i);

	return j;
}

void UserConf::load(const nlohmann::json& j)
{
	// ALWAYS SEE start() AND COPY CONSTRUCTOR!!!

	if (j.count("points") && !j["points"].is_null()) pts = j["points"];
	if (j.count("roles") && !j["roles"].is_null()) {
		for (const auto& _field : j["roles"])
			conquered_roles.push_back(_field.get<unsigned long long>());
	}
}


bool ControlUsers::save_nolock(const aegis::snowflake& gid, const UserConf& gg)
{
	if (!gid) throw std::exception("NULL user id!");
	CreateDirectoryA("users", NULL);

	auto cpy = gg.save().dump();
	if (cpy.empty()) {
		std::cout << "User #" << gid << " had null/empty config. Not saving." << std::endl;
		return true; // no config also means no one has set a config, so no error.
	}

	std::unique_ptr<FILE, std::function<void(FILE*)>> fp = std::move(std::unique_ptr<FILE, std::function<void(FILE*)>>([&] {
		std::string p = "users/" + std::to_string(gid) + ".sl1";
		FILE* f;
		fopen_s(&f, p.c_str(), "wb");
		return f;
		}(), [](FILE* f) {fclose(f); }));

	if (!fp.get()) return false;

	fwrite(cpy.c_str(), sizeof(char), cpy.length(), fp.get());
	fp.reset();
	return true;
}

UserConf ControlUsers::load_nolock(const aegis::snowflake& gid)
{
	if (!gid) throw std::exception("NULL user id!");
	CreateDirectoryA("users", NULL);

	std::unique_ptr<FILE, std::function<void(FILE*)>> fp = std::move(std::unique_ptr<FILE, std::function<void(FILE*)>>([&] {
		std::string p = "users/" + std::to_string(gid) + ".sl1";
		FILE* f;
		fopen_s(&f, p.c_str(), "rb");
		return f;
		}(), [](FILE* f) {fclose(f); }));

	if (!fp.get()) return UserConf{};

	std::string all_buf;
	char quickbuf[256];
	size_t u = 0;
	while (u = fread_s(quickbuf, 256, sizeof(char), 256, fp.get())) {
		for (size_t k = 0; k < u; k++) all_buf += quickbuf[k];
	}

	nlohmann::json j = nlohmann::json::parse(all_buf);

	UserConf gg;
	gg.load(j);
	return std::move(gg);
}

UserConf& ControlUsers::grab_user(const aegis::snowflake& user)
{
	if (!user) throw std::exception("invalid user!");
	std::lock_guard<std::mutex> l(access_guilds);

	auto a = known_users.find(user);
	if (a != known_users.end()) {
		return a->second;
	}

	auto& ref = known_users[user];
	ref.start(load_nolock(user));
	return ref;
}

unsigned long long ControlUsers::get(const aegis::snowflake& user)
{
	return grab_user(user).pts;	
}

void ControlUsers::add(const aegis::snowflake& user, const long long delta)
{
	auto& usr = grab_user(user);

	std::lock_guard<std::mutex> l(usr.m);

	if (delta < 0 && usr.pts <= static_cast<unsigned long long>(-delta)) usr.pts = 1;
	else usr.pts += delta;

	flush(user);
}

size_t ControlUsers::users_known_size() const
{
	return known_users.size();
}

void ControlUsers::flush(const aegis::snowflake& user)
{
	if (!user) throw std::exception("invalid user!");
	std::lock_guard<std::mutex> l(access_guilds);

	auto a = known_users.find(user);
	if (a != known_users.end()) {
		save_nolock(user, a->second);
	}
}