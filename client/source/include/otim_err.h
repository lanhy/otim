// **********************************************************************
// This file was generated by a TARS parser!
// TARS version 3.0.13.
// **********************************************************************

#ifndef __OTIM_ERR_H_
#define __OTIM_ERR_H_

#include <map>
#include <string>
#include <vector>
#ifdef __APPLE__
#include "Tars.h"
#include "TarsJson.h"
#elif defined ANDROID  // android
#include "Tars.h"
#include "TarsJson.h"
#else
#include "tup/Tars.h"
#include "tup/TarsJson.h"
#endif

using namespace std;


namespace otim
{
    enum ERROR_CODE
    {
        EC_SUCCESS = 0,
        EC_PROTOCOL = 1,
        EC_PARAM = 2,
        EC_UNKOWN = 3,
        EC_DB_ERROR = 5,
        EC_SERVICE_UNAVAILABLE = 6,
        EC_SERVER_REFUSED = 7,
        EC_SERVER_EXCEPTION = 8,
        EC_MEM_NOT_ENOUGH = 9,
        EC_SENSI_MATCH = 10,
        EC_REDIS_ERROR = 11,
        EC_CONN_INDICATE = 20,
        EC_CONN_USERNAME_OR_PASSWORD = 21,
        EC_CONN_KICKOUT = 22,
        EC_CONN_ACCESS_DENIED = 23,
        EC_CONN_THRIDS_REJECT = 24,
        EC_HOTSESSION_MORE = 25,
        EC_HOTSESSION_FAILED = 26,
        EC_HISTORYMSG_ERROR = 27,
        EC_HISTORYMSG_DENY = 28,
        EC_GROUP_USER_NOTEXIST = 30,
        EC_GROUP_ID_EMPTY = 31,
        EC_GROUP_EMPTY = 32,
        EC_GROUP_NO_PRIVILEGE = 33,
        EC_GROUP_MEMBER_LIMIT = 34,
        EC_GROUP_NOT_EXIST = 35,
        EC_GROUP_CREAGOR_ERROR = 36,
        EC_MSG_TOO_SHORT = 40,
        EC_MSG_TOO_LONG = 41,
        EC_MSG_INVALID_FORMAT = 42,
        EC_MSG_FOBIDDEN = 43,
        EC_MSG_NOT_EXIST = 44,
        EC_MSG_TIMEOUT = 45,
        EC_MSG_REPEATED = 46,
        EC_MSG_OP_CMD = 47,
        EC_USER_RELATION_INVALID = 50,
        EC_USER_BLACKLISt = 51,
        EC_USER_VERSION = 52,
        EC_ATTR_FIELD_EMPTY = 60,
        EC_ATTR_VALUE_EMPTY = 61,
    };
    inline string etos(const ERROR_CODE & e)
    {
        switch(e)
        {
            case EC_SUCCESS: return "EC_SUCCESS";
            case EC_PROTOCOL: return "EC_PROTOCOL";
            case EC_PARAM: return "EC_PARAM";
            case EC_UNKOWN: return "EC_UNKOWN";
            case EC_DB_ERROR: return "EC_DB_ERROR";
            case EC_SERVICE_UNAVAILABLE: return "EC_SERVICE_UNAVAILABLE";
            case EC_SERVER_REFUSED: return "EC_SERVER_REFUSED";
            case EC_SERVER_EXCEPTION: return "EC_SERVER_EXCEPTION";
            case EC_MEM_NOT_ENOUGH: return "EC_MEM_NOT_ENOUGH";
            case EC_SENSI_MATCH: return "EC_SENSI_MATCH";
            case EC_REDIS_ERROR: return "EC_REDIS_ERROR";
            case EC_CONN_INDICATE: return "EC_CONN_INDICATE";
            case EC_CONN_USERNAME_OR_PASSWORD: return "EC_CONN_USERNAME_OR_PASSWORD";
            case EC_CONN_KICKOUT: return "EC_CONN_KICKOUT";
            case EC_CONN_ACCESS_DENIED: return "EC_CONN_ACCESS_DENIED";
            case EC_CONN_THRIDS_REJECT: return "EC_CONN_THRIDS_REJECT";
            case EC_HOTSESSION_MORE: return "EC_HOTSESSION_MORE";
            case EC_HOTSESSION_FAILED: return "EC_HOTSESSION_FAILED";
            case EC_HISTORYMSG_ERROR: return "EC_HISTORYMSG_ERROR";
            case EC_HISTORYMSG_DENY: return "EC_HISTORYMSG_DENY";
            case EC_GROUP_USER_NOTEXIST: return "EC_GROUP_USER_NOTEXIST";
            case EC_GROUP_ID_EMPTY: return "EC_GROUP_ID_EMPTY";
            case EC_GROUP_EMPTY: return "EC_GROUP_EMPTY";
            case EC_GROUP_NO_PRIVILEGE: return "EC_GROUP_NO_PRIVILEGE";
            case EC_GROUP_MEMBER_LIMIT: return "EC_GROUP_MEMBER_LIMIT";
            case EC_GROUP_NOT_EXIST: return "EC_GROUP_NOT_EXIST";
            case EC_GROUP_CREAGOR_ERROR: return "EC_GROUP_CREAGOR_ERROR";
            case EC_MSG_TOO_SHORT: return "EC_MSG_TOO_SHORT";
            case EC_MSG_TOO_LONG: return "EC_MSG_TOO_LONG";
            case EC_MSG_INVALID_FORMAT: return "EC_MSG_INVALID_FORMAT";
            case EC_MSG_FOBIDDEN: return "EC_MSG_FOBIDDEN";
            case EC_MSG_NOT_EXIST: return "EC_MSG_NOT_EXIST";
            case EC_MSG_TIMEOUT: return "EC_MSG_TIMEOUT";
            case EC_MSG_REPEATED: return "EC_MSG_REPEATED";
            case EC_MSG_OP_CMD: return "EC_MSG_OP_CMD";
            case EC_USER_RELATION_INVALID: return "EC_USER_RELATION_INVALID";
            case EC_USER_BLACKLISt: return "EC_USER_BLACKLISt";
            case EC_USER_VERSION: return "EC_USER_VERSION";
            case EC_ATTR_FIELD_EMPTY: return "EC_ATTR_FIELD_EMPTY";
            case EC_ATTR_VALUE_EMPTY: return "EC_ATTR_VALUE_EMPTY";
            default: return "";
        }
    }
    inline int stoe(const string & s, ERROR_CODE & e)
    {
        if(s == "EC_SUCCESS")  { e=EC_SUCCESS; return 0;}
        if(s == "EC_PROTOCOL")  { e=EC_PROTOCOL; return 0;}
        if(s == "EC_PARAM")  { e=EC_PARAM; return 0;}
        if(s == "EC_UNKOWN")  { e=EC_UNKOWN; return 0;}
        if(s == "EC_DB_ERROR")  { e=EC_DB_ERROR; return 0;}
        if(s == "EC_SERVICE_UNAVAILABLE")  { e=EC_SERVICE_UNAVAILABLE; return 0;}
        if(s == "EC_SERVER_REFUSED")  { e=EC_SERVER_REFUSED; return 0;}
        if(s == "EC_SERVER_EXCEPTION")  { e=EC_SERVER_EXCEPTION; return 0;}
        if(s == "EC_MEM_NOT_ENOUGH")  { e=EC_MEM_NOT_ENOUGH; return 0;}
        if(s == "EC_SENSI_MATCH")  { e=EC_SENSI_MATCH; return 0;}
        if(s == "EC_REDIS_ERROR")  { e=EC_REDIS_ERROR; return 0;}
        if(s == "EC_CONN_INDICATE")  { e=EC_CONN_INDICATE; return 0;}
        if(s == "EC_CONN_USERNAME_OR_PASSWORD")  { e=EC_CONN_USERNAME_OR_PASSWORD; return 0;}
        if(s == "EC_CONN_KICKOUT")  { e=EC_CONN_KICKOUT; return 0;}
        if(s == "EC_CONN_ACCESS_DENIED")  { e=EC_CONN_ACCESS_DENIED; return 0;}
        if(s == "EC_CONN_THRIDS_REJECT")  { e=EC_CONN_THRIDS_REJECT; return 0;}
        if(s == "EC_HOTSESSION_MORE")  { e=EC_HOTSESSION_MORE; return 0;}
        if(s == "EC_HOTSESSION_FAILED")  { e=EC_HOTSESSION_FAILED; return 0;}
        if(s == "EC_HISTORYMSG_ERROR")  { e=EC_HISTORYMSG_ERROR; return 0;}
        if(s == "EC_HISTORYMSG_DENY")  { e=EC_HISTORYMSG_DENY; return 0;}
        if(s == "EC_GROUP_USER_NOTEXIST")  { e=EC_GROUP_USER_NOTEXIST; return 0;}
        if(s == "EC_GROUP_ID_EMPTY")  { e=EC_GROUP_ID_EMPTY; return 0;}
        if(s == "EC_GROUP_EMPTY")  { e=EC_GROUP_EMPTY; return 0;}
        if(s == "EC_GROUP_NO_PRIVILEGE")  { e=EC_GROUP_NO_PRIVILEGE; return 0;}
        if(s == "EC_GROUP_MEMBER_LIMIT")  { e=EC_GROUP_MEMBER_LIMIT; return 0;}
        if(s == "EC_GROUP_NOT_EXIST")  { e=EC_GROUP_NOT_EXIST; return 0;}
        if(s == "EC_GROUP_CREAGOR_ERROR")  { e=EC_GROUP_CREAGOR_ERROR; return 0;}
        if(s == "EC_MSG_TOO_SHORT")  { e=EC_MSG_TOO_SHORT; return 0;}
        if(s == "EC_MSG_TOO_LONG")  { e=EC_MSG_TOO_LONG; return 0;}
        if(s == "EC_MSG_INVALID_FORMAT")  { e=EC_MSG_INVALID_FORMAT; return 0;}
        if(s == "EC_MSG_FOBIDDEN")  { e=EC_MSG_FOBIDDEN; return 0;}
        if(s == "EC_MSG_NOT_EXIST")  { e=EC_MSG_NOT_EXIST; return 0;}
        if(s == "EC_MSG_TIMEOUT")  { e=EC_MSG_TIMEOUT; return 0;}
        if(s == "EC_MSG_REPEATED")  { e=EC_MSG_REPEATED; return 0;}
        if(s == "EC_MSG_OP_CMD")  { e=EC_MSG_OP_CMD; return 0;}
        if(s == "EC_USER_RELATION_INVALID")  { e=EC_USER_RELATION_INVALID; return 0;}
        if(s == "EC_USER_BLACKLISt")  { e=EC_USER_BLACKLISt; return 0;}
        if(s == "EC_USER_VERSION")  { e=EC_USER_VERSION; return 0;}
        if(s == "EC_ATTR_FIELD_EMPTY")  { e=EC_ATTR_FIELD_EMPTY; return 0;}
        if(s == "EC_ATTR_VALUE_EMPTY")  { e=EC_ATTR_VALUE_EMPTY; return 0;}

        return -1;
    }


}



#endif
