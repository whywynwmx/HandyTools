#include "JsonConfig.h"
#include "CJson/cJSON.h"
#include <fstream>
#include <streambuf>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "string_common.h"

#include <Windows.h>
#include <io.h>
#include <direct.h>

bool usejsonname = true;

using namespace std;
struct JsonTableData
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

void doCaclJsonLevels(cJSON* jsonfile, int curlevel, int& outLv)
{
// 	if (!lua_istable(L, -1))
// 		return;
// 	int nIndex = lua_gettop(L);  // 取 table 索引值 
// 	lua_pushnil(L);
// 	while (lua_next(L, nIndex))
// 	{
// 		if (lua_istable(L, -1))
// 		{
// 			int newIndex = lua_gettop(L);  // 取 table 索引值 
// 			doCaclTableLevels(L, curlevel + 1, outLv);
// 		}
// 		else
// 		{
// 			if (outLv == -1 || curlevel < outLv)
// 			{
// 				outLv = curlevel;
// 			}
// 		}
// 
// 		lua_pop(L, 1);
// 	}

	auto* child = jsonfile->child;
	while (child)
	{
		if (child->child)
		{
			doCaclJsonLevels(child, curlevel + 1, outLv);
		}
		else
		{
			if (outLv == -1 || curlevel < outLv)
			{
				outLv = curlevel;
			}
		}

		child = child->next;
	}
}

int calc_json_table_grade(cJSON* jsonfile) {
	int outLv = -1;
	doCaclJsonLevels(jsonfile, 0, outLv);

	return outLv;
}

void traverse_table_andwrite(cJSON* jsonfile, int curLv, JsonTableData& data, const std::string& key_prefix = "", bool splitsubtable = true)
{
	bool isArray = jsonfile->type == cJSON_Array;
	int idx = 0;

	cJSON* child = jsonfile->child;
	while (child)
	{
		std::string key_ = "";
		if (child->string)
			key_ = child->string;
		else
			key_ = CGMISC::toString(idx);
		std::string key = key_;
		std::string value = "";
		if (child->type == cJSON_Number)
		{
			if (child->valueint != child->valuedouble)
			{
				value = CGMISC::toString("%f", child->valuedouble);
			}
			else
				value = CGMISC::toString("%d", child->valueint);
		}
		else if (child->type == cJSON_String)
			value = child->valuestring;
		child->valuestring;

		if (curLv > data.totallv)
		{
			key = CGMISC::toString("%s_%s", key_prefix.c_str(), key_.c_str());
		}

		std::string newkeyprefix = key_prefix;
		if (curLv >= data.totallv)
		{
			newkeyprefix = CGMISC::toString("%s_%s", key_prefix.c_str(), key_.c_str());
		}

		if (curLv < data.totallv)
		{
			data.lvValues[curLv] = key;
		}
		else
		{
			if (child->child)
			{
				if (!splitsubtable)
				{
					data.setData(key, cJSON_PrintUnformatted(child/*->child*/));
				}
			}
			else
				data.setData(key, value);
		}

		if (child->child && (splitsubtable || curLv < data.totallv))
		{
			traverse_table_andwrite(child, curLv + 1, data, newkeyprefix, splitsubtable);
		}

		if (curLv == data.totallv - 1)
		{
			data.newline();
		}

		child = child->next;
		++idx;
	}
}

void parse_json_table(cJSON* jsonfileroot, const char* outname1, bool splitsubtable = true)
{
	std::string outname = outname1;

	cJSON* jsonfile = jsonfileroot->child;
	while (jsonfile)
	{
		if(usejsonname)
			outname = std::string(outname1) + jsonfile->string + ".xls";

		int level = calc_json_table_grade(jsonfile);

		JsonTableData data;
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

		traverse_table_andwrite(jsonfile, 0, data, "", splitsubtable);

		data.write(outname.c_str());

		jsonfile = jsonfile->next;
	}
}

void parse_json_andwrite(const char* jsonfile, const char* outname, bool splitsubtable/* = true*/)
{
	char* filebuf = NULL;
	auto* f =  fopen(jsonfile, "rt");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		int len = ftell(f);
		filebuf = new char[len + 1];
		fseek(f, 0, SEEK_SET);
		int redlen = fread((void*)filebuf, 1, len, f);
		filebuf[redlen] = 0;

		fclose(f);
	}
	else
	{
		printf("bad json file....");
	}

	cJSON* root = cJSON_Parse(filebuf);
	std::string fname = outname;
	fname += "\\";
	parse_json_table(root, fname.c_str(), splitsubtable);
/*	auto* child = root->child;
	while (child)
	{
		std::string fname = outname;
		fname += "\\";
		fname += child->string;
		fname += ".xls";
		parse_json_table(child, fname.c_str(), splitsubtable);

		child = child->next;
	}*/
}

void foreachFolerJson(const std::string& srcPath, const std::string& outPath, bool splitsubtable/* = true*/)
{
/*	splitsubtable = false;*/
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

					foreachFolerJson(newPath, newOutputPath);
				}
			}
			else
			{
				if (strstr(filefind.name, "json"))
				{
					printf("parase file: %s....\n", filefind.name);

					char* filebuf = NULL;
					std::string fullpath = srcPath + "\\" + filefind.name;
					auto* f = fopen(fullpath.c_str(), "rt");
					if (f)
					{
						fseek(f, 0, SEEK_END);
						int len = ftell(f);
						filebuf = new char[len + 1];
						fseek(f, 0, SEEK_SET);
						int redlen = fread((void*)filebuf, 1, len, f);
						filebuf[redlen] = 0;

						fclose(f);
					}
					else
					{
						printf("bad json file....");
					}
					
					cJSON* root = cJSON_Parse(filebuf);
					if (root) {
						std::string outexlname = outPath + "\\" +filefind.name + ".xls";
						if (usejsonname)
							outexlname = outPath + "\\" + std::string(filefind.name).substr(0, std::string(filefind.name).find(".")) + "_";
						parse_json_table(root, outexlname.c_str(), splitsubtable);
					}

					delete[] filebuf;
				}
			}
		}
	}
	_findclose(handle);
}

