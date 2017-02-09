#include <string>
#include "json/json.h"
#include <iostream>
#include <stdio.h>

using namespace std;

void readJson();
void writeJson();

int main(int argc, char** argv) {
	readJson();
	writeJson();
	return 0;
}

void readJson() {
	using namespace std;
	std::string strValue = "{\"name\":\"json\",\"array\":[{\"cpp\":\"jsoncpp\"},{\"java\":\"jsoninjava\"},{\"php\":\"support\"}]}";

	Json::Reader reader;
	Json::Value value;

	if (reader.parse(strValue, value))
	{
		std::string out = value["name"].asString();
		std::cout << out << std::endl;
		const Json::Value arrayObj = value["array"];
		for (unsigned int i = 0; i < arrayObj.size(); i++)
		{
			if (!arrayObj[i].isMember("cpp")) 
				continue;
			out = arrayObj[i]["cpp"].asString();
			std::cout << out;
			if (i != (arrayObj.size() - 1))
				std::cout << std::endl;
		}
	}
}

void writeJson() {
	using namespace std;

	Json::Value root;
	Json::Value arrayObj;
	Json::Value item;

	item["cpp"] = "jsoncpp你好";
	item["java"] = "jsoninjava";
	item["php"] = "support";
	arrayObj.append(item);

	root["name"] = "json";
	root["array"] = arrayObj;
	
	if (!root["array1"].empty())
	{printf("1111\n");}
	else
	{printf("2222\n");}
//root["name2"].isNull();

	root.toStyledString();
	std::string out = root.toStyledString();
	std::cout << out << std::endl;
	Json::FastWriter writer;  
	std::string out2 = writer.write(root);  
	std::cout << out2 << std::endl;
	std::string out3 = writer.write(arrayObj);  
	std::cout << out3 << std::endl;
}
