#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include <vector>
#include <sstream>
#include <memory>
#include "util/tc_common.h"

using namespace std;
using namespace rapidjson;
using namespace tars;


class JsonUtil {
public:

    /**
     * 
     */
    static bool parseObj(const string &str, Document &doc);

    /**
     * 
     */
    static bool parseArray(const string &str, Document &v);

    /**
     * 
     */
    static std::shared_ptr<Document> parseObj(const string &str);

    /**
     * 
     */
    static std::shared_ptr<Document> parseArray(const string &str);

    /**
     * 
     */
    static bool getInt(const Value &doc, const char *name, int &v);

    /**
     * 
     */
    static bool getLong(const Value &doc, const char *name, long &v);

    /**
     * 
     */
    static bool getBool(const Value &doc, const char *name, bool &v);

    /**
     * 
     */
    static bool getStr(const Value &doc, const char *name, string &v);

    /**
     * 
     */
    static bool getLongs(const Value &doc, const char *name, vector<long> &v);

    /**
     * 
     */
    static bool getStrs(const Value &doc, const char *name, vector<string> &v);

    /**
     *
     */
    static bool getObj(const Value &doc, const char *name, const Value *&v);

    /**
     *
     */
    static bool getObjs(const Value &doc, const char *name, std::vector<const Value*> &v);

    /**
     *
     */
    static bool getObjs(const Value &doc, std::vector<const Value*> &v);

    /**
     *
     */
    static bool getArray(const Value &doc, const char *name, const Value *&v);

    /**
     *
     */
    static bool getArrayStr(const Value &doc, const char *name, string &str);

    /**
     *
     */
    static std::string getOutput(const Value &doc);

    /**
     * 
     */
    template <typename T>
    static vector<T> convertFromString(const vector<string> &v) {
        vector<T> vResult;
        for (auto item : v) {
            vResult.push_back(TC_Common::strto<T>(item));
        }
        return vResult;
    }

    /**
     * 
     */
    template <typename T>
    static vector<string> convertToString(const vector<T> &v) {
        vector<string> vResult;
        for (auto item : v) {
            vResult.push_back(TC_Common::tostr<T>(item));
        }
        return vResult;
    }

    template <typename T>
    static std::string getCommaString(const std::vector<T> &v) {
        std::ostringstream os;
        for (int i = 0; i < (int)v.size(); i++) {
            if (i > 0)
                os << ',';
            os << v[i];
        }
        return os.str();
    }
    /*
     *
     */
    static bool getDouble(const Value &doc, const char *name, double &v);

    /**
     * 
     */
    static bool getDoubleV2(const Value &doc, const char *name, double &v);
    /**
     *
     */
    static std::string mapToJson(std::map<std::string,std::string> map);

    /**
     *
     */
    static std::string encode(const std::string &data);

    static std::string encodeNoSlanting(const std::string &data);
};

