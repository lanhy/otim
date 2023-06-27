#ifndef __LOCKER_H__
#define __LOCKER_H__
#include <semaphore.h>
#include <pthread.h>
#include <assert.h>
#include "log.h"



const int nNO_END_LOCKER = -1;  //永久阻塞式的
const int nNO_BLOCK_LOCKER = 0; //非阻塞式的

enum EmLocker
{
	EM_RLOCK = 0,
	EM_WLOCK = 1
};

class CIFRWLocker
{
public:
	virtual bool DoLock(int nTimeOut = nNO_END_LOCKER, EmLocker emLocker = EM_RLOCK) = 0;
	virtual bool DoUnLock() = 0;
	virtual ~CIFRWLocker()
	{
	};
};




class CRWLockSem : public CIFRWLocker
{
public:
	CRWLockSem()
	{
		int nRet = pthread_rwlock_init(&m_RWLock, NULL);
        if (0 == nRet)
        {
            m_bInitial = true;
        }
        else
        {
            m_bInitial = false;
            MLOG_ERROR("pthread_rwlock_init failed, errorcode:" << nRet);
        }
	}

	~CRWLockSem()
	{
		if (!m_bInitial)
		{
            return;
		}

        int nRet = pthread_rwlock_destroy(&m_RWLock);
        if (0 != nRet)
        {
            MLOG_ERROR("pthread_rwlock_destroy failed, errorcode:" << nRet);
        }
	}

	virtual bool DoLock(int nTimeOut = nNO_END_LOCKER, EmLocker emLocker = EM_RLOCK)
	{
        if (!m_bInitial)
        {
            MLOG_ERROR("DoLock failed, reason: rwlock init failed.");
            return false;
        }

        if (nNO_BLOCK_LOCKER == nTimeOut)
        {
            return TryLock(emLocker);
        }
        else
        {
            return Lock(emLocker);
        }
	}

	virtual bool DoUnLock()
	{
        if (!m_bInitial)
        {
            MLOG_ERROR("DoUnLock failed, reason: rwlock init failed.");
            return false;
        }
        return ReleaseLock();
	}

private:
    bool Lock(EmLocker emLocker = EM_RLOCK)
    {
        int nRet = 0;
        if (EM_RLOCK == emLocker)
        {
            nRet = pthread_rwlock_rdlock(&m_RWLock);
        }
        else
        {
            nRet = pthread_rwlock_wrlock(&m_RWLock);
        }

        if (nRet != 0)
        {
            MLOG_ERROR("Lock failed, pthread_rwlock_XXlock error, rwlock type:" << (int)emLocker << ", errorcode:" << nRet);
            return false;
        }
        return true;
    }

    bool TryLock(EmLocker emLocker = EM_RLOCK)
    {
        int nRet = 0;
        if (EM_RLOCK == emLocker)
        {
            nRet = pthread_rwlock_tryrdlock(&m_RWLock);
        }
        else
        {
            nRet = pthread_rwlock_trywrlock(&m_RWLock);
        }

        if (nRet != 0)
        {
            return false;
        }
        return true;
    }

    bool ReleaseLock()
    {
        int nRet = pthread_rwlock_unlock(&m_RWLock);
        if (nRet != 0)
        {
            MLOG_ERROR("ReleaseLock failed, pthread_rwlock_unlock error, errorcode:" << nRet);
            return false;
        }
        return true;
    }
	pthread_rwlock_t m_RWLock;
	bool m_bInitial;
};


class CNoCopy
{
protected:
	CNoCopy()
	{
	}
	~CNoCopy()
	{
	}
private:  // emphasize the following members are private
	CNoCopy(const CNoCopy &);
	const CNoCopy &operator=(const CNoCopy &);
};

class CScopeRWLocker : public CNoCopy
{
public:
    CScopeRWLocker(CIFRWLocker &cRWLocker, int nTimeOut = nNO_END_LOCKER, EmLocker emLocker = EM_RLOCK) : m_cRWLocker(cRWLocker), m_bIsLock(true)
	{
        m_bIsLock = m_cRWLocker.DoLock(nTimeOut, emLocker);
	}

    bool IsLockAvailable() const
    {
        return m_bIsLock;
    }

	~CScopeRWLocker()
	{
        if (m_bIsLock)
        {
            m_cRWLocker.DoUnLock();
        }		
	}
private:
	CIFRWLocker &m_cRWLocker;
    bool m_bIsLock;
};



// 互斥锁
class CIMutex
{
public:
	virtual bool DoLock(int nTimeOut = nNO_END_LOCKER) = 0;
	virtual bool DoUnLock(void) = 0;
	virtual ~CIMutex()
	{
	};
};

class CScopeMutexLocker : public CNoCopy
{
public:
    CScopeMutexLocker(CIMutex &cMutex, int nTimeOut = nNO_END_LOCKER) : m_cMutex(cMutex), m_bIsLock(true)
	{
        m_bIsLock = m_cMutex.DoLock(nTimeOut);
	}

    bool IsLockAvailable() const
    {
        return m_bIsLock;
    }

	~CScopeMutexLocker()
	{
        if (m_bIsLock)
        {
            m_cMutex.DoUnLock();
        }		
	}
private:
	CIMutex &m_cMutex;
    bool m_bIsLock;
};


class CMutexSem : public CIMutex
{
public:
	CMutexSem(void)
	{
		int nRet = pthread_mutex_init(&m_cMux, NULL);
        if (0 == nRet)
        {
            m_bInitial = true;
        }
        else
        {
            m_bInitial = false;
            MLOG_ERROR("pthread_mutex_init failed, errorcode:" << nRet);
        }
	}

	virtual ~CMutexSem()
	{
        if (!m_bInitial)
        {
            return;
        }

		int nRet = pthread_mutex_destroy(&m_cMux);
        if (0 != nRet)
        {
            MLOG_ERROR("pthread_mutex_destroy failed, errorcode:" << nRet);
        }
	}

	virtual bool DoLock(int nTimeOut = nNO_END_LOCKER)
	{
        if (!m_bInitial)
        {
            MLOG_ERROR("DoUnLock failed, reason: Mutex init failed.");
            return false;
        }

        if (nNO_BLOCK_LOCKER == nTimeOut)
		{
            return TryLock();
		}
        else
        {
            return Lock();
        }
	}

	virtual bool DoUnLock(void)
	{
        if (!m_bInitial)
        {
            MLOG_ERROR("DoUnLock failed, reason: Mutex init failed.");
            return false;
        }
        return ReleaseLock();
	}

private:
    bool Lock()
    {
        int nRet = pthread_mutex_lock(&m_cMux);
        if (0 != nRet)
        {
            MLOG_ERROR("Lock failed, pthread_mutex_lock error, errorcode:" << nRet);
            return false;
        }
        return true;
    }

    bool TryLock()
    {
        int nRet = pthread_mutex_trylock(&m_cMux);
        if (0 != nRet)
        {
            return false;
        }
        return true;
    }

    bool ReleaseLock()
    {
        int nRet = pthread_mutex_unlock(&m_cMux);
        if (0 != nRet)
        {
            MLOG_ERROR("ReleaseLock failed, pthread_mutex_unlock error, errorcode:" << nRet);
            return false;
        }
        return true;
    }

private:
	pthread_mutex_t m_cMux;
    bool m_bInitial;
};


template <typename T>
class CSafeSimpleResouce
{
public:
	CSafeSimpleResouce(void)
	{
		CScopeMutexLocker cLock(m_mutex);
	}

	explicit CSafeSimpleResouce(T xObj)
	{
		//有意留空
		CScopeMutexLocker cLock(m_mutex);
		m_xObj = xObj;
	}

	operator T() const /* parasoft-suppress  JSF-177 "此处用于定义多线程安全的基本类型，必须支持该接口" */
	{
		CScopeMutexLocker cLock(m_mutex);

		T xObj;
		xObj = m_xObj;
		return xObj;
	}


	T data() const
	{
		CScopeMutexLocker cLock(m_mutex);

		T xObj;
		xObj = m_xObj;
		return xObj;
	}


	void SetValue(T xObj)
	{
		CScopeMutexLocker cLock(m_mutex);
		m_xObj = xObj;
	}

	T operator=(T xObj) /* parasoft-suppress  JSF-94_b "此处用于支持多线程安全的基本类型的赋值操作，虽然会遮掩基类型的赋值操作，但基类型不支持赋值，为方便使用，此处有意违背规范" */
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;
		xLocalObj = xObj;
		m_xObj = xObj;
		return xLocalObj;
	}

	T operator~()
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;

		xLocalObj = m_xObj;
		xLocalObj = ~xLocalObj;

		return xLocalObj;
	}

	T operator!()
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;

		xLocalObj = m_xObj;
		xLocalObj = !xLocalObj;

		return xLocalObj;
	}

	T operator++(int)
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;

		xLocalObj = m_xObj;
		++m_xObj;

		return xLocalObj;
	}

	T operator++()
	{
		CScopeMutexLocker cLock(m_mutex);
		T xLocalObj;

		++m_xObj;
		xLocalObj = m_xObj;

		return xLocalObj;
	}

	T operator--(int)
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;

		xLocalObj = m_xObj;
		--m_xObj;

		return xLocalObj;
	}

	T operator--()
	{
		CScopeMutexLocker cLock(m_mutex);
		T xLocalObj;

		--m_xObj;
		xLocalObj = m_xObj;

		return xLocalObj;
	}

	T operator+(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;

		xLocalObj = m_xObj;
		xLocalObj += xObj;

		return xLocalObj;
	}

	T operator-(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);
		T xLocalObj;

		xLocalObj = m_xObj;
		xLocalObj -= xObj;

		return xLocalObj;
	}

	T operator *(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);
		T xLocalObj;

		xLocalObj = m_xObj;
		xLocalObj *= xObj;

		return xLocalObj;
	}

	T operator/(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);
		T xLocalObj;

		xLocalObj = m_xObj;
		xLocalObj /= xObj;

		return xLocalObj;
	}

	T operator &(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;

		xLocalObj = m_xObj;
		xLocalObj &= xObj;

		return xLocalObj;
	}

	T operator|(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);
		T xLocalObj;

		xLocalObj = m_xObj;
		xLocalObj |= xObj;

		return xLocalObj;
	}

	T operator^(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;
		xLocalObj = m_xObj;
		xLocalObj ^= xObj;
		return xLocalObj;
	}

	T operator+=(T xObj)
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;
		m_xObj += xObj;
		xLocalObj = m_xObj;
		return xLocalObj;
	}

	T operator-=(T xObj)
	{
		CScopeMutexLocker cLock(m_mutex);
		T xLocalObj;

		m_xObj -= xObj;
		xLocalObj = m_xObj;

		return xLocalObj;
	}

	T operator/=(T xObj)
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;

		m_xObj /= xObj;
		xLocalObj = m_xObj;

		return xLocalObj;
	}

	T operator*=(T xObj)
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;
		m_xObj *= xObj;
		xLocalObj = m_xObj;
		return xLocalObj;
	}

	T operator&=(T xObj)
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;
		m_xObj &= xObj;
		xLocalObj = m_xObj;
		return xLocalObj;
	}

	T operator|=(T xObj)
	{
		CScopeMutexLocker cLock(m_mutex);
		T xLocalObj;

		m_xObj |= xObj;
		xLocalObj = m_xObj;

		return xLocalObj;
	}

	T operator^=(T xObj)
	{
		CScopeMutexLocker cLock(m_mutex);

		T xLocalObj;

		m_xObj ^= xObj;
		xLocalObj = m_xObj;

		return xLocalObj;
	}

	bool operator>(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);

		bool bRet;

		bRet = m_xObj > xObj;

		return bRet;
	}
	bool operator<(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);

		bool bRet;
		bRet = m_xObj < xObj;
		return bRet;
	}

	bool operator>=(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);

		bool bRet;
		bRet = m_xObj >= xObj;
		return bRet;
	}

	bool operator<=(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);
		bool bRet;
		bRet = m_xObj <= xObj;
		return bRet;
	}

	bool operator==(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);

		bool bRet;
		bRet = m_xObj == xObj;
		return bRet;
	}

	bool operator!=(T xObj) const
	{
		CScopeMutexLocker cLock(m_mutex);

		bool bRet;
		bRet = m_xObj != xObj;
		return bRet;
	}
private:
	T m_xObj;//被封装对象，可以是基本类型
	mutable CMutexSem m_mutex;
};

typedef CSafeSimpleResouce<unsigned short> sshort;//多线程安全int符类型
typedef CSafeSimpleResouce<int> sint;//多线程安全int符类型

#endif
