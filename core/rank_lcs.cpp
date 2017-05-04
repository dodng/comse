#include "rank_lcs.h"

/*
 * long str:long_str
 * small str:small_str
 * both str:both_str(include dup string)
 * lcs:lcs_n
 * time cost: both_str * log(lcs_n)[binary search] + (small_str - both_str) * O(1)[hash search]
 * */

bool cmp(int a,int b){return a>=b;}

std::string fast_lcs_v3_1(const char *p_str1,const char *p_str2)
{
    if (0 == p_str1 || 0 == p_str2)
    {return "";}

    int str1_len = strlen(p_str1);
    int str2_len = strlen(p_str2);
    std::string loop_string = "";
    std::string hash_string = "";
    std::map<std::string,std::vector<int> > hash_map;
    std::map<std::string,std::vector<int> >::iterator hash_map_it;
    std::vector<int> judge_vec;
    //for lcs
    std::string lcs_string = "";
    std::vector<std::vector<int> > judge_vec2vec;

    //get smaller string
    if (str1_len < str2_len)
    {
        loop_string = p_str1;
        hash_string = p_str2;
    }
    else
    {
        loop_string = p_str2;
        hash_string = p_str1;

    }
    //make longer string to hash(vector)
    for (int i = 0 ; i < hash_string.length();i++)
    {
        hash_map_it = hash_map.find(hash_string.substr(i,1));
        if (hash_map_it != hash_map.end())
        {
            //update,push vector back
            hash_map_it->second.push_back(i);
        }
        else
        {
            std::vector<int> tmp_vec(1,i);
            //insert
            hash_map.insert(std::pair<std::string,std::vector<int> >(hash_string.substr(i,1),tmp_vec));
        }
    }
    //loop smaller string
    for (int i = 0 ; i < loop_string.length();i++)
    {
        hash_map_it = hash_map.find(loop_string.substr(i,1));
        if (hash_map_it != hash_map.end())
        {
            //if dup index,from high to low loop
            //make judge vector,binary search
            for (int j = hash_map_it->second.size() - 1;j>=0;j--)
            {
                std::vector<int>::iterator vec_it;
                vec_it = std::lower_bound (judge_vec.begin(), judge_vec.end(), hash_map_it->second[j]);
                int index = vec_it - judge_vec.begin();
                if (index == 0 && judge_vec.size() <= 0)
                {
                    //vec is null,insert to vec front
                    judge_vec.push_back(hash_map_it->second[j]);
                    //for lcs
                    std::vector<int> tmp_vec(1,hash_map_it->second[j]);
                    judge_vec2vec.push_back(tmp_vec);
                }
                else if (index >= judge_vec.size())
                {
                    //insert to vec back
                    judge_vec.push_back(hash_map_it->second[j]);
                    //for lcs
                    std::vector<int> tmp_vec(1,hash_map_it->second[j]);
                    judge_vec2vec.push_back(tmp_vec);
                }
                else
                {
                    //equal skip
                    if (judge_vec[index] == hash_map_it->second[j])
                    {
                    
        //                printf("skip\n");
                    }
                    else
                    {
                    //replace element
                        judge_vec[index] = hash_map_it->second[j];
                    //for lcs
                        judge_vec2vec[index].push_back(hash_map_it->second[j]);
                    }
                }

            }

        }
        else
        {// doesn't exist char
            continue;
        }

    }
    //get lcs
    int use_threshold = 0;
    for (int i = judge_vec2vec.size() - 1;i >= 0;i--)
    {   

        //printf("row[%d]\n",i);
        //last one use judge_vec
        if (i == (judge_vec2vec.size() - 1))
        {
            lcs_string += hash_string.substr(judge_vec[i],1);
            use_threshold = judge_vec[i];
            continue;
        }
        //else sort,from end to start get the value
        std::vector<int>::iterator vec_it;
        vec_it = std::lower_bound (judge_vec2vec[i].begin(), judge_vec2vec[i].end(), use_threshold,cmp);
        int index = vec_it - judge_vec2vec[i].begin();
        if ((index) < 0)
        {//printf("error\n");
        }
        else
        {
            lcs_string += hash_string.substr(judge_vec2vec[i][index],1);
            use_threshold = judge_vec2vec[i][index];
        }
        //printf("change row\n");
    }
    //printf("lcs_length: %d\n",judge_vec.size()); 

    //reverse to get lcs string
    std::reverse(lcs_string.begin(), lcs_string.end());
    //retrun judge vector length
    return lcs_string;
}

