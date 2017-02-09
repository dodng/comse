#ifndef COMSE_SEARCH_ENGINE_H_
#define COMSE_SEARCH_ENGINE_H_

#include <string>
#include <map>
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
		Search_Engine()
		{
			//lock init
			pthread_rwlock_init(&_info_dict_lock,NULL);	
			pthread_rwlock_init(&_info_md5_dict_lock,NULL);
			max_index_num = 0;	
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
	private:
		//index
		Index_Core _index_core;
		std::map<uint32_t,Json::Value> _info_dict;
		pthread_rwlock_t _info_dict_lock;
		std::map<std::string,uint32_t> _info_md5_dict;
		pthread_rwlock_t _info_md5_dict_lock;
		//json
		Json::Reader json_reader;
		Json::FastWriter json_writer;
		//data
		uint32_t max_index_num;

};
#endif
