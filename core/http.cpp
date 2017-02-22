#include "http.h"

char url_decode_one(char high,char low)
{
	uint8_t rst = 0;

	if (high >= '0' && high <= '9')
	{
		rst = (high - '0')<<4;
	}
	else if (high >= 'a' && high <= 'f')
	{
		rst = (high - 'a' + 10)<<4;
	}
	else if (high >= 'A' && high <= 'F')
	{
		rst = (high - 'A' + 10)<<4;
	}

	if (low >= '0' && low <= '9')
	{
		rst |= (low - '0');
	}
	else if (low >= 'a' && low <= 'f')
	{
		rst |= (low - 'a' + 10);
	}
	else if (low >= 'A' && low <= 'F')
	{
		rst |= (low - 'A' + 10);
	}
	return (char)rst;
}

std::string url_decode(std::string str)
{
	std::string rst;
	for (int i = 0 ;i < str.size();i++)
	{
		if (str.c_str()[i] == '%')
		{
			if (i+2 < str.size())
			{
				rst += url_decode_one(str.c_str()[i+1],str.c_str()[i+2]);
				i+=2;
			}
		}
		else if (str.c_str()[i] == '+')
		{
			rst += " ";
		}
		else
		{
			rst += str.c_str()[i];
		}
	}
	return rst;
}

std::string url_encode_one(char ch)
{
	std::string rst;
	rst += ch;

	if(ch == ' ')
	{
		return "+";
	}else if(ch >= 'A' && ch <= 'Z'){
		return rst;
	}else if(ch >= 'a' && ch <= 'z'){
		return rst;
	}else if(ch >= '0' && ch <= '9'){
		return rst;
	}else if(ch == '-' || ch == '_' || ch == '.' || ch == '*' ){
		return rst;
	}
	else
	{
		rst = "%";
		uint8_t high = (((uint8_t)ch) >> 4) % 16;
		uint8_t low = (((uint8_t)ch)) % 16;
		if (high >= 10)
		{rst += 'A' + (high - 10);}
		else
		{rst += '0' + high;}
		
		if (low >= 10)
		{rst += 'A' + (low - 10);}
		else
		{rst += '0' + low;}
		
		return rst;
	}

}

std::string url_encode(std::string str)
{
	std::string rst;
	for (int i = 0 ; i < str.size();i++)
	{
		rst += url_encode_one(str.c_str()[i]);
	}

	return rst;
}

void http_entity::parse_header(char *p_str)
{
	if (0 == p_str) {return;}
	std::string str_here(p_str);
	std::vector<std::string> split_rst1 = split(str_here,"\r\n\r\n");
	if (split_rst1.size() >= 2)
	{
		status = http_status_parse_head_done;
		parse_header_len = split_rst1[0].size();
		std::vector<std::string> split_rst2 = split(split_rst1[0],"\r\n");
		if (split_rst2.size() >= 2)
		{
			parse_first_row_len = split_rst2[0].size();
			header_first_row = split_rst2[0];
			//parse head
			for (int i = 1; i < split_rst2.size();i++)
			{
				std::string be_found(": ");
				std::size_t found = split_rst2[i].find(be_found);
				if (found == std::string::npos) {continue;}
				std::string key = split_rst2[i].substr(0,found);
				std::string value = split_rst2[i].substr(found+be_found.size());
				header_map.insert ( std::pair<std::string,std::string>(key,value) );

			}
		}
	}

}

void http_entity::parse_body(char *p_str)
{
	if (0 == p_str) {return;}
	std::string str_here(p_str);
	std::vector<std::string> split_rst1 = split(str_here,"\r\n\r\n");
	if (split_rst1.size() >= 2)
	{
		std::vector<std::string> split_rst2 = split(split_rst1[1],"&");
		if (split_rst2.size() >= 1)
		{
			parse_body_len = split_rst1[1].size();
			//parse body
			for (int i = 0; i < split_rst2.size();i++)
			{
				std::string be_found("=");
				std::size_t found = split_rst2[i].find(be_found);
				if (found == std::string::npos) {continue;}
				std::string key = split_rst2[i].substr(0,found);
				std::string value = split_rst2[i].substr(found+be_found.size());
				body_map.insert ( std::pair<std::string,std::string>(key,value) );

			}
		}
	}
}
#if 0
std::string http_entity::print_all()
{
	std::string rst;
	//			printf("%s\n",header_first_row.c_str());
	rst += header_first_row + "\n";
	for (std::map<std::string,std::string>::iterator it=header_map.begin(); it!=header_map.end(); ++it)
		//				printf("[%s]=>[%s]\n",it->first.c_str(),it->second.c_str());
		rst += "[" + it->first + "]=>[" + it->second.c_str() + "]\n";
	for (std::map<std::string,std::string>::iterator it=body_map.begin(); it!=body_map.end(); ++it)
		//				printf("[%s]=>[%s]\n",it->first.c_str(),it->second.c_str());
		rst += "[" + it->first + "]=>[" + it->second.c_str() + "]\n";
	return rst;
}
#endif
int http_entity::parse_done(char *p_str)
{//return -1 error, 0 continue receive ,1 parse done
//do parse
	int rst = -1;
	if (0 == p_str) {return -2;}

	if (status == http_status_init)
	{
		parse_header(p_str);
	}
	
	if (status == http_status_parse_head_done)
	{
		parse_body(p_str);
	}
//done
//	if ( parse_first_row_len <= 0 || parse_header_len <= 0)
//	{return rst;}
	if (status == http_status_init)
	{
		return 0;	
	}
	else if (status == http_status_parse_head_done)
	{ //next
	}
	else
	{
		return 1;
	}
	

	//Content-Length
	std::map<std::string,std::string>::iterator it;
	it = header_map.find("Content-Length");
	if (it == header_map.end())	
	{
		rst = 1;
		status = http_status_parse_body_done;
		return rst;
	}

	int ct_len = atoi(it->second.c_str());

	if (parse_body_len < ct_len)
	{rst = 0;}
	else
	{
		rst = 1;
		status = http_status_parse_body_done;
	}

	return rst;	
}
