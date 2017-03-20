#include "index_core.h"
#include <string.h>
//#include <stdio.h> //dodng-test


//uint32_t _shrink_buffer[MAX_INDEX_NUM] = {0};
//int _shrink_buffer_now = 0;

void Index_Core::all_query_index(std::string index_hash_key,std::vector<uint32_t> &query_out)
{
	if (index_hash_key.size() <= 0) {return;}

	// need all data
	{// enter big lock space
		AUTO_LOCK auto_lock(&index_map_key_lock,false);
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,index_hash_value>::iterator it = index_map.find(index_hash_key);
#else
		std::map<std::string,index_hash_value>::iterator it = index_map.find(index_hash_key);
#endif
		if (it != index_map.end())
		{//enter tiny lock space
			uint32_t skip_value = 0;
			AUTO_LOCK auto_lock_2(&index_inner_lock_p[it->second.my_pos % _inner_lock_num],false);
			//			index_node *p_node = &(it->second.head);
			index_node *p_node = it->second.head_p;
			for (int i = 0;
					i < it->second.sum_node_num && 0 != p_node; 
					i++,p_node = p_node->next_p)
			{//loop node
				for (int j = 0; j < p_node->use_data_num && j < p_node->sum_data_num;j++)
				{//loop data
					if (p_node->data_p[j] > skip_value)
					{
						query_out.push_back(p_node->data_p[j]);
						skip_value = p_node->data_p[j];
						//truncate return,avoid too many search and overflow the stack size
						if (query_out.size() > MAX_RECALL_NUM)
						{
							return;
						}
					}
				}

			}


		}

	}
}


void Index_Core::cross_query_index(std::string index_hash_key,std::vector<uint32_t> &query_in,std::vector<uint32_t> &query_out)
{
	if (index_hash_key.size() <= 0 || query_in.size() <= 0) {return;}

	//need cross data
	{// enter big lock space
		AUTO_LOCK auto_lock(&index_map_key_lock,false);
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,index_hash_value>::iterator it = index_map.find(index_hash_key);
#else
		std::map<std::string,index_hash_value>::iterator it = index_map.find(index_hash_key);
#endif
		if (it != index_map.end())
		{//enter tiny lock space
			AUTO_LOCK auto_lock_2(&index_inner_lock_p[it->second.my_pos % _inner_lock_num],false);
			//			index_node *p_node = &(it->second.head);
			index_node *p_node = it->second.head_p;
			for (int i = 0,k = 0;
					i < it->second.sum_node_num && 0 != p_node; 
					i++,p_node = p_node->next_p)
			{//loop node
				for (;k<query_in.size();)
				{
					//check
					if (query_in[k] <= 0 ) {k++;continue;}

					if (query_in[k] < p_node->data_p[0])	
					{//loop query_in
						k++;
					}
					else if (query_in[k] > p_node->data_p[p_node->use_data_num - 1])
					{//loop node
						break;
					}
					else
					{//search
						if(binary_search(p_node->data_p,p_node->use_data_num,query_in[k]) >= 0)
						{
							query_out.push_back(query_in[k]);
							//truncate return,avoid too many search and overflow the stack size
							if (query_out.size() > MAX_RECALL_NUM)
							{
								return;
							}
						}
						k++;
					}
				}
				//loop node break
				if (k >= query_in.size()){break;}
			}
		}
	}
}

bool Index_Core::insert_index(std::string index_hash_key,uint32_t insert_in_value)
{
	{ //enter index_update lock space
		AutoLock_Mutex auto_lock0(&index_update_lock);
		/*
		   1.push_back value in data 
		   2.insert query and head
		   3.insert node in last node
		 */
		int insert_statue = 0;

		if (index_hash_key.size() <= 0 || insert_in_value <= 0)
		{return false;}

		index_node *p_node_last = 0;
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,index_hash_value>::iterator it;
#else
		std::map<std::string,index_hash_value>::iterator it;
#endif
		{// enter big lock space
			AUTO_LOCK auto_lock(&index_map_key_lock,false);
			it = index_map.find(index_hash_key);

			if (it == index_map.end())
			{insert_statue = 2;}
			else
			{//enter tiny lock space
				AUTO_LOCK auto_lock_2(&index_inner_lock_p[it->second.my_pos % _inner_lock_num],false);
				//				index_node *p_node = &(it->second.head);
				index_node *p_node = it->second.head_p;
#if 0
				for (int i = 0;
						i < it->second.sum_node_num && 0 != p_node; 
						i++,p_node = p_node->next_p)
				{//loop node
					//if last node
					if (i == (it->second.sum_node_num - 1)) 
					{ p_node_last = p_node;}
				}
#endif
				if (it->second.sum_node_num == 1)
				{
					//					p_node_last = &(it->second.head);
					p_node_last = it->second.head_p;
				}
				else if (it->second.use_data_num > MAX_INDEX_NUM)
				{
					return false;
				}
				else
				{
					//					p_node_last = it->second.head.prev_p;
					p_node_last = it->second.head_p->prev_p;
				}

			}
		}

		if (insert_statue == 2)
		{
			//_hash_now_num
			index_hash_value one_hash_value = {0};//init 0
			index_hash_value * p_hash_value = &one_hash_value;
			//init hash value
			memset(p_hash_value,0,sizeof(index_hash_value));
			p_hash_value->my_pos = _hash_now_num;
			p_hash_value->del_data_num = 0;
			p_hash_value->use_data_num = 2;
			p_hash_value->sum_node_num = 1;
			p_hash_value->head_p = new index_node();
			memset(p_hash_value->head_p,0,sizeof(index_node));

			p_hash_value->head_p->node_pos = 0;
			p_hash_value->head_p->sum_data_num = _data_num;
			p_hash_value->head_p->use_data_num = 2;
			p_hash_value->head_p->data_p = new uint32_t[_data_num];
			//		printf("dodng-test 167 new[] %p\n",p_hash_value->head.data_p);
			memset(p_hash_value->head_p->data_p,0,sizeof(uint32_t)*_data_num);
			p_hash_value->head_p->data_p[0] = 0;
			p_hash_value->head_p->data_p[1] = insert_in_value;

			//insert query and head
			{// enter big lock space
				AUTO_LOCK auto_lock(&index_map_key_lock,true);
				index_map.insert ( std::pair<std::string,index_hash_value>(index_hash_key,*p_hash_value) );
				_hash_now_num++;	
			}
			return true;

		}
		else
		{
			if (0 == p_node_last || 
					p_node_last->data_p[p_node_last->use_data_num - 1] >= insert_in_value) {return false;}

			if (p_node_last->use_data_num < p_node_last->sum_data_num)
			{
				//1.push_back value in data 
				insert_statue = 1;
				p_node_last->data_p[p_node_last->use_data_num] = insert_in_value;
				{//enter tiny lock space
					AUTO_LOCK auto_lock_2(&index_inner_lock_p[it->second.my_pos % _inner_lock_num],true);
					it->second.use_data_num++;
					p_node_last->use_data_num++;
				}
			}
			else
			{
				//3.insert node in last node
				insert_statue = 3;
				//init index_node
				index_node *p_node = new index_node();
				//			printf("dodng-test 203 new %p\n",p_node);
				memset(p_node,0,sizeof(index_node));
				p_node->node_pos = p_node_last->node_pos + 1;
				p_node->sum_data_num = _data_num;
				p_node->use_data_num = 1;
				p_node->data_p = new uint32_t[_data_num];
				//			printf("dodng-test 209 new[] %p\n",p_node->data_p);
				memset(p_node->data_p,0,sizeof(uint32_t)*_data_num);
				p_node->data_p[0] = insert_in_value;

				{//enter tiny lock space
					AUTO_LOCK auto_lock_2(&index_inner_lock_p[it->second.my_pos % _inner_lock_num],true);
					it->second.use_data_num++;
					it->second.sum_node_num++;
					p_node_last->next_p = p_node;
					p_node->prev_p = p_node_last;
					p_node->next_p = it->second.head_p;
					it->second.head_p->prev_p = p_node;
				}
			}
			return true;
		}
	}

}

bool Index_Core::delete_index(std::string index_hash_key,uint32_t delete_in_value)
{
	{ //enter index_update lock space
		AutoLock_Mutex auto_lock0(&index_update_lock);

		if (index_hash_key.size() <= 0 || delete_in_value <= 0)
		{return false;}

		uint32_t delete_num = 0;
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,index_hash_value>::iterator it;
#else
		std::map<std::string,index_hash_value>::iterator it;
#endif

		{// enter big lock space
			AUTO_LOCK auto_lock(&index_map_key_lock,false);
			it = index_map.find(index_hash_key);
			if (it != index_map.end())
			{//enter tiny lock space
				AUTO_LOCK auto_lock_2(&index_inner_lock_p[it->second.my_pos % _inner_lock_num],false);
				index_node *p_node = it->second.head_p;
				for (int i = 0;
						i < it->second.sum_node_num && 0 != p_node; 
						i++,p_node = p_node->next_p)
				{//loop node
					if (delete_in_value < p_node->data_p[0])	
					{//loop query_in
						break;
					}
					else if (delete_in_value > p_node->data_p[p_node->use_data_num - 1])
					{//loop node
					}
					else
					{//search
						int ret_code = -4;
						if( (ret_code = binary_search(p_node->data_p,p_node->use_data_num,delete_in_value)) >= 0)
						{
							for (int j = ret_code;j < p_node->use_data_num;j++)
							{
								if (p_node->data_p[j] == delete_in_value)
								{
									p_node->data_p[j] = (ret_code > 0) ? p_node->data_p[j-1] :p_node->prev_p->data_p[p_node->prev_p->use_data_num - 1];
									delete_num++;
								}
								else
								{break;}

							}
						}
						else
						{//not find ,so break the loop
							break;
						}
					}
				}
			}
		}

		if (delete_num > 0)
		{//not need lock,beacuse no-query just one thread
			//			it->second.use_data_num = (it->second.use_data_num - delete_num) >= 0 ? (it->second.use_data_num - delete_num) :0;
			it->second.use_data_num--;
			//		it->second.del_data_num += delete_num;
			it->second.del_data_num++;
			return true;
		}
		else
		{return false;}

	}
}

bool Index_Core::clear_index(std::string index_hash_key)
{
	{ //enter index_update lock space
		AutoLock_Mutex auto_lock0(&index_update_lock);

		bool hit_hash_key = false;
		if (index_hash_key.size() <= 0 )
		{return false;}
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,index_hash_value>::iterator it;
#else
		std::map<std::string,index_hash_value>::iterator it;
#endif
		index_hash_value one_hash_value = {0};//init 0

		{// enter big lock space
			AUTO_LOCK auto_lock(&index_map_key_lock,true);
			it = index_map.find(index_hash_key);
			if (it != index_map.end())
			{
				hit_hash_key = true;
				one_hash_value = it->second;
				index_map.erase(it);
			}
		}

		if (!hit_hash_key)
		{return false;}

		//release resource
		index_node *p_node = one_hash_value.head_p;
		index_node p_node_mirror = {0};

		for (int i = 0;
				i < one_hash_value.sum_node_num && 0 != p_node; 
				i++,p_node = p_node->next_p)
		{//loop node
			//clone p_node for loop
			p_node_mirror = *p_node;

			//if head node,skip
#if 0
			if (i == 0) 
			{ 
				//release head data_p
				if (0 != p_node->data_p)
				{
					//				printf("dodng-test 332 delete[] %p\n",p_node->data_p);
					delete []p_node->data_p;
				}
			}
			else
#endif
			{
				//release myself
				if (0 != p_node->data_p)
				{
					//				printf("dodng-test 340 delete[] %p\n",p_node->data_p);
					delete []p_node->data_p;
				}

				//			printf("dodng-test 344 delete %p\n",p_node);
				delete p_node;
				//use 
				p_node = &p_node_mirror;

			}
		}


		return true;
	}
}

bool Index_Core::shrink_index(std::string index_hash_key,bool adapt_mem)
{
	{ //enter index_update lock space
		AutoLock_Mutex auto_lock0(&index_update_lock);

		bool hit_hash_key = false;
		bool empty_hash_key = false;

		if (index_hash_key.size() <= 0) {return false;}

//		AutoRelease_HeapVec heap_vec;
//		std::vector<uint32_t> &query_out = *(heap_vec.m_p_vec);

		//1.all_query get all datas(skip same and continuation numbers)
		{
			// need all data
			{// enter big lock space
				AUTO_LOCK auto_lock(&index_map_key_lock,false);
#ifdef _USE_HASH_
				std::tr1::unordered_map<std::string,index_hash_value>::iterator it = index_map.find(index_hash_key);
#else
				std::map<std::string,index_hash_value>::iterator it = index_map.find(index_hash_key);
#endif
				if (it != index_map.end())
				{//enter tiny lock space
					uint32_t skip_value = 0;
					//push 0 into first 
					//query_out.push_back(0);
					_shrink_buffer_now = 0;
					if (_shrink_buffer_now < MAX_INDEX_NUM)
					{_shrink_buffer[_shrink_buffer_now++] = 0;}

					AUTO_LOCK auto_lock_2(&index_inner_lock_p[it->second.my_pos % _inner_lock_num],false);
					index_node *p_node = it->second.head_p;
					for (int i = 0;
							i < it->second.sum_node_num && 0 != p_node;
							i++,p_node = p_node->next_p)
					{//loop node
						for (int j = 0; j < p_node->use_data_num && j < p_node->sum_data_num;j++)
						{//loop data
							if (p_node->data_p[j] > skip_value)
							{
								//query_out.push_back(p_node->data_p[j]);
								if (_shrink_buffer_now < MAX_INDEX_NUM)
								{_shrink_buffer[_shrink_buffer_now++] = p_node->data_p[j];}
								skip_value = p_node->data_p[j];
							}
						}

					}


				}

			}
		}

		if (_shrink_buffer_now <= 0 )
		{return false;}
		else if (_shrink_buffer_now == 1)//notice:pushed 0 into first 
		{empty_hash_key = true;}

		//2.generate new hash_values
		index_hash_value new_one_hash_value = {0};//init 0
		index_hash_value * p_hash_value = &new_one_hash_value;

		if (!empty_hash_key)
		{//init hash value

			uint32_t data_num_here = adapt_mem?get_adapt_mem(_shrink_buffer_now,_data_num):_data_num;
			memset(p_hash_value,0,sizeof(index_hash_value));
			p_hash_value->my_pos = 0;//notice use old;
			p_hash_value->del_data_num = 0;
			p_hash_value->use_data_num = 0;
			p_hash_value->sum_node_num = 1;
			p_hash_value->head_p = new index_node();
			memset(p_hash_value->head_p,0,sizeof(index_node));

			p_hash_value->head_p->node_pos = 0;
			p_hash_value->head_p->sum_data_num = data_num_here;
			p_hash_value->head_p->use_data_num = 0;
			p_hash_value->head_p->data_p = new uint32_t[data_num_here];
			//		printf("dodng-test 413 new[] %p\n",p_hash_value->head.data_p);
			memset(p_hash_value->head_p->data_p,0,sizeof(uint32_t)*data_num_here);

			uint32_t *data_value_p = 0;
			index_node *p_node_last = p_hash_value->head_p;

			for (int i = 0 ; i < _shrink_buffer_now; i++)
			{
				if ( p_node_last->use_data_num >= data_num_here)
				{//need malloc one node
					data_num_here=adapt_mem?get_adapt_mem(_shrink_buffer_now - i,_data_num):_data_num;
					index_node *p_node = new index_node();
					memset(p_node,0,sizeof(index_node));
					//					printf("dodng-test 426 new %p\n",p_node);
					p_node->node_pos = p_node_last->node_pos + 1;
					p_node->sum_data_num = data_num_here;
					p_node->use_data_num = 0;
					p_node->data_p = new uint32_t[data_num_here];
					//					printf("dodng-test 431 new[] %p\n",p_node->data_p);
					memset(p_node->data_p,0,sizeof(uint32_t)*data_num_here);

					{//pointer
						p_node_last->next_p = p_node;
						p_node->prev_p = p_node_last;
						p_node->next_p = p_hash_value->head_p;
						p_hash_value->head_p->prev_p = p_node;
					}
					p_node_last = p_node;
					p_hash_value->sum_node_num++;

				}

				p_hash_value->use_data_num++;
				p_node_last->use_data_num++;
				data_value_p = &(p_node_last->data_p[p_node_last->use_data_num - 1]);
				*data_value_p = _shrink_buffer[i];
			}

		}

		//3.erase old and insert new
		index_hash_value old_one_hash_value = {0};//init 0
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,index_hash_value>::iterator it;
#else
		std::map<std::string,index_hash_value>::iterator it;
#endif

		{// enter big lock space
			AUTO_LOCK auto_lock(&index_map_key_lock,true);
			it = index_map.find(index_hash_key);
			if (it != index_map.end())
			{
				hit_hash_key = true;
				old_one_hash_value = it->second;
				new_one_hash_value.my_pos = old_one_hash_value.my_pos;
				index_map.erase(it);
				if (!empty_hash_key)
				{index_map.insert ( std::pair<std::string,index_hash_value>(index_hash_key,new_one_hash_value) );}
			}
		}

		if (!hit_hash_key) {return false;}

		//4.release old hash_values
		if (!empty_hash_key)
		{
			//release resource
			index_node *p_node = old_one_hash_value.head_p;
			index_node p_node_mirror = {0};

			for (int i = 0;
					i < old_one_hash_value.sum_node_num && 0 != p_node; 
					i++,p_node = p_node->next_p)
			{//loop node
				//clone p_node for loop
				p_node_mirror = *p_node;

				//if head node,skip
#if 0
				if (i == 0) 
				{ 
					//release head data_p
					if (0 != p_node->data_p)
					{
						//					printf("dodng-test 499 delete[] %p\n",p_node->data_p);
						delete []p_node->data_p;
					}
				}
				else
#endif
				{
					//release myself
					if (0 != p_node->data_p)
					{
						//					printf("dodng-test 507 delete[] %p\n",p_node->data_p);
						delete []p_node->data_p;
					}

					//				printf("dodng-test 511 delete %p\n",p_node);
					delete p_node;
					//use 
					p_node = &p_node_mirror;

				}
			}

		}
		return true;
	}
}

index_hash_value Index_Core::find_index(std::string index_hash_key)
{
	index_hash_value ret_data = {0};
	{// enter big lock space
		AUTO_LOCK auto_lock(&index_map_key_lock,false);
#ifdef _USE_HASH_
		std::tr1::unordered_map<std::string,index_hash_value>::iterator it = index_map.find(index_hash_key);
#else
		std::map<std::string,index_hash_value>::iterator it = index_map.find(index_hash_key);
#endif
		if (it != index_map.end())
		{
			ret_data = it->second;
		}
	}
	return ret_data;
}

void Index_Core::clear_all_index()
{
	{ //enter index_update lock space
		AutoLock_Mutex auto_lock0(&index_update_lock);

		{// enter big lock space
			AUTO_LOCK auto_lock(&index_map_key_lock,true);
#ifdef _USE_HASH_
			for (std::tr1::unordered_map<std::string,index_hash_value>::iterator it = index_map.begin(); it!=index_map.end(); ++it)
#else
				for (std::map<std::string,index_hash_value>::iterator it = index_map.begin(); it!=index_map.end(); ++it)
#endif
				{
					index_hash_value one_hash_value = {0};//init 0
					one_hash_value = it->second;

					//release resource
					index_node *p_node = one_hash_value.head_p;
					index_node p_node_mirror = {0};

					for (int i = 0;
							i < one_hash_value.sum_node_num && 0 != p_node; 
							i++,p_node = p_node->next_p)
					{//loop node
						//clone p_node for loop
						p_node_mirror = *p_node;

						//if head node,skip
#if 0
						if (i == 0) 
						{ 
							//release head data_p
							if (0 != p_node->data_p)
							{
								//				printf("dodng-test 332 delete[] %p\n",p_node->data_p);
								delete []p_node->data_p;
							}
						}
						else
#endif
						{
							//release myself
							if (0 != p_node->data_p)
							{
								//				printf("dodng-test 340 delete[] %p\n",p_node->data_p);
								delete []p_node->data_p;
							}

							//			printf("dodng-test 344 delete %p\n",p_node);
							delete p_node;
							//use 
							p_node = &p_node_mirror;

						}
					}



				}

		}
	}

}
