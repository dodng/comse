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

class http_entity{
	public:
		http_entity()
		{
			parse_first_row_len = 0;
			parse_header_len = 0;
			parse_body_len = 0;
		}
		~http_entity()
		{
		}
		void parse_header(char *p_str);
		void parse_body(char *p_str);
		std::string print_all();
		int parse_done();
		std::string header_first_row;
		std::map<std::string,std::string> header_map;
		std::map<std::string,std::string> body_map;
		int parse_first_row_len;
		int parse_header_len;
		int parse_body_len;

};
#endif
