#include "JsonUtil.h"
#include "strutil.h"

bool JsonUtil::parseObj(const string &str, Document &doc) {
    doc.Parse(str.c_str());
    if (doc.HasParseError())
        return false;
    if (!doc.IsObject())
        return false;
    return true;
}

bool JsonUtil::parseArray(const string &str, Document &doc) {
    doc.Parse(str.c_str());
    if (doc.HasParseError())
        return false;
    if (!doc.IsArray())
        return false;
    return true;
}

std::shared_ptr<Document> JsonUtil::parseObj(const string &str) {
    std::shared_ptr<Document> d = std::make_shared<Document>();
    if (parseObj(str, *d))
        return d;
    return nullptr;
}

std::shared_ptr<Document> JsonUtil::parseArray(const string &str) {
    std::shared_ptr<Document> d = std::make_shared<Document>();
    if (parseArray(str, *d))
        return d;
    return nullptr;
}

bool JsonUtil::getInt(const Value &doc, const char *name, int &v) {
    if (doc.HasMember(name) && doc[name].IsInt()) {
        v = doc[name].GetInt();
        return true;
    }
    return false;
}

bool JsonUtil::getLong(const Value &doc, const char *name, long &v) {
    if (doc.HasMember(name) && doc[name].IsInt64()) {
        v = doc[name].GetInt64();
        return true;
    }
    return false;
}

bool JsonUtil::getBool(const Value &doc, const char *name, bool &v) {
    if (doc.HasMember(name) && doc[name].IsBool()) {
        v = doc[name].GetBool();
        return true;
    }
    return false;
}

bool JsonUtil::getStr(const Value &doc, const char *name, string &v) {
    if (doc.HasMember(name) && doc[name].IsString()) {
        v = doc[name].GetString();
        return true;
    }
    return false;
}

bool JsonUtil::getDouble(const Value &doc, const char *name, double &v) {
    if (doc.HasMember(name) && doc[name].IsDouble()) {
        v = doc[name].GetDouble();
        return true;
    }
    return false;
}

bool JsonUtil::getDoubleV2(const Value &doc, const char *name, double &v) {
    if (doc.HasMember(name) && doc[name].IsNumber()) {
        v = doc[name].GetDouble();
        return true;
    }
    return false;
}

bool JsonUtil::getLongs(const Value &doc, const char *name, vector<long> &v) {
    v.clear();
    if (doc.HasMember(name) && doc[name].IsArray()) {
        auto ar = doc[name].GetArray();
        int count = ar.Size();
        for (int i = 0; i < count; i++) {
            if (ar[i].IsInt() || ar[i].IsInt64()) {
                v.push_back(ar[i].GetInt64());
            } else {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool JsonUtil::getStrs(const Value &doc, const char *name, vector<string> &v) {
    v.clear();
    if (doc.HasMember(name) && doc[name].IsArray()) {
        auto ar = doc[name].GetArray();
        int count = ar.Size();
        for (int i = 0; i < count; i++) {
            if (ar[i].IsString()) {
                v.push_back(ar[i].GetString());
            } else {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool JsonUtil::getObj(const Value &doc, const char *name, const Value * &v) {
    if (doc.HasMember(name)&& doc[name].IsObject()) {
        v = &doc[name];
        return true;
    }
    return false;
}

bool JsonUtil::getObjs(const Value &doc, const char *name, std::vector<const Value*> &v) {
    v.clear();
    if (name == nullptr) {
        return getObjs(doc, v);
    }

    if (doc.HasMember(name) && doc[name].IsArray()) {
        const Value &ar = doc[name];
        int count = ar.Size();
        for (int i = 0; i < count; i++) {
            if (ar[i].IsObject()) {
                v.push_back(&ar[i]);
            } else {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool JsonUtil::getObjs(const Value &doc, std::vector<const Value*> &v) {
    v.clear();
    if (doc.IsArray()) {
        int count = doc.Size();
        for (int i = 0; i < count; i++) {
            if (doc[i].IsObject()) {
                v.push_back(&doc[i]);
            } else {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool JsonUtil::getArray(const Value &doc, const char *name, const Value *&v) {
    if (doc.HasMember(name) && doc[name].IsArray()) {
        v = &doc[name];
        return true;
    }
    return false;
}

bool JsonUtil::getArrayStr(const Value &doc, const char *name, string &str) {
    str = "";
    const Value *value = nullptr;
    if (!getArray(doc, name, value))
        return false;
    if (value) {
        str = getOutput(*value);
        return true;
    }
    return false;
}

std::string JsonUtil::getOutput(const Value &doc) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    doc.Accept(writer);
    return sb.GetString();
}

std::string JsonUtil::mapToJson(std::map<std::string,std::string> map)
{
    rapidjson::Document root(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& alloc = root.GetAllocator();
    std::map<std::string,std::string>::iterator it;
    for(it = map.begin();it != map.end(); it++)
    {
        std::string name = it->first;
        std::string value = it->second;
        if(!name.empty() && !value.empty())
        {
            root.AddMember(rapidjson::Value::StringRefType(name.c_str()), rapidjson::Value::StringRefType(value.c_str()), alloc);
        }
    }
    
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    return sb.GetString(); 
}

/*
 *转义: \->\\ ,\n->\\n ,\r->\\r, \t->\\t,"->\",\b->\\b,\f->\\f
 */
 std::string JsonUtil::encode(const std::string &data)
 {
    std::string input = data;
    hcommon::replace_all(input, "\\", "\\\\");
    hcommon::replace_all(input, "\"", "\\\"");
    
    hcommon::replace_all(input, "\r", "\\r");
    hcommon::replace_all(input, "\n", "\\n");
    hcommon::replace_all(input, "\f", "\\f");
    hcommon::replace_all(input, "\b", "\\b");
    hcommon::replace_all(input, "\t", "\\t");
    return input;
 }

 std::string JsonUtil::encodeNoSlanting(const std::string &data)
 {
    std::string input = data;
    hcommon::replace_all(input, "\"", "\\\"");
    hcommon::replace_all(input, "\r", "\\r");
    hcommon::replace_all(input, "\n", "\\n");
    hcommon::replace_all(input, "\f", "\\f");
    hcommon::replace_all(input, "\b", "\\b");
    hcommon::replace_all(input, "\t", "\\t");
    return input;
 }
