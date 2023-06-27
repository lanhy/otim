#include "strutil.h"
#include <sstream>
#include <iconv.h> 
#include <regex>
#include <arpa/inet.h>
#include "log.h"

namespace hcommon
{
#define random(a,b) (rand()%(b-a)+a)
#define STR_TO_NUM(type) \
    std::stringstream ss; \
    ss << strNumber; \
    type num = 0; \
    ss >> num; \
    return num; \



// 下面的这个函数会把“1,2,3,"这种分割为4个
std::vector<std::string> string_split(const std::string &strSource, const char &chSplitChar)
{
    std::vector<std::string> ret;
    std::string::size_type last = 0;
    std::string::size_type index = strSource.find(chSplitChar, last);
    while (index != std::string::npos)
    {
        ret.push_back(strSource.substr(last, index - last));
        last = index + 1;
        index = strSource.find(chSplitChar, last);
    }
    if (index - last > 0)
    {
        ret.push_back(strSource.substr(last, index - last));
    }

    return ret;
}

std::string string_concat(const std::vector<std::string> &strv, const std::string &c)
{
    if (strv.empty())
    {
        return "";
    }
    if (strv.empty() == 1)
    {
        return strv.at(0);
    }

    std::string ret;
    for (auto item : strv)
    {
        ret += item;
        ret += c;
        MLOG_INFO("item:"<<item<<" ret:"<<ret);
    }
    if (ret.size() > 0)
    {
        ret.erase(ret.end()-c.length(), ret.end());
    }
    MLOG_INFO("ret:"<<ret);

    return ret;
}

std::string string_trim(const std::string &str, const char c)
{
    std::string::size_type pos = str.find_first_not_of(c);
    if (pos == std::string::npos)
    {
        return str;
    }
    std::string::size_type pos2 = str.find_last_not_of(c);
    if (pos2 != std::string::npos)
    {
        return str.substr(pos, pos2 - pos + 1);
    }
    return str.substr(pos);
}

std::string& replace_all(std::string& str,const std::string& old_value,const std::string& new_value)
{
    if (str.empty() || old_value.empty() || old_value == new_value){
        return str;
    }
    
    size_t pos = 0;
    while (true)
    {
        pos = str.find(old_value, pos);
        if (pos == std::string::npos){
            break;
        }
        
        str.replace(pos, old_value.length(), new_value);
        pos = pos + new_value.length();
    }
    
    return str;
}

uint64_t str_to_UINT64(const std::string &strNumber)
{
    STR_TO_NUM(uint64_t);
}

int32_t str_to_INT32(const std::string &strNumber)
{
    STR_TO_NUM(int32_t);
}

bool utf8_to_utf32(const std::string& input, std::u32string &output)
{
    output.clear();
    
    iconv_t cd = iconv_open("UTF-32LE", "UTF-8");
    if (cd == (iconv_t)-1)
    {
        return false;
    }

    /* 由于iconv()函数会修改指针，所以要保存源指针 */
    char outbuf[16384 * 4] = {0};
    size_t outlen = 16384 * 4;
    char *tmpin = (char *)input.c_str();
    char *tmpout = outbuf;
    size_t inlen = input.size();
    size_t ori_outsize = outlen;

    size_t ret = iconv(cd, &tmpin, &inlen, &tmpout, &outlen);
    if (ret == (size_t)-1)
    {
        iconv_close(cd);
        return false;
    }

    /* 存放转换后的字符串 */
    //printf("outbuf=%s\n", outbuf);

    //存放转换后outbuf剩余的空间
    //printf("outlen=%d\n", *outlen);

    //for (auto i = 0; i < (ori_outsize-(*outlen)); i++)
    //{
        //printf("%x\n", outbuf[i]);
    //}

    iconv_close(cd);
    //printf("outsize=%d\n", outsize);
    output.assign((char32_t *)outbuf, (ori_outsize-outlen)/4);

    return true;
}

uint32_t get_utf8_word(const char *s, uint32_t& offset, uint32_t wantedNum)  
{  
    uint32_t i = 0, j = 0;  

    uint32_t readedNum = 0;  
    uint32_t isReach = 0;  
    if(wantedNum == 0) {
        offset = 0;
        return readedNum;
    }
    while (s[i])  {  
        //符合UTF8编码规则就计数
        if ((s[i] & 0xc0) != 0x80)  {  
            if(isReach)  {  
                break;  
            }  
            ++j;  
            readedNum = j;  
            if(j == wantedNum)  {  
                isReach = 1;  
            }  
        }  
        ++i;  
    }  
    offset = i;  
    return readedNum;  
}  

uint32_t get_utf8_length(const std::string &input)
{
    uint32_t length = 0;
    for (size_t i = 0, len = 0; i < input.length(); i += len) {
        unsigned char byte = input[i];
        if (byte >= 0xFC) // lenght 6
            len = 6;
        else if (byte >= 0xF8)
            len = 5;
        else if (byte >= 0xF0)
            len = 4;
        else if (byte >= 0xE0)
            len = 3;
        else if (byte >= 0xC0)
            len = 2;
        else
            len = 1;
        length++;
    }
    return length;
}

std::string json2string(rapidjson::Value & value)
{
    std::string result; 
    if(value.IsObject()) 
    {
            rapidjson::StringBuffer sbBuf;
            rapidjson::Writer<rapidjson::StringBuffer> jWriter(sbBuf);
            value.Accept(jWriter);
            result = sbBuf.GetString();
    }
    return result;
}

std::string sub_string(const std::string &strCurStr, int length)
{
    
    int len = hcommon::get_utf8_length(strCurStr);
    if (len <= length)
    {
        return strCurStr;
    }

    std::string strLastData;
    std::string strChar;

    len = 0;
    for (size_t i = 0; i < strCurStr.length();)
    {
       
        char chr = strCurStr[i];

        //chr是0xxx xxxx，即ascii码
        if ((chr & 0x80) == 0) {
            strChar = strCurStr.substr(i, 1);
            ++i;
            strLastData += strChar;
        }//chr是1111 1xxx
        else if ((chr & 0xF8) == 0xF8)
        {
            strChar = strCurStr.substr(i, 5);
            i += 5;
            strLastData += strChar;
        }//chr是1111 xxxx
        else if ((chr & 0xF0) == 0xF0)
        {
            strChar = strCurStr.substr(i, 4);
            i += 4;
            strLastData += strChar;
        }//chr是111x xxxx
        else if ((chr & 0xE0) == 0xE0)
        {
            strChar = strCurStr.substr(i, 3);
            i += 3;
            strLastData += strChar;
        }//chr是11xx xxxx
        else if ((chr & 0xC0) == 0xC0)
        {
            strChar = strCurStr.substr(i, 2);
            i += 2;
            strLastData += strChar;
        }
        
        len ++;
        if (len >= length)
        {
            break;
        }
    }

    return strLastData;    
}

uint32_t ipToLong(const char * ip, uint32_t &prefix)
{
    
    /*int a, b, c, d;
        sscanf_s(ip, "%u.%u.%u.%u", &a, &b, &c, &d);
        prefix = (BYTE)a;
        return ((BYTE)a << 24) | ((BYTE)b << 16) | ((BYTE)c << 8) | (BYTE)d;
        */
    
    int a = 0, b = 0, c = 0, d = 0;
    int iLen;
    int abcdIndex = 0;
    iLen = strlen(ip);
    char ips[3];
    memset(ips, '\0', 3);
    
    int ipsCnt = 0;
    for (int i = 0; i < iLen; i++)
    {
        if ('.' == ip[i])
        {
            if (0 == abcdIndex)
            {
                abcdIndex = 1;
                a = atoi(ips);
            }
            else if (1 == abcdIndex)
            {
                abcdIndex = 2;
                b = atoi(ips);
            }
            else if (2 == abcdIndex)
            {
                abcdIndex = 3;
                c = atoi(ips);
            }
            
            ipsCnt = 0;
            memset(ips, '\0', 3);
        }
        else
        {
            ips[ipsCnt] = ip[i];
            ipsCnt++;
        }
    }
    d = atoi(ips);
    
    prefix = (uint32_t)a;
    return ((uint8_t)a << 24) | ((uint8_t)b << 16) | ((uint8_t)c << 8) | (uint8_t)d;
    
}

std::string bin2str(const char *buf, int length) {
    std::ostringstream os;
    char szBuf[32];
    const unsigned char *p = (const unsigned char *)buf;
    const unsigned char *end = p + length;

    while (p < end) {
        snprintf(szBuf, sizeof szBuf, "%02x ", *p++);
        os << szBuf;
    }

    return os.str();
}

std::string bin2str(const std::string &buf) {
    if (buf.empty())
        return "";
    return bin2str(&buf[0], buf.length());        
}

/**
 * 判断指定的字符串是否为数字
 */
//bool isDigits(const char *p) {
//    if (*p == 0)
//        return false;
//    while (*p) {
//        if (!isdigit(*p++))
//            return false;
//    }
//    return true;
//}


std::string encodeURL(const std::string &sUrl)
{
    static char HEX_TABLE[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    std::string result;
    for (size_t i = 0; i < sUrl.length(); i++)
    {
        char c = sUrl[i];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            result.append(1, c);
        }
        else
        {
            result.append(1, '%');
            result.append(1, HEX_TABLE[(c >> 4) & 0x0f]);
            result.append(1, HEX_TABLE[c & 0x0f]);
        }
    }

    return result;
}


uint16_t byteswap_ushort(uint16_t number)
{
#if defined(_MSC_VER) && _MSC_VER > 1310
        return _byteswap_ushort(number);
#elif defined(__GNUC__)
        return __builtin_bswap16(number);
#else
        return (number >> 8) | (number << 8);
#endif
}

// 获取转换为UTF-16 BE的字符串
std::u16string utf8_to_utf16be(const std::string& u8str, bool addbom, bool* ok)
{
	// 先获取utf16le编码字符串
	std::u16string u16str = utf8_to_utf16le(u8str, addbom, ok);
	// 将小端序转换为大端序
	for (size_t i = 0; i < u16str.size(); ++i) {
		u16str[i] = byteswap_ushort(u16str[i]);
	}
	return u16str;
}

// 获取转换为UTF-16 LE编码的字符串
std::u16string utf8_to_utf16le(const std::string& u8str, bool addbom, bool* ok)
{
	std::u16string u16str;
	u16str.reserve(u8str.size());
	if (addbom) {
		u16str.push_back(0xFEFF);	//bom (字节表示为 FF FE)
	}
	std::string::size_type len = u8str.length();

	const unsigned char* p = (unsigned char*)(u8str.data());
	// 判断是否具有BOM(判断长度小于3字节的情况)
	if (len > 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF){
		p += 3;
		len -= 3;
	}

	bool is_ok = true;
	// 开始转换
	for (std::string::size_type i = 0; i < len; ++i) {
		uint32_t ch = p[i];	// 取出UTF8序列首字节
		if ((ch & 0x80) == 0) {
			// 最高位为0，只有1字节表示UNICODE代码点
			u16str.push_back((char16_t)ch);
			continue;
		}
		switch (ch & 0xF0)
		{
    		case 0xF0: // 4 字节字符, 0x10000 到 0x10FFFF
		    {
			    uint32_t c2 = p[++i];
			    uint32_t c3 = p[++i];
			    uint32_t c4 = p[++i];
			    // 计算UNICODE代码点值(第一个字节取低3bit，其余取6bit)
			    uint32_t codePoint = ((ch & 0x07U) << 18) | ((c2 & 0x3FU) << 12) | ((c3 & 0x3FU) << 6) | (c4 & 0x3FU);
			    if (codePoint >= 0x10000)
			    {
				    // 在UTF-16中 U+10000 到 U+10FFFF 用两个16bit单元表示, 代理项对.
				    // 1、将代码点减去0x10000(得到长度为20bit的值)
				    // 2、high 代理项 是将那20bit中的高10bit加上0xD800(110110 00 00000000)
				    // 3、low  代理项 是将那20bit中的低10bit加上0xDC00(110111 00 00000000)
				    codePoint -= 0x10000;
				    u16str.push_back((char16_t)((codePoint >> 10) | 0xD800U));
				    u16str.push_back((char16_t)((codePoint & 0x03FFU) | 0xDC00U));
			    }
			    else
			    {
				    // 在UTF-16中 U+0000 到 U+D7FF 以及 U+E000 到 U+FFFF 与Unicode代码点值相同.
				    // U+D800 到 U+DFFF 是无效字符, 为了简单起见，这里假设它不存在(如果有则不编码)
				    u16str.push_back((char16_t)codePoint);
			    }
		    }
		        break;
		    case 0xE0: // 3 字节字符, 0x800 到 0xFFFF
		    {
			    uint32_t c2 = p[++i];
			    uint32_t c3 = p[++i];
			    // 计算UNICODE代码点值(第一个字节取低4bit，其余取6bit)
			    uint32_t codePoint = ((ch & 0x0FU) << 12) | ((c2 & 0x3FU) << 6) | (c3 & 0x3FU);
			    u16str.push_back((char16_t)codePoint);
		    }
		        break;
		    case 0xD0: // 2 字节字符, 0x80 到 0x7FF
		    case 0xC0:
		    {
			    uint32_t c2 = p[++i];
			    // 计算UNICODE代码点值(第一个字节取低5bit，其余取6bit)
			    uint32_t codePoint = ((ch & 0x1FU) << 12) | ((c2 & 0x3FU) << 6);
			    u16str.push_back((char16_t)codePoint);
		    }
		        break;
		    default:	// 单字节部分(前面已经处理，所以不应该进来)
			    is_ok = false;
			    break;
		}
	}
	if (ok != NULL) { *ok = is_ok; }
	return u16str;
}

bool isContainEmoji(const std::string &str, bool onlyWord)
{
	bool bContain = false;
	std::u16string ut16 = utf8_to_utf16le(str);
    MLOG_DEBUG("utf8 length:"<<str.length()<<" ,utf16 length:"<<ut16.length());
	if (!ut16.empty())
	{
        if(onlyWord && ut16.length() > 2) //仅仅判断单个字符是否是表情
        {
            MLOG_DEBUG("the length is more than one emoji, not hit");
            return bContain;
        }
        size_t len = ut16.length();
		for (size_t i = 0; i < len; ++i)
		{
			char16_t hs = ut16[i];
			if (0xd800 <= hs && hs <= 0xdbff)
			{
				if (ut16.length() > (i + 1))
				{
					char16_t ls = ut16[i + 1];
					int uc = ((hs - 0xd800) * 0x400) + (ls - 0xdc00) + 0x10000;
					if (0x1d000 <= uc && uc <= 0x1f77f)
					{
						bContain = true;
						break;
					}
				}
			}
			else
			{
				if (0x2100 <= hs && hs <= 0x27ff)
				{
					bContain = true;
				}
				else if (0x2b05 <= hs && hs <= 0x2b07)
				{
					bContain = true;
				}
				else if (0x2934 <= hs && hs <= 0x2935)
				{
					bContain = true;
				}
				else if (0x3297 <= hs && hs <= 0x3299)
				{
					bContain = true;
				}
				else if (hs == 0xa9 || hs == 0xae || hs == 0x303d || hs == 0x3030 || hs == 0x2b55 || hs == 0x2b1c || hs == 0x2b1b || hs == 0x2b50)
				{
					bContain = true;
				}
				else if (hs >= 0x2600 && hs <= 0x27BF) // 杂项符号与符号字体
				{
					bContain = true;
				}
				else if (hs == 0x303D || hs == 0x2049 || hs == 0x203C || hs == 0x205F)
				{
					bContain = true;
				}
				else if (hs >= 0x2000 && hs <= 0x200F)//
				{
					bContain = true;
				}
				else if (hs >= 0x2028 && hs <= 0x202F)//
				{
					bContain = true;
				}
				else if (hs >= 0x2065 && hs <= 0x206F)
				{
					bContain = true;
				}
				/* 标点符号占用区域 */
				else if (hs >= 0x2100 && hs <= 0x214F)// 字母符号
				{
					bContain = true;
				}
				else if (hs >= 0x2300 && hs <= 0x23FF)// 各种技术符号
				{
					bContain = true;
				}
				else if (hs >= 0x2B00 && hs <= 0x2BFF)// 箭头A
                {
					bContain = true;
                }
				else if (hs >= 0x2900 && hs <= 0x297F)// 箭头B
				{
                    bContain = true;
				}	
                else if (hs >= 0x3200 && hs <= 0x32FF)// 中文符号
				{
                    bContain = true;
				}
                else if (hs >= 0xD800 && hs <= 0xDFFF)// 高低位替代符保留区域
			    {
                    bContain = true;
				}    
                else if (hs >= 0xE000 && hs <= 0xF8FF)// 私有保留区域
				{
                    bContain = true;
				}
                else if (hs >= 0xFE00 && hs <= 0xFE0F)// 变异选择器
				{
                    bContain = true;
				}
                else if (hs >= 0x10000)
				{
                    bContain = true;
                }
			}
		}
	}
	return bContain;
}

bool startWith(const std::string &str,const std::string &head)
{
    return str.compare(0,head.size(),head) == 0;
}
bool endWith(const std::string &str,const std::string &tail)
{
    return str.compare(str.size() - tail.size(), tail.size(), tail) == 0;
}

};
