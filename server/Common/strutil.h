#pragma once 

#include <string>
#include <vector>
#include <stdint.h>
#include <map>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

namespace hcommon
{
	std::vector<std::string> string_split(const std::string &strSource, const char &chSplitChar);

	std::string string_concat(const std::vector<std::string> &strv, const std::string &c = " ");

	std::string string_trim(const std::string &str, const char c = ' ');

	std::string& replace_all(std::string& str,const std::string& old_value,const std::string& new_value);

	uint64_t str_to_UINT64(const std::string &strNumber);

	int32_t str_to_INT32(const std::string &strNumber);

	//目前最多转换16384字节(大约5461个中文字)的源数据
	bool utf8_to_utf32(const std::string& input, std::u32string &output);

	//s为需要进行定位的UTF8字符串
	//输入:wantedNum为s中以UTF8字符为单位的位置
	//输出:offset为s中以字节为单位的实际换算后的位置
	uint32_t get_utf8_word(const char *s, uint32_t& offset, uint32_t wantedNum);

	uint32_t get_utf8_length(const std::string &input);
        std::string json2string(rapidjson::Value & value);

	std::string sub_string(const std::string &str, int length);
    
    uint32_t ipToLong(const char * ip, uint32_t &prefix);
    
	/**
	 * 将二进制数据，转成ascii格式的字符串，方便调试
	 */
	std::string bin2str(const char *buf, int length);
	std::string bin2str(const std::string &buf);

    // 获取转换为UTF-16 LE编码的字符串
    std::u16string utf8_to_utf16le(const std::string& u8str, bool addbom = false, bool* ok = NULL);
    
    // 获取转换为UTF-16 BE的字符串
    std::u16string utf8_to_utf16be(const std::string& u8str, bool addbom = false, bool* ok = NULL);
    
    // 判断字符是否包含表情
    bool isContainEmoji(const std::string &str, bool onlyWord);

    //startwith endwith
    bool startWith(const std::string &str,const std::string &head);
    bool endWith(const std::string &str,const std::string &tail);

 };

