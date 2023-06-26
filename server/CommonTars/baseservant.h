// **********************************************************************
// This file was generated by a TARS parser!
// TARS version 3.0.13.
// **********************************************************************

#ifndef __BASESERVANT_H_
#define __BASESERVANT_H_

#include <map>
#include <string>
#include <vector>
#include "tup/Tars.h"
#include "tup/TarsJson.h"
using namespace std;
#include "otim.h"
#include "servant/ServantProxy.h"
#include "servant/Servant.h"
#include "promise/promise.h"
#include "servant/Application.h"


namespace otim
{

    /* callback of async proxy for client */
    class BaseServantPrxCallback: public tars::ServantProxyCallback
    {
    public:
        virtual ~BaseServantPrxCallback(){}
        virtual void callback_request(tars::Int32 ret,  const otim::OTIMPack& resp)
        { throw std::runtime_error("callback_request() override incorrect."); }
        virtual void callback_request_exception(tars::Int32 ret)
        { throw std::runtime_error("callback_request_exception() override incorrect."); }
        virtual void callback_request(tars::Int32 ret, otim::OTIMPack&&  resp)
        { callback_request(ret, resp); }

    public:
        virtual const map<std::string, std::string> & getResponseContext() const
        {
            CallbackThreadData * pCbtd = CallbackThreadData::getData();
            assert(pCbtd != NULL);

            if(!pCbtd->getContextValid())
            {
                throw TC_Exception("cann't get response context");
            }
            return pCbtd->getResponseContext();
        }

    public:
        virtual int onDispatch(tars::ReqMessagePtr _msg_)
        {
            static ::std::string __BaseServant_all[]=
            {
                "request"
            };
            auto it = _msg_->response->status.find("TARS_FUNC");
            pair<string*, string*> r = equal_range(__BaseServant_all, __BaseServant_all+1, (it==_msg_->response->status.end())?_msg_->request.sFuncName:it->second);
            if(r.first == r.second) return tars::TARSSERVERNOFUNCERR;
            switch(r.first - __BaseServant_all)
            {
                case 0:
                {
                    if (_msg_->response->iRet != tars::TARSSERVERSUCCESS)
                    {
                        callback_request_exception(_msg_->response->iRet);

                        return _msg_->response->iRet;
                    }
                    tars::TarsInputStream<tars::BufferReader> _is;

                    _is.setBuffer(_msg_->response->sBuffer);
                    tars::Int32 _ret;
                    _is.read(_ret, 0, true);

                    otim::OTIMPack resp;
                    _is.read(resp, 3, true);
                    ServantProxyThreadData *_pSptd_ = ServantProxyThreadData::getData();
                    if (_pSptd_ && _pSptd_->_traceCall)
                    {
                        string _trace_param_;
                        int _trace_param_flag_ = _pSptd_->needTraceParam(ServantProxyThreadData::TraceContext::EST_CR, _is.size());
                        if (ServantProxyThreadData::TraceContext::ENP_NORMAL == _trace_param_flag_)
                        {
                            tars::JsonValueObjPtr _p_ = new tars::JsonValueObj();
                            _p_->value[""] = tars::JsonOutput::writeJson(_ret);
                            _p_->value["resp"] = tars::JsonOutput::writeJson(resp);
                            _trace_param_ = tars::TC_Json::writeValue(_p_);
                        }
                        else if(ServantProxyThreadData::TraceContext::ENP_OVERMAXLEN == _trace_param_flag_)
                        {
                            _trace_param_ = "{\"trace_param_over_max_len\":true}";
                        }
                        TARS_TRACE(_pSptd_->getTraceKey(ServantProxyThreadData::TraceContext::EST_CR), TRACE_ANNOTATION_CR, "", ServerConfig::Application + "." + ServerConfig::ServerName, "request", 0, _trace_param_, "");
                    }

                    CallbackThreadData * pCbtd = CallbackThreadData::getData();
                    assert(pCbtd != NULL);

                    pCbtd->setResponseContext(_msg_->response->context);

                    callback_request(_ret, std::move(resp));

                    pCbtd->delResponseContext();

                    return tars::TARSSERVERSUCCESS;

                }
            }
            return tars::TARSSERVERNOFUNCERR;
        }

    };
    typedef tars::TC_AutoPtr<BaseServantPrxCallback> BaseServantPrxCallbackPtr;

    //callback of promise async proxy for client
    class BaseServantPrxCallbackPromise: public tars::ServantProxyCallback
    {
    public:
        virtual ~BaseServantPrxCallbackPromise(){}
    public:
        struct Promiserequest: virtual public TC_HandleBase
        {
        public:
            tars::Int32 _ret;
            otim::OTIMPack resp;
            map<std::string, std::string> _mRspContext;
        };
        
        typedef tars::TC_AutoPtr< BaseServantPrxCallbackPromise::Promiserequest > PromiserequestPtr;

        BaseServantPrxCallbackPromise(const tars::Promise< BaseServantPrxCallbackPromise::PromiserequestPtr > &promise)
        : _promise_request(promise)
        {}
        
        virtual void callback_request(const BaseServantPrxCallbackPromise::PromiserequestPtr &ptr)
        {
            _promise_request.setValue(ptr);
        }
        virtual void callback_request_exception(tars::Int32 ret)
        {
            std::string str("");
            str += "Function:request_exception|Ret:";
            str += TC_Common::tostr(ret);
            _promise_request.setException(tars::copyException(str, ret));
        }

    protected:
        tars::Promise< BaseServantPrxCallbackPromise::PromiserequestPtr > _promise_request;

    public:
        virtual int onDispatch(tars::ReqMessagePtr _msg_)
        {
            static ::std::string __BaseServant_all[]=
            {
                "request"
            };

            pair<string*, string*> r = equal_range(__BaseServant_all, __BaseServant_all+1, string(_msg_->request.sFuncName));
            if(r.first == r.second) return tars::TARSSERVERNOFUNCERR;
            switch(r.first - __BaseServant_all)
            {
                case 0:
                {
                    if (_msg_->response->iRet != tars::TARSSERVERSUCCESS)
                    {
                        callback_request_exception(_msg_->response->iRet);

                        return _msg_->response->iRet;
                    }
                    tars::TarsInputStream<tars::BufferReader> _is;

                    _is.setBuffer(_msg_->response->sBuffer);

                    BaseServantPrxCallbackPromise::PromiserequestPtr ptr = new BaseServantPrxCallbackPromise::Promiserequest();

                    try
                    {
                        _is.read(ptr->_ret, 0, true);

                        _is.read(ptr->resp, 3, true);
                    }
                    catch(std::exception &ex)
                    {
                        callback_request_exception(tars::TARSCLIENTDECODEERR);

                        return tars::TARSCLIENTDECODEERR;
                    }
                    catch(...)
                    {
                        callback_request_exception(tars::TARSCLIENTDECODEERR);

                        return tars::TARSCLIENTDECODEERR;
                    }

                    ptr->_mRspContext = _msg_->response->context;

                    callback_request(ptr);

                    return tars::TARSSERVERSUCCESS;

                }
            }
            return tars::TARSSERVERNOFUNCERR;
        }

    };
    typedef tars::TC_AutoPtr<BaseServantPrxCallbackPromise> BaseServantPrxCallbackPromisePtr;

    /* callback of coroutine async proxy for client */
    class BaseServantCoroPrxCallback: public BaseServantPrxCallback
    {
    public:
        virtual ~BaseServantCoroPrxCallback(){}
    public:
        virtual const map<std::string, std::string> & getResponseContext() const { return _mRspContext; }

        virtual void setResponseContext(const map<std::string, std::string> &mContext) { _mRspContext = mContext; }

    public:
        int onDispatch(tars::ReqMessagePtr _msg_)
        {
            static ::std::string __BaseServant_all[]=
            {
                "request"
            };

            pair<string*, string*> r = equal_range(__BaseServant_all, __BaseServant_all+1, string(_msg_->request.sFuncName));
            if(r.first == r.second) return tars::TARSSERVERNOFUNCERR;
            switch(r.first - __BaseServant_all)
            {
                case 0:
                {
                    if (_msg_->response->iRet != tars::TARSSERVERSUCCESS)
                    {
                        callback_request_exception(_msg_->response->iRet);

                        return _msg_->response->iRet;
                    }
                    tars::TarsInputStream<tars::BufferReader> _is;

                    _is.setBuffer(_msg_->response->sBuffer);
                    try
                    {
                        tars::Int32 _ret;
                        _is.read(_ret, 0, true);

                        otim::OTIMPack resp;
                        _is.read(resp, 3, true);
                        setResponseContext(_msg_->response->context);

                        callback_request(_ret, std::move(resp));

                    }
                    catch(std::exception &ex)
                    {
                        callback_request_exception(tars::TARSCLIENTDECODEERR);

                        return tars::TARSCLIENTDECODEERR;
                    }
                    catch(...)
                    {
                        callback_request_exception(tars::TARSCLIENTDECODEERR);

                        return tars::TARSCLIENTDECODEERR;
                    }

                    return tars::TARSSERVERSUCCESS;

                }
            }
            return tars::TARSSERVERNOFUNCERR;
        }

    protected:
        map<std::string, std::string> _mRspContext;
    };
    typedef tars::TC_AutoPtr<BaseServantCoroPrxCallback> BaseServantCoroPrxCallbackPtr;

    /* proxy for client */
    class BaseServantProxy : public tars::ServantProxy
    {
    public:
        typedef map<string, string> TARS_CONTEXT;
        tars::Int32 request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,const map<string, string> &context = TARS_CONTEXT(),map<string, string> * pResponseContext = NULL)
        {
            tars::TarsOutputStream<tars::BufferWriterVector> _os;
            _os.write(clientContext, 1);
            _os.write(req, 2);
            _os.write(resp, 3);
            ServantProxyThreadData *_pSptd_ = ServantProxyThreadData::getData();
            if (_pSptd_ && _pSptd_->_traceCall)
            {
                _pSptd_->newSpan();
                string _trace_param_;
                int _trace_param_flag_ = _pSptd_->needTraceParam(ServantProxyThreadData::TraceContext::EST_CS, _os.getLength());
                if (ServantProxyThreadData::TraceContext::ENP_NORMAL == _trace_param_flag_)
                {
                    tars::JsonValueObjPtr _p_ = new tars::JsonValueObj();
                    _p_->value["clientContext"] = tars::JsonOutput::writeJson(clientContext);
                    _p_->value["req"] = tars::JsonOutput::writeJson(req);
                    _trace_param_ = tars::TC_Json::writeValue(_p_);
                }
                else if(ServantProxyThreadData::TraceContext::ENP_OVERMAXLEN == _trace_param_flag_)
                {
                    _trace_param_ = "{\"trace_param_over_max_len\":true}";
                }
                TARS_TRACE(_pSptd_->getTraceKey(ServantProxyThreadData::TraceContext::EST_CS), TRACE_ANNOTATION_CS, ServerConfig::Application + "." + ServerConfig::ServerName, tars_name(), "request", 0, _trace_param_, "");
            }

            std::map<string, string> _mStatus;
            shared_ptr<tars::ResponsePacket> rep = tars_invoke(tars::TARSNORMAL,"request", _os, context, _mStatus);
            if(pResponseContext)
            {
                pResponseContext->swap(rep->context);
            }

            tars::TarsInputStream<tars::BufferReader> _is;
            _is.setBuffer(rep->sBuffer);
            tars::Int32 _ret;
            _is.read(_ret, 0, true);
            _is.read(resp, 3, true);
            if (_pSptd_ && _pSptd_->_traceCall)
            {
                string _trace_param_;
                int _trace_param_flag_ = _pSptd_->needTraceParam(ServantProxyThreadData::TraceContext::EST_CR, _is.size());
                if (ServantProxyThreadData::TraceContext::ENP_NORMAL == _trace_param_flag_)
                {
                    tars::JsonValueObjPtr _p_ = new tars::JsonValueObj();
                    _p_->value[""] = tars::JsonOutput::writeJson(_ret);
                    _p_->value["resp"] = tars::JsonOutput::writeJson(resp);
                    _trace_param_ = tars::TC_Json::writeValue(_p_);
                }
                else if(ServantProxyThreadData::TraceContext::ENP_OVERMAXLEN == _trace_param_flag_)
                {
                    _trace_param_ = "{\"trace_param_over_max_len\":true}";
                }
                TARS_TRACE(_pSptd_->getTraceKey(ServantProxyThreadData::TraceContext::EST_CR), TRACE_ANNOTATION_CR, ServerConfig::Application + "." + ServerConfig::ServerName, tars_name(), "request", 0, _trace_param_, "");
            }

            return _ret;
        }

        void async_request(BaseServantPrxCallbackPtr callback,const otim::ClientContext &clientContext,const otim::OTIMPack &req,const map<string, string>& context = TARS_CONTEXT())
        {
            tars::TarsOutputStream<tars::BufferWriterVector> _os;
            _os.write(clientContext, 1);
            _os.write(req, 2);
            std::map<string, string> _mStatus;
            ServantProxyThreadData *_pSptd_ = ServantProxyThreadData::getData();
            if (_pSptd_ && _pSptd_->_traceCall)
            {
                _pSptd_->newSpan();
                string _trace_param_;
                int _trace_param_flag_ = _pSptd_->needTraceParam(ServantProxyThreadData::TraceContext::EST_CS, _os.getLength());
                if (ServantProxyThreadData::TraceContext::ENP_NORMAL == _trace_param_flag_)
                {
                    tars::JsonValueObjPtr _p_ = new tars::JsonValueObj();
                    _p_->value["clientContext"] = tars::JsonOutput::writeJson(clientContext);
                    _p_->value["req"] = tars::JsonOutput::writeJson(req);
                    _trace_param_ = tars::TC_Json::writeValue(_p_);
                }
                else if(ServantProxyThreadData::TraceContext::ENP_OVERMAXLEN == _trace_param_flag_)
                {
                    _trace_param_ = "{\"trace_param_over_max_len\":true}";
                }
                TARS_TRACE(_pSptd_->getTraceKey(ServantProxyThreadData::TraceContext::EST_CS), TRACE_ANNOTATION_CS, ServerConfig::Application + "." + ServerConfig::ServerName, tars_name(), "request", 0, _trace_param_, "");
            }
            tars_invoke_async(tars::TARSNORMAL,"request", _os, context, _mStatus, callback);
        }
        
        tars::Future< BaseServantPrxCallbackPromise::PromiserequestPtr > promise_async_request(const otim::ClientContext &clientContext,const otim::OTIMPack &req,const map<string, string>& context)
        {
            tars::Promise< BaseServantPrxCallbackPromise::PromiserequestPtr > promise;
            BaseServantPrxCallbackPromisePtr callback = new BaseServantPrxCallbackPromise(promise);

            tars::TarsOutputStream<tars::BufferWriterVector> _os;
            _os.write(clientContext, 1);
            _os.write(req, 2);
            std::map<string, string> _mStatus;
            tars_invoke_async(tars::TARSNORMAL,"request", _os, context, _mStatus, callback);

            return promise.getFuture();
        }

        void coro_request(BaseServantCoroPrxCallbackPtr callback,const otim::ClientContext &clientContext,const otim::OTIMPack &req,const map<string, string>& context = TARS_CONTEXT())
        {
            tars::TarsOutputStream<tars::BufferWriterVector> _os;
            _os.write(clientContext, 1);
            _os.write(req, 2);
            std::map<string, string> _mStatus;
            tars_invoke_async(tars::TARSNORMAL,"request", _os, context, _mStatus, callback, true);
        }

        BaseServantProxy* tars_hash(uint32_t key)
        {
            return (BaseServantProxy*)ServantProxy::tars_hash(key);
        }

        BaseServantProxy* tars_consistent_hash(uint32_t key)
        {
            return (BaseServantProxy*)ServantProxy::tars_consistent_hash(key);
        }

        BaseServantProxy* tars_open_trace(bool traceParam = false)
        {
            return (BaseServantProxy*)ServantProxy::tars_open_trace(traceParam);
        }

        BaseServantProxy* tars_set_timeout(int msecond)
        {
            return (BaseServantProxy*)ServantProxy::tars_set_timeout(msecond);
        }

        static const char* tars_prxname() { return "BaseServantProxy"; }
    };
    typedef tars::TC_AutoPtr<BaseServantProxy> BaseServantPrx;

    /* servant for server */
    class BaseServant : public tars::Servant
    {
    public:
        virtual ~BaseServant(){}
        virtual tars::Int32 request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr _current_) = 0;
        virtual tars::Int32 request(otim::ClientContext && clientContext,otim::OTIMPack && req,otim::OTIMPack &resp,tars::TarsCurrentPtr _current_) 
        { return request(clientContext, req, resp, _current_); }
        static void async_response_request(tars::TarsCurrentPtr _current_, tars::Int32 _ret, const otim::OTIMPack &resp)
        {
            size_t _rsp_len_ = 0;
            if (_current_->getRequestVersion() == TUPVERSION )
            {
                UniAttribute<tars::BufferWriterVector, tars::BufferReader>  _tarsAttr_;
                _tarsAttr_.setVersion(_current_->getRequestVersion());
                _tarsAttr_.put("", _ret);
                _tarsAttr_.put("tars_ret", _ret);
                _tarsAttr_.put("resp", resp);

                vector<char> sTupResponseBuffer;
                _tarsAttr_.encode(sTupResponseBuffer);
                _current_->sendResponse(tars::TARSSERVERSUCCESS, sTupResponseBuffer);
                _rsp_len_ = sTupResponseBuffer.size();
            }
            else if (_current_->getRequestVersion() == JSONVERSION)
            {
                tars::JsonValueObjPtr _p = new tars::JsonValueObj();
                _p->value["resp"] = tars::JsonOutput::writeJson(resp);
                _p->value["tars_ret"] = tars::JsonOutput::writeJson(_ret);
                vector<char> sJsonResponseBuffer;
                tars::TC_Json::writeValue(_p, sJsonResponseBuffer);
                _current_->sendResponse(tars::TARSSERVERSUCCESS, sJsonResponseBuffer);
                _rsp_len_ = sJsonResponseBuffer.size();
            }
            else
            {
                tars::TarsOutputStream<tars::BufferWriterVector> _os;
                _os.write(_ret, 0);

                _os.write(resp, 3);

                _rsp_len_ = _os.getLength();
                _current_->sendResponse(tars::TARSSERVERSUCCESS, _os);
            }
            if (_current_->isTraced())
            {
                string _trace_param_;
                int _trace_param_flag_ = ServantProxyThreadData::needTraceParam(ServantProxyThreadData::TraceContext::EST_SS, _current_->getTraceKey(), _rsp_len_);
                if (ServantProxyThreadData::TraceContext::ENP_NORMAL == _trace_param_flag_)
                {
                    tars::JsonValueObjPtr _p_ = new tars::JsonValueObj();
                    _p_->value[""] = tars::JsonOutput::writeJson(_ret);
                    _p_->value["resp"] = tars::JsonOutput::writeJson(resp);
                    _trace_param_ = tars::TC_Json::writeValue(_p_);
                }
                else if(ServantProxyThreadData::TraceContext::ENP_OVERMAXLEN == _trace_param_flag_)
                {
                    _trace_param_ = "{\"trace_param_over_max_len\":true}";
                }
                TARS_TRACE(_current_->getTraceKey(), TRACE_ANNOTATION_SS, "", ServerConfig::Application + "." + ServerConfig::ServerName, "request", 0, _trace_param_, "");
            }

        }
        static void async_response_push_request(tars::CurrentPtr _current_, tars::Int32 _ret, const otim::OTIMPack &resp, const map<string, string> &_context = tars::Current::TARS_STATUS())
        {
            {
                tars::TarsOutputStream<tars::BufferWriterVector> _os;
                _os.write(_ret, 0);

                _os.write(resp, 3);

                _current_->sendPushResponse( tars::TARSSERVERSUCCESS ,"request", _os, _context);
            }
        }

    public:
        int onDispatch(tars::TarsCurrentPtr _current, vector<char> &_sResponseBuffer)
        {
            static ::std::string __otim__BaseServant_all[]=
            {
                "request"
            };

            pair<string*, string*> r = equal_range(__otim__BaseServant_all, __otim__BaseServant_all+1, _current->getFuncName());
            if(r.first == r.second) return tars::TARSSERVERNOFUNCERR;
            switch(r.first - __otim__BaseServant_all)
            {
                case 0:
                {
                    tars::TarsInputStream<tars::BufferReader> _is;
                    _is.setBuffer(_current->getRequestBuffer());
                    otim::ClientContext clientContext;
                    otim::OTIMPack req;
                    otim::OTIMPack resp;
                    if (_current->getRequestVersion() == TUPVERSION)
                    {
                        UniAttribute<tars::BufferWriterVector, tars::BufferReader>  _tarsAttr_;
                        _tarsAttr_.setVersion(_current->getRequestVersion());
                        _tarsAttr_.decode(_current->getRequestBuffer());
                        _tarsAttr_.get("clientContext", clientContext);
                        _tarsAttr_.get("req", req);
                        _tarsAttr_.getByDefault("resp", resp, resp);
                    }
                    else if (_current->getRequestVersion() == JSONVERSION)
                    {
                        tars::JsonValueObjPtr _jsonPtr = tars::JsonValueObjPtr::dynamicCast(tars::TC_Json::getValue(_current->getRequestBuffer()));
                        tars::JsonInput::readJson(clientContext, _jsonPtr->value["clientContext"], true);
                        tars::JsonInput::readJson(req, _jsonPtr->value["req"], true);
                        tars::JsonInput::readJson(resp, _jsonPtr->value["resp"], false);
                    }
                    else
                    {
                        _is.read(clientContext, 1, true);
                        _is.read(req, 2, true);
                        _is.read(resp, 3, false);
                    }
                    ServantProxyThreadData *_pSptd_ = ServantProxyThreadData::getData();
                    if (_pSptd_ && _pSptd_->_traceCall)
                    {
                        string _trace_param_;
                        int _trace_param_flag_ = _pSptd_->needTraceParam(ServantProxyThreadData::TraceContext::EST_SR, _is.size());
                        if (ServantProxyThreadData::TraceContext::ENP_NORMAL == _trace_param_flag_)
                        {
                            tars::JsonValueObjPtr _p_ = new tars::JsonValueObj();
                            _p_->value["clientContext"] = tars::JsonOutput::writeJson(clientContext);
                            _p_->value["req"] = tars::JsonOutput::writeJson(req);
                            _trace_param_ = tars::TC_Json::writeValue(_p_);
                        }
                        else if(ServantProxyThreadData::TraceContext::ENP_OVERMAXLEN == _trace_param_flag_)
                        {
                            _trace_param_ = "{\"trace_param_over_max_len\":true}";
                        }
                        TARS_TRACE(_pSptd_->getTraceKey(ServantProxyThreadData::TraceContext::EST_SR), TRACE_ANNOTATION_SR, "", ServerConfig::Application + "." + ServerConfig::ServerName, "request", 0, _trace_param_, "");
                    }

                    tars::Int32 _ret = request(std::move(clientContext),std::move(req),resp, _current);
                    if(_current->isResponse())
                    {
                        if (_current->getRequestVersion() == TUPVERSION)
                        {
                            UniAttribute<tars::BufferWriterVector, tars::BufferReader>  _tarsAttr_;
                            _tarsAttr_.setVersion(_current->getRequestVersion());
                            _tarsAttr_.put("", _ret);
                            _tarsAttr_.put("tars_ret", _ret);
                            _tarsAttr_.put("resp", resp);
                            _tarsAttr_.encode(_sResponseBuffer);
                        }
                        else if (_current->getRequestVersion() == JSONVERSION)
                        {
                            tars::JsonValueObjPtr _p = new tars::JsonValueObj();
                            _p->value["resp"] = tars::JsonOutput::writeJson(resp);
                            _p->value["tars_ret"] = tars::JsonOutput::writeJson(_ret);
                            tars::TC_Json::writeValue(_p, _sResponseBuffer);
                        }
                        else
                        {
                            tars::TarsOutputStream<tars::BufferWriterVector> _os;
                            _os.write(_ret, 0);
                            _os.write(resp, 3);
                            _os.swap(_sResponseBuffer);
                        }
                        if (_pSptd_ && _pSptd_->_traceCall)
                        {
                            string _trace_param_;
                            int _trace_param_flag_ = _pSptd_->needTraceParam(ServantProxyThreadData::TraceContext::EST_SS, _sResponseBuffer.size());
                            if (ServantProxyThreadData::TraceContext::ENP_NORMAL == _trace_param_flag_)
                            {
                                tars::JsonValueObjPtr _p_ = new tars::JsonValueObj();
                                _p_->value[""] = tars::JsonOutput::writeJson(_ret);
                                _p_->value["resp"] = tars::JsonOutput::writeJson(resp);
                                _trace_param_ = tars::TC_Json::writeValue(_p_);
                            }
                            else if(ServantProxyThreadData::TraceContext::ENP_OVERMAXLEN == _trace_param_flag_)
                            {
                                _trace_param_ = "{\"trace_param_over_max_len\":true}";
                            }
                            TARS_TRACE(_pSptd_->getTraceKey(ServantProxyThreadData::TraceContext::EST_SS), TRACE_ANNOTATION_SS, "", ServerConfig::Application + "." + ServerConfig::ServerName, "request", 0, _trace_param_, "");
                        }

                    }
                    else if(_pSptd_ && _pSptd_->_traceCall)
                    {
                        _current->setTrace(_pSptd_->_traceCall, _pSptd_->getTraceKey(ServantProxyThreadData::TraceContext::EST_SS));
                    }

                    return tars::TARSSERVERSUCCESS;

                }
            }
            return tars::TARSSERVERNOFUNCERR;
        }
    };


}



#endif
