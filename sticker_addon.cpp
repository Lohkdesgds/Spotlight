#include "sticker_addon.h"

void _sticker_addon::from_json(const nlohmann::json& j)
{
	if (j.count("id") && !j["id"].is_null())						id = j["id"];
	if (j.count("pack_id") && !j["pack_id"].is_null())				pack_id = j["pack_id"];
	if (j.count("name") && !j["name"].is_null())					name = j["name"];
	if (j.count("description") && !j["description"].is_null())		description = j["description"];
	if (j.count("tags") && !j["tags"].is_null())					tags = j["tags"];
}

_sticker_addon::_sticker_addon(const nlohmann::json& j)
{
	from_json(j);
}
