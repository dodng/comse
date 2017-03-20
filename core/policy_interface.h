/***************************************************************************
 * 
 *	author:dodng
 *	e-mail:dodng12@163.com
 * 	2017/3/16
 *   
 **************************************************************************/

#ifndef COMSE_POLICY_H_
#define COMSE_POLICY_H_

#include "http.h"
#include <string>
#include "json/json.h"

class policy_entity{
	public:
		policy_entity(){it_http = 0;}
		~policy_entity(){}
		void set_http(http_entity *it_http_p)
		{
			if (0 != it_http_p)
			{it_http = it_http_p;}
		}
		int parse_in_json();
		int get_out_json();
		int cook_senddata(char *send_buff_p,int buff_len,int &send_len);
//		std::string print_all();
		void reset()
		{
			it_http = 0;
			json_in.clear();
			json_out.clear();
		}
		int do_one_action(http_entity *it_http_p,char *send_buff_p,int buff_len,int &send_len);
	private:
		http_entity *it_http;
		Json::Value json_in;
		Json::Value json_out;
};

bool policy_interface_init_once();
#endif
