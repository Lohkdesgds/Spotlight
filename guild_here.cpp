#include "guild_here.h"

GuildConf::GuildConf(GuildConf&& c)
{
	std::lock_guard<std::mutex> l(m);

	points_to_role = std::move(c.points_to_role);
	chat_registry = c.chat_registry;
}

void GuildConf::start(const GuildConf& c)
{
	points_to_role = c.points_to_role;
	chat_registry = c.chat_registry;
}

nlohmann::json GuildConf::save() const
{
	// ALWAYS SEE start() AND COPY CONSTRUCTOR!!!

	nlohmann::json j;

	for(const auto& i : points_to_role)
		j["rules"].push_back(i);

	j["registry"] = chat_registry;

	return j;
}

void GuildConf::load(const nlohmann::json& j)
{
	// ALWAYS SEE start() AND COPY CONSTRUCTOR!!!

	if (j.count("rules") && !j["rules"].is_null()) {
		for (const auto& _field : j["rules"])
			points_to_role.push_back(_field.get<std::pair<unsigned long long, unsigned long long>>());
	}

	if (j.count("registry") && !j["registry"].is_null()) chat_registry = j["registry"];
}


bool ControlGuilds::save_nolock(const aegis::snowflake& gid, const GuildConf& gg)
{
	if (!gid) throw std::exception("NULL guild id!");
	CreateDirectoryA("servers", NULL);

	auto cpy = gg.save().dump();
	if (cpy.empty()) {
		std::cout << "Guild #" << gid << " had null/empty config. Not saving." << std::endl;
		return true; // no config also means no one has set a config, so no error.
	}

	std::unique_ptr<FILE, std::function<void(FILE*)>> fp = std::move(std::unique_ptr<FILE, std::function<void(FILE*)>>([&] {
		std::string p = "servers/" + std::to_string(gid) + ".sl1";
		FILE* f;
		fopen_s(&f, p.c_str(), "wb");
		return f;
		}(), [](FILE* f) {fclose(f); }));

	if (!fp.get()) return false;

	fwrite(cpy.c_str(), sizeof(char), cpy.length(), fp.get());
	fp.reset();
	return true;
}

GuildConf ControlGuilds::load_nolock(const aegis::snowflake& gid)
{
	if (!gid) throw std::exception("NULL guild id!");
	CreateDirectoryA("servers", NULL);

	std::unique_ptr<FILE, std::function<void(FILE*)>> fp = std::move(std::unique_ptr<FILE, std::function<void(FILE*)>>([&]{
		std::string p = "servers/" + std::to_string(gid) + ".sl1";
		FILE* f;
		fopen_s(&f, p.c_str(), "rb");
		return f;
		}(), [](FILE* f) {fclose(f); }));

	if (!fp.get()) return GuildConf{};

	std::string all_buf;
	char quickbuf[256];
	size_t u = 0;
	while (u = fread_s(quickbuf, 256, sizeof(char), 256, fp.get())) {
		for (size_t k = 0; k < u; k++) all_buf += quickbuf[k];
	}

	nlohmann::json j = nlohmann::json::parse(all_buf);

	GuildConf gg;
	gg.load(j);
	return std::move(gg);
}


GuildConf& ControlGuilds::grab_guild(const aegis::snowflake& guild)
{
	if (!guild) throw std::exception("invalid guild!");
	std::lock_guard<std::mutex> l(access_guilds);

	auto a = known_guilds.find(guild);
	if (a != known_guilds.end()) {
		return a->second;
	}

	auto& ref = known_guilds[guild];
	ref.start(load_nolock(guild));
	return ref;
}

void ControlGuilds::reapply_user_on_guild(aegis::guild& rawg, const aegis::snowflake& guild, const aegis::snowflake& userid, UserConf& user)
{
	std::lock_guard<std::mutex> l(user.m);
	auto& wrk = grab_guild(guild);
	std::lock_guard<std::mutex> l2(wrk.m);

	for (auto& i : wrk.points_to_role) // i.first role, i.second points
	{
		for (auto& j : user.conquered_roles) {
			if (j == i.first) {
				rawg.add_guild_member_role(userid, i.first);
			}
		}
	}
}

std::pair<unsigned long long, unsigned long long> ControlGuilds::next_role_for(const aegis::snowflake& guild, UserConf& user)
{
	std::lock_guard<std::mutex> l(user.m);
	auto& wrk = grab_guild(guild);
	std::lock_guard<std::mutex> l2(wrk.m);

	const auto& user_points = user.pts;

	for (auto& i : wrk.points_to_role) // i.first role, i.second points
	{
		bool has_already = false;
		for (auto& j : user.conquered_roles) {
			if (j == i.first) {
				has_already = true;
				break;
			}
		}
		if (!has_already) {
			return i; // next available one
		}
		// else search for new one
	}
	return { 0ULL, 0ULL }; // none
}

unsigned long long ControlGuilds::last_role_from(const aegis::snowflake& guild, UserConf& user)
{
	std::lock_guard<std::mutex> l(user.m);
	auto& wrk = grab_guild(guild);
	std::lock_guard<std::mutex> l2(wrk.m);

	for (size_t p = user.conquered_roles.size() - 1; p != static_cast<size_t>(-1); p--) // loops to -1
	{
		auto& i = user.conquered_roles[p];
		for (auto& j : wrk.points_to_role) {
			if (j.first == i) return i;
		}
	}
	return 0;
}

bool ControlGuilds::server_has_no_role_system(const aegis::snowflake& guild)
{
	auto& wrk = grab_guild(guild);
	std::lock_guard<std::mutex> l(wrk.m);
	return !wrk.points_to_role.size();
}

void ControlGuilds::reg_add(const aegis::snowflake& guild, const unsigned long long role, const unsigned long long pts)
{
	auto& wrk = grab_guild(guild);
	std::lock_guard<std::mutex> l(wrk.m);
	bool add = true;

	//guild_ptr->add_guild_member_role(who, 0);

	for (auto& i : wrk.points_to_role) {
		if (i.first == role) {
			i.second = pts;
			add = false;
			break;
		}
	}
	if (add) wrk.points_to_role.push_back({ role, pts });

	// no sort because this is not how it should work
	/*std::sort(wrk.points_to_role.begin(), wrk.points_to_role.end(), [](auto& left, auto& right) {
		return left.second < right.second;
	});*/

	flush(guild);
}

void ControlGuilds::reg_remove(const aegis::snowflake& guild, const unsigned long long role)
{
	auto& wrk = grab_guild(guild);
	std::lock_guard<std::mutex> l(wrk.m);

	for (size_t p = 0; p < wrk.points_to_role.size(); p++)
	{
		if (wrk.points_to_role[p].first == role) {
			wrk.points_to_role.erase(wrk.points_to_role.begin() + p);
			flush(guild);
			return;
		}
	}
}

void ControlGuilds::reg_remove_all(const aegis::snowflake& guild)
{
	auto& wrk = grab_guild(guild);
	std::lock_guard<std::mutex> l(wrk.m);

	wrk.points_to_role.clear();
	flush(guild);
}

std::string ControlGuilds::list(const aegis::snowflake& guild)
{
	auto& wrk = grab_guild(guild);
	std::lock_guard<std::mutex> l(wrk.m);

	std::string endd;
	size_t count = 0;

	for (auto& i : wrk.points_to_role)
	{
		endd += "[#" + std::to_string(++count) + "] `" + std::to_string(i.second) + u8"` ↣ <@&" + std::to_string(i.first) + "> (ID" + std::to_string(i.first) + ")\n";
	}
	if (!endd.empty()) endd.pop_back(); // '\n'

	return endd;
}

unsigned long long ControlGuilds::get_registry_chat(const aegis::snowflake& guild)
{
	return grab_guild(guild).chat_registry;
}

void ControlGuilds::set_registry_chat(const aegis::snowflake& guild, const unsigned long long chatid)
{
	auto& wrk = grab_guild(guild);
	std::lock_guard<std::mutex> l(wrk.m);
	wrk.chat_registry = chatid;
	flush(guild);
}

void ControlGuilds::flush(const aegis::snowflake& guild)
{
	if (!guild) throw std::exception("invalid guild!");
	std::lock_guard<std::mutex> l(access_guilds);

	auto a = known_guilds.find(guild);
	if (a != known_guilds.end()) {
		save_nolock(guild, a->second);
	}
}