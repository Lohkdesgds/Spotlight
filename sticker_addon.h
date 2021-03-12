#pragma once

#include <aegis.hpp>

struct _sticker_addon {
	aegis::snowflake id;//	snowflake	id of the sticker
	aegis::snowflake pack_id;//	snowflake	id of the pack the sticker is from
	std::string name;//	string	name of the sticker
	std::string description;//	string	description of the sticker
	std::string tags;// ? string	a comma - separated list of tags for the sticker
	//std::string asset;// *string	sticker asset hash
	//std::string preview_asset;// *? string	sticker preview asset hash
	//int format_type; //	integer	type of sticker format

	void from_json(const nlohmann::json&);

	_sticker_addon() = default;
	_sticker_addon(const nlohmann::json&);
};