#ifndef COMSE_SEARCH_ENGINE_H_
#define COMSE_SEARCH_ENGINE_H_

#include <string>

#ifdef _USE_HASH_
#include <tr1/unordered_map>
#else
#include <map>
#endif

#include "json/json.h"
#include "index_core.h"
#include <pthread.h>
#include <stdint.h> 
#include <vector>
#include "cppjieba/Jieba.hpp"

/////

float policy_jisuan_score(std::string &query,std::vector<std::string> & term_list,Json::Value & one_info);
void policy_cut_query(cppjieba::Jieba &jieba,std::string & query,std::vector<std::string> &term_list);

/////

class Search_Engine{
	public:
		Search_Engine(char *p_file_dump_file = 0,char * p_file_load_file = 0)
		{
			//lock init
			pthread_rwlock_init(&_info_dict_lock,NULL);	
			pthread_rwlock_init(&_info_md5_dict_lock,NULL);
			max_index_num = 0;
			//init dump and load file
			if (0 != p_file_dump_file)
			{_dump_file = p_file_dump_file;}

			if (0 != p_file_load_file)
			{
				_load_file = p_file_load_file;
				load_from_file();
			}	
		}
		~Search_Engine()
		{
			//lock destroy
			pthread_rwlock_destroy(&_info_dict_lock);
			pthread_rwlock_destroy(&_info_md5_dict_lock);

		}
		bool add(std::vector<std::string> & term_list,Json::Value & one_info);
		bool del(std::vector<std::string> & term_list,Json::Value & one_info);
		bool search(std::vector<std::string> & in_term_list,
				std::string & in_query,
				std::vector<Json::Value> &out_vec,
				int in_start_id = 0,int in_ret_num = 20,int in_max_ret_num = 40);
		bool dump_to_file();
		bool load_from_file();
	private:
		//index
		Index_Core _index_core;
#ifdef _USE_HASH_
		std::tr1::unordered_map<uint32_t,Json::Value> _info_dict;
		std::tr1::unordered_map<std::string,uint32_t> _info_md5_dict;
#else
		std::map<uint32_t,Json::Value> _info_dict;
		std::map<std::string,uint32_t> _info_md5_dict;
#endif
		pthread_rwlock_t _info_dict_lock;
		pthread_rwlock_t _info_md5_dict_lock;
		//json
		Json::Reader json_reader;
		Json::FastWriter json_writer;
		//data
		uint32_t max_index_num;
		//file
		std::string _dump_file;
		std::string _load_file;

};
#endif
