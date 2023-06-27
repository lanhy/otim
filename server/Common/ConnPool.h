#pragma once

#include <vector>
#include <mutex>
#include <atomic>
#include <map>
#include <thread>
#include <condition_variable>
#include "log.h"

template <typename T>
class ConnPool {
public:
    struct Node {
        T t;
        int count;
    };
public:
    /**
     * 返回一个默认池对象
     */
    static ConnPool *instance() {
        static ConnPool pool;
        return &pool;
    }

    static ConnPool *instance(int id) {
        ConnPool * p = nullptr;
        if (id >= 0 && id < (int)sizeof(m_id_conns)) {
            p = m_id_conns[id];
        } else {
            MLOG_ERROR("SERVICEALARM id out of range, " << id);
        }

        if (!p) {
            MLOG_ERROR("SERVICEALARM pool not initialized?");
        }

        return p;
    }

    static ConnPool *create(int id = -1) {
        ConnPool *p = new ConnPool();
        if (!p)
            return nullptr;
        p->m_id = id;
        if (id >= 0) {
            m_id_conns[id] = p;
        }
        return p;
    }

    static ConnPool *create(int count, std::function<T()> init, std::function<void(T)> fini = nullptr, int id = -1) {
        ConnPool *p = new ConnPool();
        if (!p)
            return nullptr;
        p->m_id = id;
        if (id >= 0) {
            m_id_conns[id] = p;
        }
        if (!p->open(count, init, fini)) {
            delete p;
            return nullptr;
        }
        return p;
    }

    static ConnPool *create(int count_min, int count_max, std::function<T()> init, std::function<void(T)> fini = nullptr, int id = -1) {
        ConnPool *p = new ConnPool();
        if (!p)
            return nullptr;
        p->m_id = id;
        if (id >= 0) {
            m_id_conns[id] = p;
        }
        if (!p->open(count_min, count_max, init, fini)) {
            delete p;
            return nullptr;
        }
    	MLOG_DEBUG("create min:"<<count_min<<" max:"<<count_max); 
	   return p;
    }

    ConnPool(){}
    /**
     * 释放池对象
     */
    ~ConnPool() {
        close();
    }

    /**
     * 建立连接池
     */
    bool open(const std::vector<T> &v) {
        std::unique_lock<std::mutex> unique_lock(m_mutex);
        if (!m_conns.empty())
            return true;
        m_init = nullptr;
        m_fini = nullptr;
        m_conns = v;
        m_capacity = (int)m_conns.size();
        return true;
    }

    bool open(int count, std::function<T()> init, std::function<void(T)> fini = nullptr) {
        return open(5, count, init, fini);
    }

    /**
     * 初始化连接池
     * @param count_min，初始化时，预先建立连接的连接数
     * @param count_max，使用中，最大建立的连接数。连接建立后不会主动释放。
     */ 
    bool open(int count_min, int count_max, std::function<T()> init, std::function<void(T)> fini = nullptr) {
        {
            std::unique_lock<std::mutex> unique_lock(m_mutex);
            if (!m_conns.empty())
                return true;
        }

        if (count_min <= 0)
            count_min = 1;
        if (count_max < count_min)
            count_max = count_min;
		MLOG_DEBUG("min:"<<count_min<<" max:"<<count_max);
        m_init = init;
        m_fini = fini;
        m_count_min = count_min;
        m_count_max = count_max;

        for (int i = 0; i < m_count_min; i++) {
            T t = init();
            if (t) {
                std::unique_lock<std::mutex> unique_lock(m_mutex);
                m_conns.push_back(t);
            }
            else {
                close();
                return false;
            }
        }
        m_capacity = (int)m_conns.size();
		MLOG_DEBUG("capacity:"<<m_capacity);
        return true;
    }

    /**
     * 关闭连接池
     */
    void close() {
        std::vector<T> conns;
        if (m_fini) {
            {
                std::unique_lock<std::mutex> unique_lock(m_mutex);
                conns = m_conns;
                m_conns.clear();
                memset(m_id_conns, 0, sizeof(m_id_conns));
            }

            for (auto &conn : conns) {
                m_fini(conn);
            }
        }
        m_capacity = 0;
    }

    /**
     * 查看当前的连接总数
     */ 
    int capacity() const {
        return m_capacity;
    }

    /**
     * 返回实例id
     */ 
    int id() const {
        return m_id;
    }

    /**
     * 在没有空闲连接时，输出告警信息的时间间隔，ms
     */ 
    void wait_interval(int interval) {
        m_wait_interval = interval;
    }

    /**
     * 获取连接
     */
    T get() {
        std::unique_lock<std::mutex> unique_lock(m_mutex);
        T p = getCache();
        if (p)
            return p;
		MLOG_DEBUG("get instance:"<<m_conns.size());
        bool lastEmpty = false;
        if (m_conns.empty()) {
            lastEmpty = true;
            MLOG_DEBUG("m_conns is empty");
            if (m_capacity == 0) {
                MLOG_ERROR("SERVICEALARM m_conns not initizlized.");
            } else if (m_capacity < m_count_max) {
                if (m_init) {
                    T t = m_init();
                    if (t) {
                        m_conns.push_back(t);
                        m_capacity++;
                    }
                }
            }
        }

        while (m_conns.empty()) {
            if (m_condition_variable.wait_for(unique_lock, std::chrono::milliseconds(m_wait_interval)) == std::cv_status::timeout) {
                MLOG_WARN("SERVICEALARM m_conns wait timeout.");
            }
        }

        if (lastEmpty) {
            MLOG_DEBUG("m_conns waitted");
        }

        int index = (int)m_conns.size() - 1;
        T t = m_conns[index];
        m_conns.erase(m_conns.begin() + index);
        addCache(t);
        return t;
    }

    /**
     * 释放连接
     */
    void release(T t) {
        std::unique_lock<std::mutex> unique_lock(m_mutex);
        if (releaseCache(t) == 0) {
            m_conns.push_back(t);
            m_condition_variable.notify_one();
        	MLOG_DEBUG("rlease ok");
		}
    }

protected:
    T getCache() {
        auto it = m_thread_cache.find(std::this_thread::get_id());
        if (it == m_thread_cache.end())
            return nullptr;
        if (it->second.t)
            it->second.count++;
        return it->second.t;
    }

    int releaseCache(T t) {
        auto it = m_thread_cache.find(std::this_thread::get_id());
        if (it != m_thread_cache.end()) {
            if (--it->second.count == 0) {
                it->second.t = nullptr;
                return 0;
            }
            return it->second.count;
        }
        return 0;
    }

    void addCache(T t) {
        Node node;
        node.t = t;
        node.count = 1;
        m_thread_cache[std::this_thread::get_id()] = node;
    }

protected:
    int m_capacity = 0;
    int m_count_max = 30;
    int m_count_min = 3;
    int m_wait_interval = 5;    // ms
    int m_id = -1;
    std::vector<T> m_conns;
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    std::function<T()> m_init = nullptr;
    std::function<void(T)> m_fini = nullptr;
    std::map<std::thread::id, Node> m_thread_cache;
    static ConnPool * m_id_conns[1024];
};

template <typename T> ConnPool<T> * ConnPool<T>::m_id_conns[1024];

template <typename T>
class ConnPtr {
public:
    ConnPtr(ConnPool<T> *pPool) {
        m_pConn = pPool->get();
        m_pPool = pPool;
    }

    ~ConnPtr() {
        release();
    }

    T operator->() {
        return m_pConn;
    }

    T get() {
        return m_pConn;
    }

    void release() {
        if (m_pConn) {
            m_pPool->release(m_pConn);
            m_pConn = nullptr;
            m_pPool = nullptr;
        }
    }

    operator bool() {
        return m_pConn != nullptr;
    }

public:
    ConnPtr(const ConnPtr&) = delete;
    ConnPtr& operator=(const ConnPtr&) = delete;

protected:
    T m_pConn;
    ConnPool<T> *m_pPool;
};

