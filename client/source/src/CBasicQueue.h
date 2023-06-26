//
//  CBasicQueue.h
//
//  Created by 兰怀玉 on 16/4/18.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef CBasicQueue_h
#define CBasicQueue_h


const int BUFFER_MAX_LEN = 4096;


template <typename T>
class CBasicQueue {
private:
    T* m_array;
    size_t m_size = 0;//实际大小
    size_t m_capacity;//可容纳大小
    
    size_t newCapacity(size_t currentCapacity) {
        //        size_t increment = (currentCapacity < (BUFFER_MAX_LEN / 2) ?
        //                         BUFFER_MAX_LEN : currentCapacity >> 1);
        return currentCapacity + BUFFER_MAX_LEN;
    }
public:
    CBasicQueue() {
        m_capacity = BUFFER_MAX_LEN;
        m_size = 0;
        m_array = new T[m_capacity];
    }
    
    ~CBasicQueue(){
        delete []m_array;
    }
    
    //     CBasicQueue(int capacity) {
    //        if (capacity <= 0) {
    //            capacity = BUFFER_MAX_LEN;
    //        }
    //
    //        m_capacity = capacity;
    //        m_array = new T[m_capacity];
    //    }
    
    const T* getArray() {
        return m_array;
    }
    
    
    
    size_t size() {
        return m_size;
    }
    
    
    void push(T* bytes, size_t len) {
        if (bytes == NULL || len <= 0) {
            return;
        }
        
        size_t newSize = m_size + len*sizeof(T); // If add overflows, arraycopy will fail
        if (newSize > m_capacity) {
            m_capacity = newCapacity(newSize);  // ~33% growth room
            T* newArray = new T[m_capacity];
            
            memcpy(newArray, m_array, sizeof(T)*m_size);
            delete [] m_array;
            
            m_array = newArray;
        }
        
//        cout<<"CBasicQueue m_capacity:"<<m_capacity<<" m_size:"<<m_size<<" len:"<<len<<" newSize:"<<newSize<<endl;
        
        memcpy(&m_array[m_size], bytes, sizeof(T)*len);
        m_size = newSize;
        
    }
    
    //
    T* pop(int count) {
        if (count <= 0 || count > m_size) {
            return NULL;
        }
        
        T* bytes = new T[count];
        memcpy(bytes, m_array, sizeof(T)*count);
        
        m_size -= count;
        memcpy(m_array, &m_array[count], sizeof(T)*m_size);
        
        return bytes;
    }
    
    void  clear(){
        m_size = 0;
    }
    
};


#endif /* CBasicQueue_h */
