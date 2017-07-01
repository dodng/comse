#ifndef QUERY_ANALYSE_SEARCH_ENGINE_H_
#define QUERY_ANALYSE_SEARCH_ENGINE_H_

#include <string>
#include <set>
#include <map>
#include <math.h>

class Query_Analyse{
	public:
		Query_Analyse()
		{
			pthread_rwlock_init(&_term_dict_lock,NULL);
			query_num = 1;
		}
		~Query_Analyse()
		{
			pthread_rwlock_destroy(&_term_dict_lock);
		}
		bool insert_query(std::vector<std::string> & term_list)
		{
			if (term_list.size() <= 0) {return false;}

			//1. vector transform to set to delete repeat term_string
			std::set<std::string> set_tmp;
			for (int i = 0;i < term_list.size();i++)
			{set_tmp.insert(term_list[i]);}

			pthread_rwlock_wrlock(&_term_dict_lock);

			//2. update term_dict and query_num
			for (std::set<std::string>::iterator it=set_tmp.begin(); it!=set_tmp.end(); ++it)
			{
#ifdef _USE_HASH_
				std::tr1::unordered_map<std::string,uint32_t>::iterator it2 = _term_dict.find(*it);
#else
				std::map<std::string,uint32_t>::iterator it2 = _term_dict.find(*it);
#endif
				if (it2 != _term_dict.end())//find
				{it2->second += 1;}
				else
				{_term_dict.insert ( std::pair<std::string,uint32_t>(*it,1));}
			}
			query_num += 1;

			pthread_rwlock_unlock(&_term_dict_lock);

			return true;
		}

		float get_term_score(std::string term_str)
		{
			float ret = 0.0f;
			pthread_rwlock_rdlock(&_term_dict_lock);
#ifdef _USE_HASH_
			std::tr1::unordered_map<std::string,uint32_t>::iterator it2 = _term_dict.find(term_str);
#else
			std::map<std::string,uint32_t>::iterator it2 = _term_dict.find(*it);
#endif
#if 0
			if (it2 != _term_dict.end())//find
			{
				ret = (float)log(query_num / it2->second);
				if (ret <= 0.0f)
				{ret = 0.0f;}
			}
#endif
#if 0
			if (it2 != _term_dict.end())//find
			{
				ret =  term_str.size() * (1.0f *(query_num - it2->second)) / query_num;
				if (ret <= 0.0f)
				{ret = 0.0f;}
			}
#endif		
			if (it2 != _term_dict.end())//find
			{
				//				ret =  term_str.size() * (1.0f *(query_num - it2->second)) / query_num;
				ret =  (1.0f *(query_num - it2->second)) / query_num;
				if (ret <= 0.0f)
				{ret = 0.0f;}
				else
				{ret *= 10;ret = ret * ret;}//improve high score,to use x*x so + 1.0f
			}
			pthread_rwlock_unlock(&_term_dict_lock);
			return ret;
		}
	private:
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,uint32_t> _term_dict;
#else
		std::map<std::string,uint32_t> _term_dict;
#endif
		pthread_rwlock_t _term_dict_lock;
		uint32_t query_num;
};
#endif
