#include "search_engine.h"
#include "limonp/Md5.hpp"
#include <algorithm>
#include <fstream> 


extern cppjieba::Jieba g_jieba;

float policy_jisuan_score(std::string &query,std::vector<std::string> & term_list,Json::Value & query_json,Json::Value & one_info,int search_mode)
{
	float ret = DEFAULT_SCORE;
	if (one_info.empty() || 
			one_info["cmd_info"].isNull () ||
			one_info["cmd_info"]["title4se"].isNull() ) {return ret;}

	if (search_mode == or_mode)
	{
		int or_terms_length = 0;

		if (!one_info["cmd_info"]["term4se"].isNull()
				&& one_info["cmd_info"]["term4se"].isArray()
				&& one_info["cmd_info"]["term4se"].size() > 0)
		{
#ifdef _USE_HASH_
			std::tr1::unordered_set<std::string> term_hash;
			std::tr1::unordered_set<std::string>::iterator term_hash_it;
#else
			std::set<std::string> term_hash;
			std::set<std::string>::iterator term_hash_it;
#endif

			for (unsigned int i = 0; i < one_info["cmd_info"]["term4se"].size(); i++)
			{
				term_hash.insert(one_info["cmd_info"]["term4se"][i].asString());
			}

			for (int i = 0 ; i < term_list.size();i++)
			{
				term_hash_it = term_hash.find(term_list[i]);
				if (term_hash_it != term_hash.end()) {or_terms_length += term_list[i].size();}
			}
		}

		ret = or_terms_length * (float)1.0 / one_info["cmd_info"]["title4se"].asString().size();
	}
	else
	{
		ret = (float)1.0 / one_info["cmd_info"]["title4se"].asString().size();
	}
	ret = (ret >= 1.0f) ? 1.0f : ret;
	return ret;
}

void policy_cut_query(cppjieba::Jieba &jieba,std::string & query,std::vector<std::string> &term_list)
{
	if (query.size() <= 0) {return;}
	std::vector<cppjieba::Word> jiebawords;
	jieba.CutForSearch(query, jiebawords, true);
	//clear
	term_list.clear();
	for (int i = 0;i < jiebawords.size();i++)
	{
		term_list.push_back(jiebawords[i].word);
	}
}

////////////////////////////////////////////////////////////////////

class sort_myclass {
	public:
		sort_myclass(uint32_t a, float b):pos(a), score(b){}
		uint32_t pos;
		float score;

		bool operator < (const sort_myclass &m)const {
			return score >= m.score;
		}
};

void merge_sort_2way(std::vector<uint32_t> & in_way1,std::vector<uint32_t> & in_way2,std::vector<uint32_t> & out_way)
{
	out_way.clear();
	for (int i = 0,j =0;;)
	{
		if (i >= in_way1.size() && j >= in_way2.size())
		{
			break;
		}
		else if (i >= in_way1.size())
		{//push in_way2
			for (;j<in_way2.size();j++) 
			{
				if (out_way.size() <= 0 || in_way2[j] != out_way.back())
				{out_way.push_back(in_way2[j]);}
			}
			break;
		}
		else if (j >= in_way2.size())
		{//push in_way1
			for (;i<in_way1.size();i++) 
			{
				if (out_way.size() <= 0 || in_way1[i] != out_way.back())
				{out_way.push_back(in_way1[i]);}
			}
			break;
		}

		if (in_way1[i] < in_way2[j])
		{
			if (out_way.size() <= 0 ||
					in_way1[i] != out_way.back())
			{
				out_way.push_back(in_way1[i]);
			}
			i++;
		}
		else
		{
			if (out_way.size() <= 0 ||
					in_way2[j] != out_way.back())
			{
				out_way.push_back(in_way2[j]);
			}
			j++;
		}

	}

}


bool Search_Engine::add(std::vector<std::string> & term_list,Json::Value & one_info)
{
	bool ret = false;
	//check if validate
	if (term_list.size() <= 0 || one_info.empty()) {return false;}
	//jisuan md5
	std::string json_str = json_writer.write(one_info);
	std::string md5_str;
	uint32_t index_num = 0;
	limonp::md5String(json_str.c_str(),md5_str);
	//add info_md5
	{// enter lock
		AUTO_LOCK auto_lock(&_info_md5_dict_lock,true);
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,uint32_t>::iterator it = _info_md5_dict.find(md5_str);
#else
		std::map<std::string,uint32_t>::iterator it = _info_md5_dict.find(md5_str);
#endif
		//check if add
		if (it != _info_md5_dict.end())
		{
			return true;		
		}
		else
		{
			index_num = ++max_index_num;
			_info_md5_dict.insert ( std::pair<std::string,uint32_t>(md5_str,index_num) );
		}
	}
	//add info
	{// enter lock std::map<uint32_t,Json::Value> _info_dict;
		AUTO_LOCK auto_lock(&_info_dict_lock,true);
		_info_dict.insert ( std::pair<uint32_t,Json::Value>(index_num,one_info) );
	}
	//add index
	{
		ret = true;
		for (int i = 0 ;i < term_list.size(); i++)
		{
			if( _index_core.insert_index(term_list[i],index_num) != true)
			{ret = false;}

			//check if need shrink
			index_hash_value i_hash_value = _index_core.find_index(term_list[i]);
			if (i_hash_value.sum_node_num >= DEFAULT_ADD_NEED_SHRINK_NODE &&
					(i_hash_value.use_data_num / i_hash_value.sum_node_num) < DEFAULT_ADD_NEED_SHRINK_AVG) 
			{
				_index_core.shrink_index(term_list[i]);
			}
		}
	}
	return ret;
}

bool Search_Engine::del(std::vector<std::string> & term_list,Json::Value & one_info)
{
	bool ret = false;
	uint32_t index_num = 0;
	//check if validate
	if (term_list.size() <= 0 || one_info.empty()) {return false;}
	//jisuan md5
	std::string json_str = json_writer.write(one_info);
	std::string md5_str;
	limonp::md5String(json_str.c_str(),md5_str);
	//add info_md5
	{// enter lock
		AUTO_LOCK auto_lock(&_info_md5_dict_lock,true);
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,uint32_t>::iterator it = _info_md5_dict.find(md5_str);
#else
		std::map<std::string,uint32_t>::iterator it = _info_md5_dict.find(md5_str);
#endif
		//check if del
		if (it != _info_md5_dict.end())
		{
			index_num = it->second;
			_info_md5_dict.erase(it);
		}
		else
		{
			return false;
		}
	}
	//del info
	{// enter lock std::map<uint32_t,Json::Value> _info_dict;
		AUTO_LOCK auto_lock(&_info_dict_lock,true);
		_info_dict.erase (index_num);
	}
	//del index
	{
		ret = true;
		for (int i = 0 ;i < term_list.size(); i++)
		{
			if( _index_core.delete_index(term_list[i],index_num) != true)
			{ret = false;}

			//check if need shrink
			index_hash_value i_hash_value = _index_core.find_index(term_list[i]);
			if (i_hash_value.del_data_num >= DEFAULT_DEL_NEED_SHRINK) 
			{
				_index_core.shrink_index(term_list[i]);
			}
		}
	}
	return ret;
}

bool Search_Engine::search(std::vector<std::string> & in_term_list,
		std::string & in_query,
		Json::Value & query_json,
		std::vector<Json::Value> &out_vec,
		int in_start_id,int in_ret_num,int in_max_ret_num,int search_mode)
{
	//check
	if (in_term_list.size() <= 0 ||
			in_query.size() <= 0 ||
			query_json.empty() ||
			in_start_id < 0 ||
			in_ret_num <= 0 ||
			in_max_ret_num <= 0)
	{return false;}

	//select a term which has min index numbers
	int term_pos = 0;
	int min_index_numbers = 0;
	if (search_mode == and_mode)
	{
		for (int i = 0;i < in_term_list.size(); i++)
		{
			index_hash_value i_hash_value = _index_core.find_index(in_term_list[i]);

			if (i_hash_value.sum_node_num <= 0) 
			{//this term not exist index
				return true;
			}

			if (i == 0)
			{//init
				term_pos = i;
				min_index_numbers = i_hash_value.use_data_num;
			}
			else
			{
				if (i_hash_value.use_data_num < min_index_numbers)
				{
					term_pos = i;
					min_index_numbers = i_hash_value.use_data_num;
				}
			}
		}
	}
	//get recall index numbers vector
	std::vector<uint32_t> query_in;
	std::vector<uint32_t> query_out;
	std::vector<uint32_t> & query_in_it = query_in;
	std::vector<uint32_t> & query_out_it = query_out;

	_index_core.all_query_index(in_term_list[term_pos],query_in_it);

	if (search_mode == and_mode)
	{
		for (int i = 0;i < in_term_list.size(); i++)
		{
			if (i == term_pos) {continue;}
			_index_core.cross_query_index(in_term_list[i],query_in_it,query_out_it);

			std::vector<uint32_t> & tmp_it = query_in_it;
			query_in_it = query_out_it;
			query_out_it = tmp_it;
			query_out_it.clear();
		}
	}
	else
	{
		for (int i = 0;i < in_term_list.size(); i++)
		{
			std::vector<uint32_t>  tmp_vec;
			if (i == term_pos) {continue;}

			_index_core.all_query_index(in_term_list[i],tmp_vec);
			merge_sort_2way(query_in_it,tmp_vec,query_out_it);

			std::vector<uint32_t> & tmp_it = query_in_it;
			query_in_it = query_out_it;
			query_out_it = tmp_it;
			query_out_it.clear();

		}

	}


	//check in_start_id and in_ret_num
	if (in_start_id >= query_in_it.size())
	{return false;}
	if (in_ret_num >= in_max_ret_num)
	{in_ret_num = in_max_ret_num;}

	//jisuan every index score,and get the need number vector,can use min heap sort
	std::vector< sort_myclass > vect_score;

	{
		AUTO_LOCK auto_lock(&_info_dict_lock,false);

		for (int i = 0; i < query_in_it.size(); i++)
		{
#ifdef _USE_HASH_
			std::tr1::unordered_map<uint32_t,Json::Value>::iterator it = _info_dict.find(query_in_it[i]);
#else
			std::map<uint32_t,Json::Value>::iterator it = _info_dict.find(query_in_it[i]);
#endif
			if (it != _info_dict.end())
			{
				float score = policy_jisuan_score(in_query,in_term_list,query_json,it->second,search_mode);
				sort_myclass my(query_in_it[i], score);
				vect_score.push_back(my);
			}
			else
			{
				//not find,default score is 0.0
				sort_myclass my(query_in_it[i], DEFAULT_SCORE);
				vect_score.push_back(my);
			}

		}
	}

	//sort
	std::sort(vect_score.begin(), vect_score.end()); 

	//out to out_vec
	{
		AUTO_LOCK auto_lock(&_info_dict_lock,false);
		for (int i = in_start_id;i < in_start_id + in_ret_num && i < vect_score.size();i++)
		{
#ifdef _USE_HASH_
			std::tr1::unordered_map<uint32_t,Json::Value>::iterator it = _info_dict.find(vect_score[i].pos);
#else
			std::map<uint32_t,Json::Value>::iterator it = _info_dict.find(vect_score[i].pos);
#endif
			if (it != _info_dict.end())
			{
				out_vec.push_back(it->second["show_info"]);
				//dump score to json
				out_vec.back()["comse_score"] = vect_score[i].score;
			}
		}
	}
	return true;
}

bool Search_Engine::dump_to_file()
{
	std::ofstream os;  
	os.open(_dump_file.c_str());

	if (!os.is_open())
	{
		return false;
	}
	else
	{
		{
			AUTO_LOCK auto_lock(&_info_dict_lock,false);
#ifdef _USE_HASH_
			for (std::tr1::unordered_map<uint32_t,Json::Value>::iterator it = _info_dict.begin(); it != _info_dict.end(); ++it)
#else
				for (std::map<uint32_t,Json::Value>::iterator it = _info_dict.begin(); it != _info_dict.end(); ++it)
#endif
				{
					os << json_writer.write(it->second);  
				}
		}
		os.close();  
		return true;
	}
}

bool Search_Engine::load_from_file()
{
	std::ifstream is;  
	is.open(_load_file.c_str());
	char in_buff[MAX_GETLINE_BUFF] = {0};

	if (!is.is_open())
	{
		return false;
	}

	while (!is.eof() )  
	{  
		is.getline(in_buff,MAX_GETLINE_BUFF);
		Json::Value tmp_json;

		//parse use json
		if (!json_reader.parse(in_buff,(char *)in_buff + strlen(in_buff), tmp_json)) 
		{
			continue;
		}

		//parse json ok 
		bool is_term_list_in = false;
		std::vector<std::string> term_list;

		if ( tmp_json["cmd_type"].isNull()
				|| tmp_json["cmd_info"].isNull() 
				|| tmp_json["show_info"].isNull() 
				|| tmp_json["cmd_info"]["title4se"].isNull() )
		{
			continue;
		}

		if (!tmp_json["cmd_info"]["term4se"].isNull() 
				&& tmp_json["cmd_info"]["term4se"].isArray()
				&& tmp_json["cmd_info"]["term4se"].size() > 0)
		{
			is_term_list_in = true;
			for (unsigned int i = 0; i < tmp_json["cmd_info"]["term4se"].size(); i++)
			{
				term_list.push_back(tmp_json["cmd_info"]["term4se"][i].asString());
			}
		}

		//parse query
		std::string query = tmp_json["cmd_info"]["title4se"].asString();

		//parse cmd_type
		if (tmp_json["cmd_type"].asString() == "add")
		{
			if (!is_term_list_in)
			{//get term_list
				policy_cut_query(g_jieba,query,term_list);
				//add term in json_in
				Json::Value term_list_json;
				for (int i = 0;i < term_list.size();i++)
				{term_list_json.append(term_list[i]);}

				tmp_json["cmd_info"]["term4se"] = term_list_json;
			}

			if (add(term_list,tmp_json))
			{
				//return true
			}
			else
			{
				//return false
			}

		}
		else
		{continue;}
	}

	return true;
}
