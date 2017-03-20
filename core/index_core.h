/***************************************************************************
 * 
 *	author:dodng
 *	e-mail:dodng12@163.com
 * 	2017/3/16
 *   
 **************************************************************************/


#ifndef __INDEX_CORE_H_
#define __INDEX_CORE_H_

#include <stdint.h>

#ifdef _USE_HASH_
#include <tr1/unordered_map>
#else
#include <map>
#endif

#include <vector>
#include <string>
#include <pthread.h>

#define MAX_RECALL_NUM (100000)
#define MAX_INDEX_NUM  (200000)


///tools


class AUTO_LOCK
{
	public:
		AUTO_LOCK(pthread_rwlock_t *p_rwlock,bool is_wrlock)
		{
			if (0 == p_rwlock)
			{return;}
			p_lock_ = p_rwlock;

			if (is_wrlock)
			{
				pthread_rwlock_wrlock(p_lock_);
			}
			else
			{
				pthread_rwlock_rdlock(p_lock_);
			}
		}

		~AUTO_LOCK()
		{
			if (0 != p_lock_)
			{
				pthread_rwlock_unlock(p_lock_);
			}
		}

	private:
		pthread_rwlock_t *p_lock_;
};

//return position,find return >= 0, not find return -1,error return -2

inline int binary_search(uint32_t *p_data,uint32_t num,uint32_t search_v)
{
	int ret_index = -1;
	if (num <= 0) {return -2;}

	for (int begin = 0,end = num - 1,mid = ((begin + end) / 2) ; mid >= 0 && mid < num;)
	{
		//begin == end ,break the loop
		if (begin >= end)
		{
			if (search_v == p_data[begin])
			{
				ret_index = begin;
				for (int i = (begin-1);i>=0;i--)
				{
					if (search_v == p_data[i] )
					{ret_index = i;}
					else
					{break;}

				}
			}
			break;
		}

		if (search_v == p_data[mid])
		{
			ret_index = mid;
			for (int i = (mid-1);i>=0;i--)
			{
				if (search_v == p_data[i] )
				{ret_index = i;}
				else
				{break;}
			}
			break;
		}
		else if (search_v < p_data[mid])
		{
			end = mid - 1;
		}
		else
		{
			begin = mid + 1;
		}


		mid = (begin + end) / 2;
	}
	return ret_index;
}

class AutoLock_Mutex
{
	public:
		AutoLock_Mutex(pthread_mutex_t *p_mutex)
		{
			if (p_mutex == 0 )
				return;
			m_pmutex = p_mutex;
			pthread_mutex_lock(m_pmutex);
		}
		~AutoLock_Mutex()
		{
			if (m_pmutex == 0)
				return;
			pthread_mutex_unlock(m_pmutex);
		}
	private:
		pthread_mutex_t *m_pmutex;
};

class AutoRelease_HeapVec
{
	public:
		AutoRelease_HeapVec()
		{
			m_p_vec = new std::vector<uint32_t>();
		}
		~AutoRelease_HeapVec()
		{
			if (m_p_vec != 0)
			{delete m_p_vec;}
		}
		std::vector<uint32_t> *m_p_vec;
};

inline uint32_t get_adapt_mem(uint32_t use_mem,uint32_t default_data_num)
{
	uint32_t ret = default_data_num;
	if (use_mem < 1024)
	{return ret;}

	for (uint32_t i = (4*1024); i >=1024; i/=2)
	{
		if (use_mem >= i)
		{
			ret = i;
			break;
		}
	}
	return ret;
}

////////////////////////////////////////////////////////

typedef struct index_node
{
	uint32_t node_pos;//for up level loop
	struct index_node *prev_p;//for loop
	struct index_node *next_p;//for loop
	uint32_t sum_data_num;//to do,can dynamic,if data_num is 0,the index_node equal NULL
	uint32_t use_data_num;//for loop
	uint32_t *data_p;
}INDEX_NODE;

typedef struct index_hash_value
{
	uint32_t my_pos;//for lock
	uint32_t del_data_num;//for check if shrink
	uint32_t use_data_num;//for check if shrink
	uint32_t sum_node_num;//for loop,just ++
	struct index_node *head_p;//head count 1
}INDEX_HASH_VALUE;

class Index_Core
{
	public:
		Index_Core(int data_num = 512,int inner_lock_num = 128):_data_num((uint32_t)data_num)
									,_inner_lock_num((uint32_t)inner_lock_num),index_inner_lock_p(0)
									,_hash_now_num(0),_shrink_buffer_now(0)
	{
		if (data_num <=1 || inner_lock_num <=0)
		{
			_data_num = 512;
			_inner_lock_num = 128;
		}
		//lock init
		pthread_rwlock_init(&index_map_key_lock,NULL);
		pthread_mutex_init(&index_update_lock,NULL);

		index_inner_lock_p = new pthread_rwlock_t[_inner_lock_num];

		for (int i = 0;i < _inner_lock_num;i++)
		{
			pthread_rwlock_init(&index_inner_lock_p[i],NULL);
		}
		// _shrink_buffer init
		_shrink_buffer = 0;
		_shrink_buffer = new uint32_t[MAX_INDEX_NUM];

	}
		~Index_Core()
		{
			//release resource
			clear_all_index();

			//lock destroy
			pthread_rwlock_destroy(&index_map_key_lock);
			pthread_mutex_destroy(&index_update_lock);

			if (0 != index_inner_lock_p)
			{
				for (int i = 0;i < _inner_lock_num;i++)
				{
					pthread_rwlock_destroy(&index_inner_lock_p[i]);
				}
				delete []index_inner_lock_p;
				index_inner_lock_p = 0;

			}
		// _shrink_buffer clear
			if (_shrink_buffer != 0)
			{
				delete []_shrink_buffer;
			}
		}
		//index function
		void all_query_index(std::string index_hash_key,std::vector<uint32_t> &query_out);
		//the both vector(query_in vector and key -> vector),if query_in size = 0,return key -> vector
		void cross_query_index(std::string index_hash_key,std::vector<uint32_t> &query_in,std::vector<uint32_t> &query_out);
		//check if insert_in is bigger than the last data
		bool insert_index(std::string index_hash_key,uint32_t insert_in_value);
		bool delete_index(std::string index_hash_key,uint32_t delete_in_value);
		bool clear_index(std::string index_hash_key);
		bool shrink_index(std::string index_hash_key,bool adapt_mem = true);
		void clear_all_index();

		//find index info
		index_hash_value find_index(std::string index_hash_key);

	private:
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,index_hash_value> index_map;
#else
		std::map<std::string,index_hash_value> index_map;
#endif
		uint32_t _data_num;
		uint32_t _inner_lock_num;
		pthread_rwlock_t index_map_key_lock;//big lock
		pthread_rwlock_t *index_inner_lock_p;//tiny lock
		pthread_mutex_t index_update_lock;//write data to Serial	
		//map record insert and delete
		uint32_t _hash_now_num;
		//for shrink use
		uint32_t *_shrink_buffer;
		int _shrink_buffer_now;

};

#endif //__INDEX_CORE_H_
