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
#include <boost/filesystem.hpp>

using namespace boost;

static int filesystem_absolute(lua_State* L){
	lua_pushstring(L, filesystem::absolute(luaL_checkstring(L, 1)).c_str());
	return 1;
}

static int filesystem_canonical(lua_State* L){
	try{
		lua_pushstring(L, filesystem::canonical(luaL_checkstring(L, 1)).c_str());
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
		filesystem::create_symlink(luaL_checkstring(L, 1), luaL_checkstring(L, 2));
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
			lua_pushstring(L, filesystem::current_path().c_str());
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

static int filesystem_filesize(lua_State* L){
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
	static const filesystem::perms option_enum[] = {filesystem::perms::add_perms, filesystem::perms::remove_perms, filesystem::perms::owner_read, filesystem::perms::owner_write, filesystem::perms::owner_exe, filesystem::perms::owner_all, filesystem::perms::group_read, filesystem::perms::group_write, filesystem::perms::group_exe, filesystem::perms::group_all, filesystem::perms::others_read, filesystem::perms::others_write, filesystem::perms::others_exe, filesystem::perms::others_all};
	const int top = lua_gettop(L);
	try{
		if(top > 1){
			filesystem::perms perms = filesystem::no_perms;
			for(int i = 2; i <= top; ++i)
				perms |= option_enum[luaL_checkoption(L, i, nullptr, option_str)];
			filesystem::permissions(luaL_checkstring(L, 1), perms);
		}else{
			const filesystem::perms perms = filesystem::status(luaL_checkstring(L, 1)).permissions();
			static const int n = sizeof(option_enum)/sizeof(option_enum[0]);
			for(int i = 0; i < n; ++i)
				if(perms & option_enum[i])
					lua_pushstring(L, option_str[i]);
			return lua_gettop(L)-1;
		}
	}catch(const filesystem::filesystem_error& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

/*static int filesystem_(lua_State* L){

	// TODO

	return 0;
}

static int filesystem_(lua_State* L){

	// TODO

	return 0;
}

static int filesystem_(lua_State* L){

	// TODO

	return 0;
}

static int filesystem_(lua_State* L){

	// TODO

	return 0;
}

static int filesystem_(lua_State* L){

	// TODO

	return 0;
}

static int filesystem_(lua_State* L){

	// TODO

	return 0;
}

static int filesystem_(lua_State* L){

	// TODO

	return 0;
}*/

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
		{"filesize", filesystem_filesize},
		{"lastmod", filesystem_lastmod},
		{"perm", filesystem_perm},
		/*{"", filesystem_},
		{"", filesystem_},
		{"", filesystem_},
		{"", filesystem_},
		{"", filesystem_},
		{"", filesystem_},
		{"", filesystem_},*/
		{NULL, NULL}
	};
	luaL_newlib(L, l);
	return 1;
}
