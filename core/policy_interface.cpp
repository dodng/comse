#include "policy_interface.h"
#include "cppjieba/Jieba.hpp"
#include "search_engine.h"

////////////////////////////////
//notice multithread is safe ?

const char* const DICT_PATH = "./cppjieba/dict/jieba.dict.utf8";
const char* const HMM_PATH = "./cppjieba/dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "./cppjieba/dict/user.dict.utf8";
const char* const IDF_PATH = "./cppjieba/dict/idf.utf8";
const char* const STOP_WORD_PATH = "./cppjieba/dict/stop_words.utf8";

cppjieba::Jieba g_jieba(DICT_PATH,
		HMM_PATH,
		USER_DICT_PATH,
		IDF_PATH,
		STOP_WORD_PATH);

Json::Reader g_json_reader;
Json::FastWriter g_json_writer; 
Search_Engine g_search_engine;

////////////////////////////////

//true return 0,other return > 0
int policy_entity::parse_in_json()
{
	if (0 == it_http) {return 1;}
	if (1 != it_http->parse_done())	{return 2;}

	std::map<std::string,std::string>::iterator it;
//find data field
	it = it_http->body_map.find("data");
	if (it == it_http->body_map.end()) {return 3;}

	std::string data_str = url_decode(it->second);
//parse use json
	if (g_json_reader.parse(data_str, json_in)) 
	{
		return 0;
	}
	return 5;
}

//true return 0,other return > 0
int policy_entity::get_out_json()
{
	int ret = -1;
	bool is_term_list_in = false;
	std::vector<std::string> term_list;

	//parse json check validation
	if ( json_in["cmd_type"].isNull()
	|| json_in["cmd_info"].isNull() 
	|| json_in["show_info"].isNull() 
	|| json_in["cmd_info"]["title4se"].isNull() )
	{
		ret = 1;
		json_out["ret_code"] = ret;
		return ret;
	}

	if (!json_in["cmd_info"]["term4se"].isNull() && json_in["cmd_info"]["term4se"].size() > 0)
	{
		is_term_list_in = true;
		for (unsigned int i = 0; i < json_in["cmd_info"]["term4se"].size(); i++)
		{
			term_list.push_back(json_in["cmd_info"]["term4se"][i].asString());
		}
	}

	//parse query
	std::string query = json_in["cmd_info"]["title4se"].asString();

	//parse cmd_type
	if (json_in["cmd_type"].asString() == "add")
	{
		if (!is_term_list_in)
		{//get term_list
			policy_cut_query(g_jieba,query,term_list);
		}
		if (g_search_engine.add(term_list,json_in["show_info"]))
		{
			//return true
			ret = 0;
		}
		else
		{
			//return false
			ret = 100;
		}

	}
	else if (json_in["cmd_type"].asString() == "del" )
	{
		if (!is_term_list_in)
		{//get term_list
			policy_cut_query(g_jieba,query,term_list);
		}
		if (g_search_engine.del(term_list,json_in["show_info"]))
		{
			//return true
			ret = 0;
		}
		else
		{
			//return false
			ret = 200;
		}
	}
	else if (json_in["cmd_type"].asString() == "search" )
	{
		int start_id = 0;
		int ret_num = 20;
		std::vector<Json::Value> out_vec;

		//parse start_id and ret_num
		if (!json_in["cmd_info"]["start_id"].isNull()) {start_id = json_in["cmd_info"]["start_id"].asInt();}
		if (!json_in["cmd_info"]["ret_num"].isNull()) {ret_num = json_in["cmd_info"]["ret_num"].asInt();}

		if (!is_term_list_in)
		{//get term_list
			policy_cut_query(g_jieba,query,term_list);
		}
		if (g_search_engine.search(term_list,query,out_vec,start_id,ret_num))
		{
			//return true
			ret = 0;
			//cook return data
			Json::Value arrayObj;
			for (int i = 0;i < out_vec.size();i++)
			{
				arrayObj.append(out_vec[i]);
			}
			json_out["ret_array"] = arrayObj;
		}
		else
		{
			//return false
			ret = 300;
		}
	}
	else
	{
		ret = 400;
	}


	json_out["ret_code"] = ret;
	return ret;
}

//true return 0,other return > 0
int policy_entity::cook_senddata(char *send_buff_p,int buff_len,int &send_len)
{
	if (0 == it_http || 0 == send_buff_p) {return 1;}
	if (buff_len <= 0 || send_len < 0) {return 3;}

	std::string json_str = g_json_writer.write(json_out);
	if (json_str.size() <= 0) {return 2;}

	int cook_len = json_str.size() < buff_len ? json_str.size(): buff_len;
	memcpy(send_buff_p,json_str.c_str(),cook_len);
	send_len = cook_len;
	return 0;

}

std::string policy_entity::print_all()
{
	std::string rst;
	rst += "json in\n";
	rst += json_in.toStyledString();
	rst += "json out\n";
	rst += json_out.toStyledString();
	return rst;

}
