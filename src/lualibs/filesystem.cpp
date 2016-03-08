/*
Project: FLuaG
File: filesystem.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include "../utils/lua.h"
#include <boost/filesystem.hpp>

using namespace boost;

static int filesystem_absolute(lua_State* L){
	lua_pushstring(L, filesystem::absolute(luaL_checkstring(L, 1)).string().c_str());
	return 1;
}

static int filesystem_canonical(lua_State* L){
	try{
		lua_pushstring(L, filesystem::canonical(luaL_checkstring(L, 1)).string().c_str());
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_copy(lua_State* L){
	try{
		filesystem::copy(luaL_checkstring(L, 1), luaL_checkstring(L, 2));
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_mkdir(lua_State* L){
	try{
		lua_pushboolean(L, filesystem::create_directories(luaL_checkstring(L, 1)));
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_link(lua_State* L){
	try{
		filesystem::create_hard_link(luaL_checkstring(L, 1), luaL_checkstring(L, 2));
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_symlink(lua_State* L){
	try{
		if(lua_gettop(L) > 1)
			filesystem::create_symlink(luaL_checkstring(L, 1), luaL_checkstring(L, 2));
		else{
			lua_pushstring(L, filesystem::read_symlink(luaL_checkstring(L, 1)).string().c_str());
			return 1;
		}
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_cd(lua_State* L){
	try{
		if(lua_gettop(L) > 0)
			filesystem::current_path(luaL_checkstring(L, 1));
		else{
			lua_pushstring(L, filesystem::current_path().string().c_str());
			return 1;
		}
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_exists(lua_State* L){
	try{
		lua_pushboolean(L, filesystem::exists(luaL_checkstring(L, 1)));
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_equal(lua_State* L){
	try{
		lua_pushboolean(L, filesystem::equivalent(luaL_checkstring(L, 1), luaL_checkstring(L, 2)));
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_size(lua_State* L){
	try{
		lua_pushinteger(L, filesystem::file_size(luaL_checkstring(L, 1)));
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_lastmod(lua_State* L){
	try{
		lua_pushinteger(L, filesystem::last_write_time(luaL_checkstring(L, 1)));
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_perm(lua_State* L){
	static const char* option_str[] = {"add", "remove", "owner_read", "owner_write", "owner_exe", "owner_all", "group_read", "group_write", "group_exe", "group_all", "others_read", "others_write", "others_exe", "others_all", nullptr};
	using perms = filesystem::perms;
	static const perms option_enum[] = {perms::add_perms, perms::remove_perms, perms::owner_read, perms::owner_write, perms::owner_exe, perms::owner_all, perms::group_read, perms::group_write, perms::group_exe, perms::group_all, perms::others_read, perms::others_write, perms::others_exe, perms::others_all};
	const int top = lua_gettop(L);
	try{
		if(top > 1){
			perms p = filesystem::no_perms;
			for(int i = 2; i <= top; ++i)
				p |= option_enum[luaL_checkoption(L, i, nullptr, option_str)];
			filesystem::permissions(luaL_checkstring(L, 1), p);
		}else{
			const perms p = filesystem::status(luaL_checkstring(L, 1)).permissions();
			static const int n = sizeof(option_enum)/sizeof(option_enum[0]);
			for(int i = 0; i < n; ++i)
				if(p & option_enum[i])
					lua_pushstring(L, option_str[i]);
			return lua_gettop(L)-1;
		}
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_remove(lua_State* L){
	try{
		if(luaL_optboolean(L, 2, false))
			lua_pushinteger(L, filesystem::remove_all(luaL_checkstring(L, 1)));
		else
			lua_pushboolean(L, filesystem::remove(luaL_checkstring(L, 1)));
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_rename(lua_State* L){
	try{
		filesystem::rename(luaL_checkstring(L, 1), luaL_checkstring(L, 2));
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_space(lua_State* L){
	try{
		const filesystem::space_info sinfo = filesystem::space(luaL_checkstring(L, 1));
        lua_pushinteger(L, sinfo.capacity);
        lua_pushinteger(L, sinfo.free);
        lua_pushinteger(L, sinfo.available);
		return 3;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_type(lua_State* L){
	try{
		static const char* types_str[] = {"status_error", "file_not_found", "regular_file", "directory_file", "symlink_file", "block_file", "character_file", "fifo_file", "socket_file", "type_unknown"};
		using ftype = filesystem::file_type;
		static const ftype types_enum[] = {ftype::status_error, ftype::file_not_found, ftype::regular_file, ftype::directory_file, ftype::symlink_file,
														ftype::block_file, ftype::character_file, ftype::fifo_file, ftype::socket_file, ftype::type_unknown};
		static const int n = sizeof(types_enum)/sizeof(types_enum[0]);
		const ftype ft = filesystem::status(luaL_checkstring(L, 1)).type();
		for(int i = 0; i < n; ++i)
			if(ft == types_enum[i]){
				lua_pushstring(L, types_str[i]);
				return 1;
			}
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_sysabsolute(lua_State* L){
	try{
		lua_pushstring(L, filesystem::system_complete(luaL_checkstring(L, 1)).string().c_str());
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_tmpdir(lua_State* L){
	try{
		lua_pushstring(L, filesystem::temp_directory_path().string().c_str());
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_unique(lua_State* L){
	try{
		lua_pushstring(L, filesystem::unique_path(luaL_checkstring(L, 1)).string().c_str());
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

static int filesystem_dir(lua_State* L){
	try{
		lua_newtable(L);
		int i = 0;
		for(filesystem::directory_iterator diter(luaL_checkstring(L, 1)), diter_end; diter != diter_end; diter++){
			lua_pushstring(L, (*diter).path().string().c_str()); lua_rawseti(L, -2, ++i);
		}
		return 1;
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

int luaopen_filesystem(lua_State* L){
	static const luaL_Reg l[] = {
		{"absolute", filesystem_absolute},
		{"canonical", filesystem_canonical},
		{"copy", filesystem_copy},
		{"mkdir", filesystem_mkdir},
		{"link", filesystem_link},
		{"symlink", filesystem_symlink},
		{"cd", filesystem_cd},
		{"exists", filesystem_exists},
		{"equal", filesystem_equal},
		{"size", filesystem_size},
		{"lastmod", filesystem_lastmod},
		{"perm", filesystem_perm},
		{"remove", filesystem_remove},
		{"rename", filesystem_rename},
		{"space", filesystem_space},
		{"type", filesystem_type},
		{"sysabsolute", filesystem_sysabsolute},
		{"tmpdir", filesystem_tmpdir},
		{"unique", filesystem_unique},
		{"dir", filesystem_dir},
		{NULL, NULL}
	};
	luaL_newlib(L, l);
	return 1;
}
