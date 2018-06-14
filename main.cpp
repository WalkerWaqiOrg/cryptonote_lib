#include "hash.h"
#include <string.h>
#include <stdio.h>
#include <iostream>

//----------------------------------------------------------------------------
inline bool trim_left(std::string& str)
{
    for(std::string::iterator it = str.begin(); it!= str.end() && isspace(static_cast<unsigned char>(*it));)
        str.erase(str.begin());
        
    return true;
}

//----------------------------------------------------------------------------
inline bool trim_right(std::string& str)
{

    for(std::string::reverse_iterator it = str.rbegin(); it!= str.rend() && isspace(static_cast<unsigned char>(*it));)
        str.erase( --((it++).base()));

    return true;
}

//----------------------------------------------------------------------------
inline std::string& trim(std::string& str)
{

    trim_left(str);
    trim_right(str);
    return str;
}

//----------------------------------------------------------------------------
inline std::string trim(const std::string& str_)
{
    std::string str = str_;
    trim_left(str);
    trim_right(str);
    return str;
}

//----------------------------------------------------------------------------
template<class CharT>
bool parse_hexstr_to_binbuff(const std::basic_string<CharT>& s, std::basic_string<CharT>& res, bool allow_partial_byte = false)
{
    res.clear();
    if (!allow_partial_byte && (s.size() & 1))
        return false;
    try
    {
        long v = 0;
        for(size_t i = 0; i < (s.size() + 1) / 2; i++)
        {
        CharT byte_str[3];
        size_t copied = s.copy(byte_str, 2, 2 * i);
        byte_str[copied] = CharT(0);
        CharT* endptr;
        v = strtoul(byte_str, &endptr, 16);
        if (v < 0 || 0xFF < v || endptr != byte_str + copied)
        {
            return false;
        }
        res.push_back(static_cast<unsigned char>(v));
        }

        return true;
    }catch(...)
    {
        return false;
    }
}

template<class t_pod_type>
bool hex_to_pod(const std::string& hex_str, t_pod_type& s)
{
    static_assert(std::is_pod<t_pod_type>::value, "expected pod type");
    std::string hex_str_tr = trim(hex_str);
    if(sizeof(s)*2 != hex_str.size())
        return false;
    std::string bin_buff;
    if(!parse_hexstr_to_binbuff(hex_str_tr, bin_buff))
        return false;
    if(bin_buff.size()!=sizeof(s))
        return false;

    s = *(t_pod_type*)bin_buff.data();
    return true;
}

class test_cn_slow_hash
{
public:
    static const size_t loop_count = 10;

#pragma pack(push, 1)
  struct data_t
  {
    char data[13];
  };
#pragma pack(pop)

  static_assert(13 == sizeof(data_t), "Invalid structure size");

  bool init()
  {
    if (!hex_to_pod("63617665617420656d70746f72", m_data))
      return false;

    if (!hex_to_pod("23bf2e70f9d7f3d017297dd7ebf62a302198c1b598115b5230579141f2824f8d", m_expected_hash))
      return false;

    return true;
  }

  bool test()
  {
    crypto::hash hash;
    crypto::cn_slow_hash(&m_data, sizeof(m_data), hash);

    return hash == m_expected_hash;
  }

private:
    data_t m_data;
    crypto::hash m_expected_hash;
};

int main(int argc, char const *argv[])
{
    test_cn_slow_hash test_slow_demo;
    test_slow_demo.init();
    if(test_slow_demo.test())
    {
        std::cout << "hash result is ok!" << std::endl;
    }
    else
    {
        std::cout << "hash result is error!" << std::endl;
    }
    return 0;
}
