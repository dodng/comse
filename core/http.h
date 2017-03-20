/***************************************************************************
 * 
 *	author:dodng
 *	e-mail:dodng12@163.com
 * 	2017/3/16
 *   
 **************************************************************************/

#ifndef COMSE_HTTP_H_
#define COMSE_HTTP_H_

#include <string>
#include <vector>
#include <map>
#include <stdint.h>
#include <stdlib.h>

inline std::vector<std::string> split(std::string str,std::string pattern)
{
    std::string::size_type pos;
    std::vector<std::string> result;
    str+=pattern;//extern str,so find never return -1
    int size=str.size();

    for(int i=0; i<size; i++)
    {   
        pos=str.find(pattern,i);
        if(pos<size)
        {   
            std::string s=str.substr(i,pos-i);
            if (s.size() > 0)
            {   
                result.push_back(s);
                i=pos+pattern.size()-1;
            }   
        }   
    }   
    return result;
}

char url_decode_one(char high,char low);
std::string url_decode(std::string str);
std::string url_encode_one(char ch);
std::string url_encode(std::string str);

enum http_status_code
{
    http_status_init = 0,
    http_status_parse_head_done = 1,
    http_status_parse_body_done = 2
};

class http_entity{
	public:
		http_entity()
		{
			status = http_status_init;
			parse_first_row_len = 0;
			parse_header_len = 0;
			parse_body_len = 0;
		}
		~http_entity()
		{
		}
		void parse_header(char *p_str);
		void parse_body(char *p_str);
//		std::string print_all();
		int parse_done(char *p_str);
		bool parse_over()
		{
			if (status == http_status_parse_body_done)
			{return true;}
			else
			{return false;}
		}
		void reset()
		{
			status = http_status_init;
			parse_first_row_len = 0;
			parse_header_len = 0;
			parse_body_len = 0;
			header_first_row.clear();
			header_map.clear();
			body_map.clear();
		}
		/////data
		int status;
		std::string header_first_row;
		std::map<std::string,std::string> header_map;
		std::map<std::string,std::string> body_map;
		int parse_first_row_len;
		int parse_header_len;
		int parse_body_len;

};
#endif
