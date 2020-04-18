#include <fstream>
#include <streambuf>
#include <iostream>
#include <string>
#include <Windows.h>
#include <io.h>
#include <direct.h>
#include <algorithm>

#include "lua.h"
#include "LuaConfig.h"
#include "JsonConfig.h"

using namespace std;

void write_to_excel(string filename, bool&flag);

#define INVALID -1

static void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
	(void)ud;
	(void)osize;
	if (nsize == 0) {
		free(ptr);
		return NULL;
	}
	else
	{
		return realloc(ptr, nsize);
	}
}

string& replace_str(string& str, const string& to_replaced, const string& newchars)
{
	for (string::size_type pos(0); pos != string::npos; pos += newchars.length())
	{
		pos = str.find(to_replaced, pos);
		if (pos != string::npos)
			str.replace(pos, to_replaced.length(), newchars);
		else
			break;
	}
	return   str;
}

void foreachFoler(lua_State* L, const std::string& srcPath, const std::string& outPath)
{
	if (0 != access(outPath.c_str(), 0))
		mkdir(outPath.c_str());

	std::string  curr = srcPath + std::string("\\*");
	int done = 0, handle = 0;
	_finddata_t filefind;
	if ((handle = _findfirst(curr.c_str(), &filefind)) != -1)
	{
		while (!(done = _findnext(handle, &filefind)))
		{
			if (filefind.attrib & _A_SUBDIR)
			{
				if ((strcmp(filefind.name, ".") != 0) && (strcmp(filefind.name, "..") != 0))
				{
					std::string newPath = srcPath + "\\" + filefind.name;
					std::string newOutputPath = outPath + "\\" + filefind.name;
					if (0 != access(newOutputPath.c_str(), 0))
						mkdir(newOutputPath.c_str());

					foreachFoler(L, newPath, newOutputPath);
				}
			}
			else
			{
				if (strstr(filefind.name, ".config"))
				{
					std::string fullpath = srcPath + "\\" + filefind.name;
					printf("process config: %s\n", fullpath.c_str());
					std::string tableName;
					FILE* f = fopen(fullpath.c_str(), "rb");
					if (f)
					{
						char buff[256] = { 0 };
						fgets(buff, 256, f);

						if (strlen(buff) > 2 && strstr(buff, "--") == NULL)
						{
							std::string s = buff;
							tableName = s.substr(0, s.find_first_of("="));
						}
						else
						{
							fgets(buff, 256, f);
							std::string s = buff;
							tableName = s.substr(0, s.find_first_of("="));
						}

						int idx = tableName.size() - 1;
						for (; idx >=0; --idx)
						{
							if (tableName[idx] == ' ')
							{
								tableName.pop_back();
							}
							else
								break;
						}

						fclose(f);
					}

//					tableName = "TianTiRobotConfig";
					int rt = luaL_loadfile(L, fullpath.c_str());
					bool bRet = lua_pcall(L, 0, 0, 0);
					if (bRet)
					{
						printf("run lua failed\n");
					}
					else
					{
						lua_getglobal(L, tableName.c_str());
						if (!lua_istable(L, -1))
							printf("get lua table failed\n");
						else
						{
							std::string outexlname = outPath + "\\" + filefind.name + ".xls";
							parse_lua_table_andwrite(L, outexlname.c_str(), true);
						}
						lua_pop(L, 1);
					}
				}
			}
		}
	}
	_findclose(handle);
}

void specialForItemConfig(lua_State* L, const std::string& srcPath, const std::string& outputPath, const std::string& jsonPath)
{
	char* buf = new char[0x2b0000];			//for item.config
	std::string filename = "item.config";
	std::string fullpath = srcPath + "\\" + filename;
	printf("process config: %s\n", fullpath.c_str());
	std::string tableName;
	FILE* f = fopen(fullpath.c_str(), "rb");
	if (f)
	{
		int ttttcount = fread(buf, 1, 0x2ac7a2, f);
		buf[ttttcount] = 0;

		fclose(f);
	}

	std::string sss = buf;
	replace_str(sss, "\\n", "@@@@@@@@@");

	// 					int rt = luaL_loadfile(L, fullpath.c_str());
	int rt = luaL_loadstring(L, sss.c_str());
	bool bRet = lua_pcall(L, 0, 0, 0);
	if (bRet)
	{
		printf("run lua failed\n");
	}
	else
	{
		tableName = "ItemConfig";
		lua_getglobal(L, tableName.c_str());
		if (!lua_istable(L, -1))
			printf("get lua table failed\n");
		else
		{
			std::string outexlname = outputPath + "\\" + filename  + ".xls";
			parse_lua_table_andwrite(L, outexlname.c_str(), false);
		}
		lua_pop(L, 1);
	}

}

int main(int argc, char**argv) {
// 	bool flag = false;
// 	write_to_excel("test2.xls", flag);
// 
// 	lua_State* L = luaL_newstate();
// 
// 	const char* luacfgname = "LeiTingEquipLevel.config";
// 	if (argv > 1)
// 		luacfgname = argc[1];
// 
// 	int rt = luaL_loadfile(L , luacfgname);
// 
// 	bool bRet = lua_pcall(L, 0, 0, 0);
// 	if (bRet)
// 	{
// 		cout << "pcall error" << endl;
// 		return 0;
// 	}
// 
// 	lua_getglobal(L, "LeiTingEquipLevelConfig");
// // 	parse_lua_table(L);
// 	parse_lua_table_andwrite(L, "out_eq.xls");
// 
// 	system("pause");

	if (argc < 3)
	{
		printf("usage: ExcelImporter [srcPath] [outPath]");
	}

	lua_State* L = luaL_newstate();

	std::string srcPath = argv[1];
	std::string destPath = argv[2];
	std::string jsonPath = "";
	if (argc > 3)
		jsonPath = argv[3];
	foreachFoler(L, srcPath, destPath);

// 	specialForItemConfig(L, srcPath, destPath, jsonPath);
	system("pause");

	return 0;
}

// int main(int argc, char**argv) {
// 	if (argc < 3)
// 	{
// 		printf("usage: ExcelImporter [srcPath] [outPath]");
// 	}
// 
// 	std::string srcPath = argv[1];
// 	std::string destPath = argv[2];
// 	std::string jsonPath = "";
// 
//  	foreachFolerJson(srcPath.c_str(), destPath.c_str(), true);
// 
// //	parse_json_andwrite(srcPath.c_str(), destPath.c_str(), true);
// 
// 	system("pause");
// 
// 	return 0;
// }


void write_to_excel(string filename, bool&flag)
{
	//定义文件输出流   
	ofstream oFile;

	//打开要输出的文件   
	oFile.open(filename, ios::out | ios::trunc);    // 这样就很容易的输出一个需要的excel 文件  
	oFile << "姓名" << "\t" << "年龄" << "\t" << "班级" << "\t" << "班主任" << endl;
	oFile << "张三" << "\t" << "22" << "\t" << "1" << "\t" << "JIM" << endl;
	oFile << "李四" << "\t" << "23" << "\t" << "3" << "\t" << "TOM" << endl;

	oFile.close();


	//打开要输出的文件   
	ifstream iFile(filename);
	string   readStr((std::istreambuf_iterator<char>(iFile)), std::istreambuf_iterator<char>());
	cout << readStr.c_str();

	flag = 1;
}

