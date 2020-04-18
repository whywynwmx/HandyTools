#ifndef JSONCONFIG_H
#define JSONCONFIG_H

#include <string>

void parse_json_andwrite(const char* jsonfile, const char* outname, bool splitsubtable = true);

void foreachFolerJson(const std::string& srcPath, const std::string& outPath, bool splitsubtable = true);


#endif
