#include "LuaConfig.h"
#include <fstream>
#include <streambuf>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "string_common.h"

using namespace std;

void traverse_table(lua_State *L, int index)
{
 	lua_pushnil(L);
	// 现在的栈：-1 => nil; index => table
	while (lua_next(L, index))
	{
		// 现在的栈：-1 => value; -2 => key; index => table
		// 拷贝一份 key 到栈顶，然后对它做 lua_tostring 就不会改变原始的 key 值了
		lua_pushvalue(L, -2);
		// 现在的栈：-1 => key; -2 => value; -3 => key; index => table
		const char* key = lua_tostring(L, -1);
		const char* value = lua_tostring(L, -2);

		printf("%s => %s\n", key, value);

		// 弹出 value 和拷贝的 key，留下原始的 key 作为下一次 lua_next 的参数
		lua_pop(L, 1);		//弹出key

		if (lua_istable(L, -1))
		{
			int newIndex = lua_gettop(L);  // 取 table 索引值 
			traverse_table(L, newIndex);
		}
		
		lua_pop(L, 1);
		// 现在的栈：-1 => key; index => table
	}
	// 现在的栈：index => table （最后 lua_next 返回 0 的时候它已经把上一次留下的 key 给弹出了）
	// 所以栈已经恢复到进入这个函数时的状态
}


void parse_lua_table(lua_State* L)
{
	if (!lua_istable(L, -1))
		return;
	int nIndex = lua_gettop(L);  // 取 table 索引值 
	traverse_table(L, nIndex);
}





void doCaclTableLevels(lua_State* L, int curlevel, int& outLv)
{
	if (!lua_istable(L, -1))
		return;
	int nIndex = lua_gettop(L);  // 取 table 索引值 
	lua_pushnil(L);
	while (lua_next(L, nIndex))
	{
		if (lua_istable(L, -1))
		{
			int newIndex = lua_gettop(L);  // 取 table 索引值 
			doCaclTableLevels(L, curlevel + 1, outLv);
		}
		else
		{
			if (outLv == -1 || curlevel < outLv)
			{
				outLv = curlevel;
			}
		}

		lua_pop(L, 1);
	}
}

int calcTableLevels(lua_State* L)
{
	if (!lua_istable(L, -1))
		return 0;
	
	int outLv = -1;
	doCaclTableLevels(L, 0, outLv);

	return outLv;
}


struct TableData
{
	int totallv;
	std::vector<std::string> desc;
	std::vector<std::vector<std::string> > tables;

	int curline;
	std::vector<string> lvValues;
	std::map<std::string, int> keymap;

	void setData(const std::string& key, const std::string& value)
	{
		if (tables.size() < curline + 1)
			tables.resize(curline + 1);

		int keyidx = 0;
		auto keyit = keymap.find(key);
		if (keyit == keymap.end())
		{
			keyidx = desc.size();
			keymap[key] = keyidx;
			desc.push_back(key);
		}
		else
		{
			keyidx = keyit->second;
		}

		if (tables[curline].size() < keyidx + 1)
			tables[curline].resize(keyidx + 1);
		tables[curline][keyidx] = value;
	}


	void write(const std::string& outname)
	{
		ofstream oFile;
		//打开要输出的文件   
		oFile.open(outname, ios::out | ios::trunc);    // 这样就很容易的输出一个需要的excel 文件  

		for (int i = 0; i < desc.size(); ++i)
			oFile << desc[i] << "\t";
		oFile << "\n";

		for (int i = 0; i < tables.size(); ++i)
		{
			for (int j = 0; j < tables[i].size(); ++j)
			{
				oFile << tables[i][j] << "\t";
			}
			oFile << "\n";
		}

		oFile.close();
	}

	void newline()
	{
		if (tables.size() >= curline)
		{
			for (int i = 0; i < totallv; ++i)
				tables[curline][i] = lvValues[i];
		}
		++curline;
		if (tables.size() < curline + 1)
			tables.resize(curline + 1);
		tables[curline].resize(desc.size() + 1);
	}
};

bool isLuaArrayTable(lua_State* L)
{
	int newIndex = lua_gettop(L);  // 取 table 索引值 
	lua_pushnil(L);

	int maxkey = 0;
	while (lua_next(L, newIndex))
	{
		lua_pushvalue(L, -2);
		bool isnumberkey = lua_type(L, -1) == LUA_TNUMBER;

		if (!isnumberkey)
		{
			lua_pop(L, 1);		//弹出key
			lua_pop(L, 1);		//弹出Value
			lua_pop(L, 1);		//弹出key

			return false;
		}

		double keynumber = lua_tonumber(L, -1);
		if (keynumber <= 0 || keynumber != (int)keynumber)
		{
			lua_pop(L, 1);		//弹出key
			lua_pop(L, 1);		//弹出Value
			lua_pop(L, 1);		//弹出key

			return false;
		}

		if (maxkey < keynumber)
		{
			maxkey = (int)keynumber;
		}

		lua_pop(L, 1);		//弹出key
		lua_pop(L, 1);		//弹出Value
	}

	int size = lua_objlen(L, -1);
	return size == maxkey;
}

std::string printTable(lua_State* L)
{
	std::string ret = "{";

	if (isLuaArrayTable(L))
	{
		int size = lua_objlen(L, -1);
		for (int i = 0; i < size; ++i)
		{
			lua_pushnumber(L, i + 1);
			lua_rawget(L, -2);

			bool isstringvalue = lua_type(L, -1) == LUA_TSTRING;
			const char* value = lua_tostring(L, -1);

			if (lua_istable(L, -1))
				ret += printTable(L);
			else if (isstringvalue)
				ret += CGMISC::toString("\"%s\"", value);
			else
				ret += CGMISC::toString("%s", value);
			ret += ",";

			lua_pop(L, 1);
		}
	}
	else
	{
		int newIndex = lua_gettop(L);  // 取 table 索引值 
		lua_pushnil(L);
		while (lua_next(L, newIndex))
		{
			lua_pushvalue(L, -2);
			bool isstringkey = lua_type(L, -1) == LUA_TSTRING;
			const char* key_ = lua_tostring(L, -1);
			bool isstringvalue = lua_type(L, -2) == LUA_TSTRING;
			const char* value = lua_tostring(L, -2);

			lua_pop(L, 1);		//弹出key

			if (key_ == 0)
			{
				printf("failed: bad key!\n");
				key_ = "Bad_key";
			}

			if (isstringkey)
				ret += key_;
			else
				ret += CGMISC::toString("[%s]", key_);
			ret += "=";

			if (lua_istable(L, -1))
				ret += printTable(L);
			else if (isstringvalue)
				ret += CGMISC::toString("\"%s\"", value);
			else
				ret += CGMISC::toString("%s", value);
			ret += ",";

			lua_pop(L, 1);
		}

	}
	ret += "}";

	return ret;
}

void traverse_table_andwrite(lua_State *L, int curLv, TableData& data, const std::string& key_prefix = "", bool splitsubtable = true)
{
	int newIndex = lua_gettop(L);  // 取 table 索引值 

	lua_pushnil(L);
	// 现在的栈：-1 => nil; index => table
	while (lua_next(L, newIndex))
	{
		// 现在的栈：-1 => value; -2 => key; index => table
		// 拷贝一份 key 到栈顶，然后对它做 lua_tostring 就不会改变原始的 key 值了
		lua_pushvalue(L, -2);
		// 现在的栈：-1 => key; -2 => value; -3 => key; index => table
		const char* key_ = lua_tostring(L, -1);
		const char* value = lua_tostring(L, -2);

		// 弹出 value 和拷贝的 key，留下原始的 key 作为下一次 lua_next 的参数
		lua_pop(L, 1);		//弹出key

		if (key_ == 0)
		{
			printf("failed: bad key!\n");
			key_ = "Bad_key";
		}

// 		printf("%s => %s\n", key_, value);
		std::string key = key_;

		if (curLv > data.totallv)
		{
			key = CGMISC::toString("%s_%s", key_prefix.c_str(), key_); 
		}

		std::string newkeyprefix = key_prefix;
		if (curLv >= data.totallv)
		{
			newkeyprefix = CGMISC::toString("%s_%s", key_prefix.c_str(), key_);
		}

		if (curLv < data.totallv)
		{
			data.lvValues[curLv] = key;
		}
		else 
		{
			if (lua_istable(L, -1) )
			{
				if (!splitsubtable)
				{
					data.setData(key, printTable(L));
				}
// 				key = CGMISC::toString("%s_%s", key_prefix.c_str(), key_);
			}
			else
				data.setData(key, value ? value : "");
		}

		if (lua_istable(L, -1) && (splitsubtable || curLv < data.totallv))
		{
			traverse_table_andwrite(L, curLv + 1, data, newkeyprefix, splitsubtable);
		}

		lua_pop(L, 1);
		// 现在的栈：-1 => key; index => table

		if (curLv == data.totallv - 1)
		{
			data.newline();
// 			data.curline++;
		}
	}
	// 现在的栈：index => table （最后 lua_next 返回 0 的时候它已经把上一次留下的 key 给弹出了）
	// 所以栈已经恢复到进入这个函数时的状态
}

void parse_lua_table_andwrite(lua_State* L, const char* outname, bool splitsubtable /*= true*/)
{
	int level = calcTableLevels(L);

	TableData data;
	for (int i = 0; i < level; ++i)
	{
		std::string key = CGMISC::toString("__type%d", i + 1);
		data.desc.push_back(key);
		data.keymap[key] = i;
	}

	if (level < 0)
	{
		printf("failed: config level < 0\n");
		return;
	}

	data.totallv = level;
	data.curline = 0;
	data.lvValues.resize(level);

	traverse_table_andwrite(L, 0, data, "", splitsubtable);

	data.write(outname);
}
