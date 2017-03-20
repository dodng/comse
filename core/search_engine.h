/***************************************************************************
 * 
 *	author:dodng
 *	e-mail:dodng12@163.com
 * 	2017/3/16
 *   
 **************************************************************************/

#ifndef COMSE_SEARCH_ENGINE_H_
#define COMSE_SEARCH_ENGINE_H_

#include <string>

#ifdef _USE_HASH_
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#else
#include <map>
#include <set>
#endif

#include "json/json.h"
#include "index_core.h"
#include <pthread.h>
#include <stdint.h> 
#include <vector>
#include "cppjieba/Jieba.hpp"
#include <set>

/////

int policy_compute_score(std::string &query,std::vector<std::string> & term_list,Json::Value & query_json,Json::Value & one_info);
void policy_cut_query(cppjieba::Jieba &jieba,std::string & query,std::vector<std::string> &term_list);

enum search_mode
{
    and_mode = 0,
    or_mode = 1
};

#define INDEX_ONE_NODE_NUM (64)
#define INDEX_TERM_LOCK_NUM (128)
#define DEFAULT_SCORE (0)
#define DEFAULT_DEL_NEED_SHRINK (1024*2)
#define DEFAULT_ADD_NEED_SHRINK_AVG ((INDEX_ONE_NODE_NUM)*2)
#define DEFAULT_ADD_NEED_SHRINK_NODE (128)
#define MAX_GETLINE_BUFF (1024*1024)

/////
struct info_storage
{
	std::string title4se;
//	std::vector<std::string> term4se_vec;
	std::set<std::string> term4se_set;
	std::string info_json_ori_str;
};

/////

class Search_Engine{
	public:
		Search_Engine(char *p_file_dump_file = 0,char * p_file_load_file = 0):_index_core(INDEX_ONE_NODE_NUM,INDEX_TERM_LOCK_NUM)
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
				Json::Value & query_json,
				std::vector<std::string> & out_info_vec,
				std::vector<int> & out_score_vec,
				int &recall_num,
				int in_start_id = 0,int in_ret_num = 20,int in_max_ret_num = 40,int search_mode = and_mode);
		bool dump_to_file();
		bool load_from_file();
	private:
		//index
		Index_Core _index_core;
#ifdef _USE_HASH_
		std::tr1::unordered_map<uint32_t,info_storage> _info_dict;
		std::tr1::unordered_map<std::string,uint32_t> _info_md5_dict;
#else
		std::map<uint32_t,info_storage> _info_dict;
		std::map<std::string,uint32_t> _info_md5_dict;
#endif
		pthread_rwlock_t _info_dict_lock;
		pthread_rwlock_t _info_md5_dict_lock;
		//json
		Json::Reader json_reader;//just one thread,not need lock
		Json::FastWriter json_writer;//just one thread,not need lock
		//data
		uint32_t max_index_num;
		//file
		std::string _dump_file;
		std::string _load_file;

};
#endif
