/***************************************************************************
 * 
 *	author:dodng
 *	e-mail:dodng12@163.com
 * 	2017/3/16
 *   
 **************************************************************************/

#ifndef MY_LOG_H
#define MY_LOG_H

#include <stdio.h>
#include <string>
#include <pthread.h>

int
my_log(FILE *p_file, const char *format, ...);
int
my_log_init(FILE **p_file,const char * file_name);


class easy_log
{
	public:
		easy_log(std::string filename,int r_sum,int l_sum)
			:p_file(NULL),record_sum(r_sum),log_sum(l_sum)
			,record_count(0),log_index(0)
		{
			file_name = filename;
			my_log_init(&p_file,file_name.c_str());
			printf("init log[%s]\n",file_name.c_str());
			pthread_mutex_init(&_lock,NULL);
		}
		~easy_log()
		{
			if (p_file)
			{
				fclose(p_file);
				p_file = NULL;
			}
			pthread_mutex_destroy(&_lock);
		}
		void write_record(const char *log_content)
		{
			//lock
			pthread_mutex_lock(&_lock);
			// change log_file
			if ((++record_count % record_sum) == 0)
			{
				std::string temp_name = file_name;
				//
				log_index = (log_index + 1) % log_sum;
				if (log_index != 0)
				{
					temp_name += '.';
					temp_name += '0'+log_index;
				}
				//close old
				if (p_file)
				{
					fclose(p_file);
					p_file = NULL;
				}
				//gen new
				my_log_init(&p_file,temp_name.c_str());
			}
			//write log
			if(p_file)
				{my_log(p_file,"%s\n",log_content);}
			pthread_mutex_unlock(&_lock);
		}
	private:
		std::string file_name;
		FILE *p_file;
		unsigned int record_count;
		unsigned int record_sum;
		unsigned int log_sum;
		unsigned int log_index;
		pthread_mutex_t _lock;//write data to Serial

};

#endif
