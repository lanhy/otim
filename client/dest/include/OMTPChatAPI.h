#ifndef _CHATAPI_H
#define _CHATAPI_H

#include <string>
#include <stdint.h>
#include <map>
#include "OMTPHostInfo.h"

using namespace std;

namespace otim {
class HotSessionResp;
class OffMsgResp;
class MsgReq;
class HotSessionResp;
class MsgAck;
}

namespace otim {
    
    
    
    /**
     * OMTP回调接口
     */
    class IOtimSdkCallback
    {
    public:
        virtual ~IOtimSdkCallback(){}
        
    
        virtual void netStatusChanged(int32_t status) = 0;
   
        virtual void loginResp(int32_t code, const std::string &clientId) = 0;
    
   
        virtual void kickOut() = 0;
   
        virtual void hotSessionResp(otim::HotSessionResp* hotSessions) = 0;
     
        virtual void msgRecv(int type, const std::string &packId, otim::MsgReq* req) = 0;
   
        virtual void msgAck(const std::string &packId, otim::MsgAck* ack) = 0;     
    };
    
    class IOtimSdk
    {
    public:
        virtual ~IOtimSdk(){}
        
        /**
         * 增加IM 服务器ip地址和端口
         * @param host ip地址
         * @param port 端口
         * @param isSSL 是否支持ssl连接
         */
        virtual void addHostInfo(const std::string&  host, int32_t port, bool isSSL) = 0;
        
        /**
         * 设置SDK 回调指针
         * @param callback 回调接口指针
         */
        virtual void setCallback(IOtimSdkCallback* callback) = 0;
        /**
         * 登录
         * @param name 用户名，可以不填写
         * @param password 密码，必须填写token
         */
        virtual int32_t login (const std::string&  name, const std::string&  password) = 0;
        /**
         * 登出
         * @param notifyServer 是否向server发送数据报文
         */
        virtual int32_t logout (bool notifyServer) = 0;
        /**
         * 发送消息
         * @param message 消息体
         */

        virtual int32_t sendMessage(int type, otim::MsgReq& req) = 0;

        /**
         * 获取网络状态
         */
        virtual int32_t getNetStatus() = 0;
        
        //获取当前clientId
        virtual std::string getClientId() = 0;
        
        /**
            * 请求用户信息
            * @param userIds 用户Id列表
            */
        virtual void reqUserinfosFromServer(std::vector<std::string> userIds) = 0;
        
     };
    
    
    /**
     * 日志记录接口
     */
    class IOtimLog{
    public:
        virtual ~IOtimLog(){}

        /**
         * 写日志
         * @param logs 日志内容
         */
        virtual void writeLog(const std::string& logs) = 0;
        
        /**
         * 获取日志文件名称及路径
         * @return 日志文件路径及名称 外部必须释放 返回字符串内存
         */
        virtual const std::string& getLogFileName() = 0;
        
    };
    
    /**
     * 初始化日志模块
     * @param appPath 日志保存路径
     */
    void initLog(const std::string& appPath);
    /**
     * 获取日志接口实例，调用此接口前必须调用 initLog
     */
    IOtimLog* getLogInstance();
    
    /**
     * 初始化IM 模块
     * @param clientInfo IM 所需要的参数
     */
    IOtimSdk* initIm(TNClientInfo &clientInfo);
    
  
    /**
     * 获取IMSDK IM模块实例
     * 调用此函数前必须调用 initIm接口
     */
    IOtimSdk* getImSDK();

}

#define SDK_LOG(logstr) try{   std::stringstream stream; stream <<" ["<<CLog::getCurrentThreadID()<<"]"<<CLog::getTimeStr()<<logstr; getLogInstance()->writeLog(stream); }catch(...){std::cout<<"++++Excecption occur on LOG++++++++"<<std::endl;}

#endif
