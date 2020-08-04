#include <iostream>
#include <thread>
#include <list>
#include <vector>

using namespace std;

/**
 * 读写案例代码演示
 *
 */
namespace s1{
    class A{
    public:
        // 把收到的消息(玩家命令)放入到一个队列
        void inMsgRecvQueue(){
            for(int i = 0; i< 100000; ++i){
                cout << "inMsgRecvQueue()执行、插入一个元素" << i << endl;
                msgRecvQueue.push_back(i);
            }
        }
        // 把数据从消息队列中取出的线程
        void outMsgRecvQueue(){
            for(int i = 0; i< 100000; ++i){
                if(!msgRecvQueue.empty()){
                    // 消息队列不为空
                    int command = msgRecvQueue.front();// 返回第一个元素，不检查元素是否存在
                    msgRecvQueue.pop_front();
                }
                else cout << "outMsgRecvQueue()执行、消息队列为空" << i << endl;
            }
            cout << "end " << endl;
        }
    private:
        std::list<int> msgRecvQueue;
    };
}

#include <iostream>
#include <thread>
#include <list>
#include <mutex>

using namespace std;


#define DIELOCK

/**
 * 互斥量概念、用法、死锁演示及解决详解
 *
 */
namespace s2{
    class A{
    public:
        // 把收到的消息(玩家命令)放入到一个队列
        void inMsgRecvQueue(){
            for(int i = 0; i< 100000; ++i){
                cout << "inMsgRecvQueue()执行、插入一个元素" << i << endl;
                /**
                 * 加锁解锁要保护的数据
                 */
#ifdef DIELOCK
                std::lock(my_mutex1,my_mutex2); // 解决死锁问题
                std::lock_guard<std::mutex> sbGuard1(my_mutex1,std::adopt_lock);
                std::lock_guard<std::mutex> sbGuard2(my_mutex2,std::adopt_lock);
//                my_mutex1.lock();
//                my_mutex2.lock();

                msgRecvQueue.push_back(i);
//                my_mutex2.unlock();
//                my_mutex1.unlock();

#endif

#ifndef DIELOCK
                //                my_mutex.lock();
                {
                    // scope
                    lock_guard<std::mutex> sbGuard(my_mutex);
                    msgRecvQueue.push_back(i);
                }
                my_mutex.unlock();
#endif
            }
        }

        bool outMsgLULProc(int& command){
            
#ifdef DIELOCK
//            my_mutex2.lock();
//            my_mutex1.lock();
            std::lock(my_mutex1,my_mutex2); // 解决死锁问题
            // 参数std::adopt_lock 表示不用再lock了
            std::lock_guard<std::mutex> sbGuard1(my_mutex1,std::adopt_lock);
            std::lock_guard<std::mutex> sbGuard2(my_mutex2,std::adopt_lock);

            if(!msgRecvQueue.empty()){
                // 消息队列不为空
                int command = msgRecvQueue.front();// 返回第一个元素，不检查元素是否存在
                msgRecvQueue.pop_front();
//                my_mutex1.unlock();
//                my_mutex2.unlock();
                return true;
            }
//            my_mutex1.unlock();
//            my_mutex2.unlock();
            return false;
#endif

#ifndef DIELOCK
//            my_mutex.lock();
//            std::lock_guard<std::mutex> sbGuard(my_mutex); //用了lock_guard 就不能用unlock和lock了
            if(!msgRecvQueue.empty()){
                // 消息队列不为空
                int command = msgRecvQueue.front();// 返回第一个元素，不检查元素是否存在
                msgRecvQueue.pop_front();
//                my_mutex.unlock();  // 这个分支也要有unlock(),否则有可能会一直锁住
                return true;
            }
//            my_mutex.unlock();
            return false;
#endif
        }
        // 把数据从消息队列中取出的线程
        void outMsgRecvQueue(){
            int command = 0;
            for(int i = 0; i< 100000; ++i){
                bool result = outMsgLULProc(command);
                if(result){
                    cout << "outMsgRecvQueue()执行、取出一个元素" << command << endl;
                }
                else cout << "outMsgRecvQueue()执行、消息队列为空" << i << endl;
            }
            cout << "end " << endl;
        }
    private:
        std::list<int> msgRecvQueue;
        std::mutex my_mutex;
        std::mutex my_mutex1;
        std::mutex my_mutex2;
    };
}

/**
 * unique_lock 详解
 */
namespace s3{
    class A{
    public:
        std::unique_lock<std::mutex> rtn_unique_lock(){
            std::unique_lock<std::mutex> tmpGuard(my_mutex);
            return tmpGuard; // 返回临时对象，调用unique_lock 移动构造函数
        }
        // 把收到的消息(玩家命令)放入到一个队列
        void inMsgRecvQueue(){
            for(int i = 0; i< 100000; ++i){
                cout << "inMsgRecvQueue()执行、插入一个元素" << i << endl;
                /**
                 * 加锁解锁要保护的数据
                 */
//                lock_guard<std::mutex> sbGuard(my_mutex);
                // unique_lock 可以完全取代 lock_guard
//                my_mutex.lock();
//                unique_lock<std::mutex> sbGuard1(my_mutex);
                // 移动语义 这个时候就直接把所有权转移到了sbGuard2了
//                unique_lock<std::mutex> sbGuard2(std::move(sbGuard1));
                // 也可以通过返回
                unique_lock<std::mutex> sbGuard2 = rtn_unique_lock();
                // 解除关系
//                std::mutex *ptx = sbGuard1.release();
                std::mutex *ptx = sbGuard2.release();
                msgRecvQueue.push_back(i);
                // 这里必须手动解除
                ptx->unlock();
                // 自己有责任解锁
//                unique_lock<std::mutex> sbGuard1(my_mutex,std::adopt_lock);
//                unique_lock<std::mutex> sbGuard1(my_mutex,std::try_to_lock); //使用这个第二参数 不能再之前lock 否则会卡住
                // 如果拿到锁的话
//                if(sbGuard1.owns_lock()){
//                    cout << "inMsgRecvQueue 执行，拿到锁头..." << endl;
//                    msgRecvQueue.push_back(i);
//                }
//                else{
//                    cout << "inMsgRecvQueue 执行，但是没有拿到锁头..." << endl;
//                }
//                unique_lock<std::mutex> sbGuard1(my_mutex,std::defer_lock); // 初始化一个额没有加锁的mymutex
//                sbGuard1.lock(); // 自己加锁
//                // 有一些非共享代码要处理 可以先unlock()
//                sbGuard1.unlock();
//                sbGuard1.lock(); // 自己加锁
//                if(sbGuard1.try_lock()){
//                    cout << "inMsgRecvQueue 执行，拿到锁头..." << endl;
//                    msgRecvQueue.push_back(i);
//                }
//                else{
//                    cout << "inMsgRecvQueue 执行，但是没有拿到锁头..." << endl;
//                }
//                msgRecvQueue.push_back(i);
//                sbGuard1.unlock();
            }
        }

        bool outMsgLULProc(int& command){
//            std::lock_guard<std::mutex> sbGuard(my_mutex); //用了lock_guard 就不能用unlock和lock了
            unique_lock<std::mutex> sbGuard1(my_mutex);

//            std::chrono::milliseconds dura(2000);      // 20s
//            std::this_thread::sleep_for(dura);              // 线程休息20s 这回导致另外一个线程也会等待
            if(!msgRecvQueue.empty()){
                // 消息队列不为空
                int command = msgRecvQueue.front();// 返回第一个元素，不检查元素是否存在
                msgRecvQueue.pop_front();
                return true;
            }
            return false;
        }

        // 把数据从消息队列中取出的线程
        void outMsgRecvQueue(){
            int command = 0;
            for(int i = 0; i< 100000; ++i){
                bool result = outMsgLULProc(command);
                if(result){
                    cout << "outMsgRecvQueue()执行、取出一个元素" << command << endl;
                }
                else cout << "outMsgRecvQueue()执行、消息队列为空" << i << endl;
            }
            cout << "end " << endl;
        }
    private:
        std::list<int> msgRecvQueue;
        std::mutex my_mutex;
        std::mutex my_mutex1;
        std::mutex my_mutex2;
    };
}

/**
 * 单例设计模式分析
 * 线程安全
 * call_once()
 */
namespace s4{
    // 单线程情况下
    namespace edition1 {
        class MyCAS {
        private:
            MyCAS() {} //私有化构造函数
        private:
            static MyCAS *m_instance;
        public:
            static MyCAS *GetInstance() {
                // 这里多线程下不安全
                if (m_instance == nullptr) {
                    // 两个线程可能同时执行到这里，这样不好
                    m_instance = new MyCAS();
                    // 创建一个静态类对象
                    static GarbageCollection c;
                }
                return m_instance;
            }

            void func() {
                cout << "test" << endl;
            }

            // 引入一个类，对new的对象进行回收
            class GarbageCollection {
            public:
                ~GarbageCollection() {
                    if (MyCAS::m_instance) {
                        // 连个线程可能同时
                        delete MyCAS::m_instance;
                        MyCAS::m_instance == nullptr;
                    }
                }
            };
        };
    }
    // 加锁版本
    namespace edition2{
        class MyCAS{
        private:
            MyCAS(){} //私有化构造函数
        private:
            static MyCAS* m_instance;
            static std::mutex resource_mutex;
        public:
            static MyCAS* GetInstance(){
                // 双重加锁，提高效率
                if(m_instance == nullptr){
                    // 加锁
                    std::unique_lock<std::mutex> sbGuard(MyCAS::resource_mutex);
                    if(m_instance == nullptr){
                        m_instance = new MyCAS();
                        // 创建一个静态类对象
                        static GarbageCollection c;
                    }
                }
                return m_instance;
            }
            void func(){
                cout << "test" << endl;
            }
            // 引入一个类，对new的对象进行回收
            class GarbageCollection{
            public:
                ~GarbageCollection(){
                    if(MyCAS::m_instance){
                        // 连个线程可能同时
                        delete MyCAS::m_instance;
                        MyCAS::m_instance == nullptr;
                    }
                }
            };
        };
        MyCAS* MyCAS::m_instance = nullptr;
        std::mutex MyCAS::resource_mutex;
    }

    namespace callOnce{
        class MyCAS{
        private:
            MyCAS(){} //私有化构造函数
        private:
            static MyCAS* m_instance;
            static std::mutex resource_mutex;
            static std::once_flag g_flag;
            static void CreateInstance(){
                cout << "createInstance()被执行了" << endl;
                m_instance = new MyCAS();
                static GarbageCollection c;
            }
        public:
            static MyCAS* GetInstance(){
                // 如果两个线程同时执行到这里，其中一个线程执行CreateInstance完毕后，才会执行另一个线程
                std::call_once(g_flag,CreateInstance);
                return m_instance;
            }
            void func(){
                cout << "test" << endl;
            }
            // 引入一个类，对new的对象进行回收
            class GarbageCollection{
            public:
                ~GarbageCollection(){
                    if(MyCAS::m_instance){
                        // 连个线程可能同时
                        delete MyCAS::m_instance;
                        MyCAS::m_instance == nullptr;
                    }
                }
            };
        };
        MyCAS* MyCAS::m_instance = nullptr;
        std::once_flag MyCAS::g_flag; // 系统定义的标记
    }

    // 线程入口函数
    void myThread(){
        cout << "我的线程开始执行了" << endl;
        edition2::MyCAS::GetInstance(); // 这里可能有问题
        cout << "我的线程执行完毕" << endl;
        return;
    }
    void myThread2(){
        cout << "我的线程开始执行了" << endl;
        callOnce::MyCAS::GetInstance(); // 这里可能有问题
        cout << "我的线程执行完毕" << endl;
        return;
    }
}

/**
 *  condition_variable、wait、notify_one、notify_all
 *
 *  提高效率
 *
 */
 // 要包含这个头文件
#include <condition_variable>

namespace s5{
    class A{
    public:
        // 把收到的消息(玩家命令)放入到一个队列
        void inMsgRecvQueue(){
            for(int i = 0; i< 100000; ++i){
                unique_lock<std::mutex> sbGuard(my_mutex);
                cout << "inMsgRecvQueue()执行、插入一个元素" << i << endl;
                msgRecvQueue.push_back(i);
                // 通知其他某一个线程，将wait()唤醒，但是outMsgRecvQueue正处理一个事务，需要处理一段时间，而不是正卡在wait()
                // 那么此时 notify_one() 可能没有效果，还有可能另外一个线程一直都没有获得锁，这是有可能出现的问题
//                my_cond.notify_one();
                my_cond.notify_all(); // 唤醒其他所有线程
            }
        }

        // 得到通知的时候 我们才去取数据
//        bool outMsgLULProc(int& command){
//
//            if(!msgRecvQueue.empty()){
//                std::unique_lock<std::mutex> sbGuard(my_mutex);
//                if(!msgRecvQueue.empty()){
//                    // 消息队列不为空
//                    int command = msgRecvQueue.front();// 返回第一个元素，不检查元素是否存在
//                    msgRecvQueue.pop_front();
//                    return true;
//                }
//                return false;
//            }
//        }
        // 把数据从消息队列中取出的线程
        void outMsgRecvQueue(){
            int command = 0;
            while(true){
                std::unique_lock<std::mutex> sbGuard(my_mutex);
                // wait 等一个东西，
                // 如果第二个参数的返回值为false，那么wait将解锁互斥量，并堵塞到本行，
                        // 堵塞到某个线程调用notify_one()成员函数为止
                // 如果第二个参数返回值为true，wait()直接返回
                // 如果没有第二个参数：那就和第二参数为false类似堵塞到某个线程调用notify_one()成员函数为止

                // 当其他线程用notify_one() 将本wait() 唤醒后，wait就开始恢复干活：
                // a):wait() 会尝试再次获取互斥锁，如果获取不到 流程就会卡在这等待获取，如果获取到 ，就继续执行b
                // b):1、如果wait有第二个参数 就判断这个表达式，如果表达式返回false，那wait又对互斥量解锁，再次休眠
                //    2、如果第二个参数返回值为true，那么就会wait就返回，就走下来了，互斥锁还是被锁住了
                //    3、如果没有第二参数，则 wait返回，流程走下来
                my_cond.wait(sbGuard,[this]{
                    if(!msgRecvQueue.empty()) return true;
                    return false;
                });
                // 流程走到这里来 互斥锁一定是锁住的,至少有一个元素
                command = msgRecvQueue.front();
                msgRecvQueue.pop_front();
                cout << "outMsgRecvQueue()执行、取出一个元素" << command << "线程id：" << std::this_thread::get_id() << endl;
                sbGuard.unlock();

                //
                //处理别的事情...
            }
//            for(int i = 0; i< 100000; ++i){
//                bool result = outMsgLULProc(command);
//                if(result){
//                    cout << "outMsgRecvQueue()执行、取出一个元素" << command << endl;
//                }
//                else cout << "outMsgRecvQueue()执行、消息队列为空" << i << endl;
//            }
//            cout << "end " << endl;
        }
    private:
        std::list<int> msgRecvQueue;
        std::mutex my_mutex;
        std::condition_variable my_cond;  // 条件变量对象
    };
}

/**
 * async、future、packaged_task、promise
 */

#include <future>

namespace s6{
    class A{
    public:
        int myThread(int myVar){
            cout << myVar << endl;
            cout << "myThread start " << "thread id: " << std::this_thread::get_id() << endl;
            std::chrono::milliseconds dura(5000); // 休息 5 s
            std::this_thread::sleep_for(dura);
            cout << "myThread end " << "thread id: " << std::this_thread::get_id() << endl;
            return 4;
        }
    };
    int myThread(){
        cout << "myThread start " << "thread id: " << std::this_thread::get_id() << endl;
        std::chrono::milliseconds dura(5000); // 休息 5 s
        std::this_thread::sleep_for(dura);
        cout << "myThread end " << "thread id: " << std::this_thread::get_id() << endl;
        return 5;
    }

    int myThread2(int myVar){
        cout << myVar << endl;
        cout << "myThread start " << "thread id: " << std::this_thread::get_id() << endl;
        std::chrono::milliseconds dura(5000); // 休息 5 s
        std::this_thread::sleep_for(dura);
        cout << "myThread end " << "thread id: " << std::this_thread::get_id() << endl;
        return 3;
    }

    void myThread3(std::promise<int>& temp,int calc){
        //做一系列复杂的参数
        calc++;
        calc*=10;
        std::chrono::milliseconds dura(5000); // 休息 5 s
        std::this_thread::sleep_for(dura);

        //计算出结果
        int result = calc;
        // 保存结果
        temp.set_value(result); // 结果保存在temp中
        return;
    }

    void myThread4(std::future<int>& tmpf){
        auto result = tmpf.get();
        cout << "mythread4 result " << result << endl;
    }
}

#include <atomic>

/**
 * future其他成员函数、shared_future、atomic
 */

namespace s7{
    int g_mycout = 0;
    std::mutex my_mutex;
    std::atomic<int> g_atomic(0);  // 封装了一个类型为int的原子对象
    std::atomic<bool> g_ifend = { false}; // 线程退出标记
    void myThreadAtomicBool(){
        std::chrono::seconds dura(1);
        while(!g_ifend){
            // 线程没退出
            cout << "thread bool id "  << this_thread::get_id() << "start"<< endl;
            std::this_thread::sleep_for(dura);
        }
        cout << "thread bool id "  << this_thread::get_id() << "end" << endl;
    }

    void myThreadAtomic(){
        for(int i= 0;i < 1000000; ++i){
//            my_mutex.lock();
            g_mycout++;         // 普通变量
            g_atomic++;         // 原子变量、效率比加锁减锁效率高
//            my_mutex.unlock();
        }
        return;
    }
    int myThread(){
        cout << "myThread start " << "thread id: " << std::this_thread::get_id() << endl;
        std::chrono::milliseconds dura(5000); // 休息 5 s
        std::this_thread::sleep_for(dura);
        cout << "myThread end " << "thread id: " << std::this_thread::get_id() << endl;
        return 0;
    }
    int myThread2(int temp){
        cout << temp;
        cout << "myThread2 start " << "thread id: " << std::this_thread::get_id() << endl;
        std::chrono::milliseconds dura(5000); // 休息 5 s
        std::this_thread::sleep_for(dura);
        cout << "myThread2 end " << "thread id: " << std::this_thread::get_id() << endl;
        return 0;
    }
    void myThread3(std::future<int>& tmpf){
        auto result = tmpf.get(); //第二次会异常，因为get() 是一个移动语义
        cout << "mythread3 result " << result << endl;
    }
    void myThread4(std::shared_future<int>& tmpf){
        auto result = tmpf.get(); //第二次会异常，因为get() 是一个移动语义
        cout << "mythread3 result " << result << endl;
    }
}

/**
 * 十一、atomic 续谈、async 深入谈
 */
namespace s8{
    int g_mycout = 0;
    std::mutex my_mutex;
    std::atomic<int> g_atomic(0);  // 封装了一个类型为int的原子对象
    std::atomic<bool> g_ifend = { false}; // 线程退出标记
    void myThreadAtomicBool(){
        std::chrono::seconds dura(1);
        while(!g_ifend){
            // 线程没退出
            cout << "thread bool id "  << this_thread::get_id() << "start"<< endl;
            std::this_thread::sleep_for(dura);
        }
        cout << "thread bool id "  << this_thread::get_id() << "end" << endl;
    }

    void myThreadAtomic(){
        for(int i= 0;i < 1000000; ++i){
//            my_mutex.lock();
            g_mycout++;         // 普通变量
            g_atomic++;         // 原子变量、效率比加锁减锁效率高
            g_atomic+=1;        // 原子变量、效率比加锁减锁效率高
            g_atomic= g_atomic + 1;     // 不是原子操作
//            my_mutex.unlock();
        }
        return;
    }

    int myThread(){
        cout << "myThread() start " << "thread id :" << this_thread::get_id() << endl;
        return 8;
    }
}


#include <windows.h>

//#define __WINDOWSJQ_
/**
 * 十二、windows 临界区、其他各种mutex互斥量
 */
namespace s9{
    //  本类自动释放windows下临界区，防止忘记离开临界区
    // RAII (Resource Acquisition is initialization) 资源获取及初始化

    class CWinLock{
    public:
        CWinLock(CRITICAL_SECTION *pCritmp){
            m_pWinsec = pCritmp;
            EnterCriticalSection(m_pWinsec);
        }
        ~CWinLock(){
            LeaveCriticalSection(m_pWinsec);
        }
    private:
        CRITICAL_SECTION *m_pWinsec;
    };

    class A{
    public:
        void inMsgRecvQueue(){
            for(int i = 0; i< 1000; ++i){
                cout << "inMsgRecvQueue()执行、插入一个元素" << i << endl;
                /**
                 * 加锁解锁要保护的数据
                 */
#ifdef __WINDOWSJQ_
                // 可以重复进临界区，但是进去多少次 就要离开多少次
//                EnterCriticalSection(&my_winsec);  // 进入临界区
//                EnterCriticalSection(&my_winsec);   // 进入临界区
                CWinLock vLock(&my_winsec);         // windows 下的自动释放类
                CWinLock vLock1(&my_winsec);         // 也可以进多次
                msgRecvQueue.push_back(i);
//                LeaveCriticalSection(&my_winsec);   // 离开临界区
//                LeaveCriticalSection(&my_winsec);   // 离开临界区
#else
//                my_recu_mutex.lock();
//                my_mutex.lock();
                // 解除关系
//                testFunc1();
                // 超时锁的用法
                std::chrono::milliseconds timeout(100);
//                my_mutex.try_lock_for(timeout)
                if(my_mutex.try_lock_until(std::chrono::steady_clock::now() + timeout)){   // 等待100ms 尝试获取锁
                    // 拿到了锁
                    msgRecvQueue.push_back(i);
                    my_mutex.unlock();
                }
                else{
                    cout << "没拿到锁头" << endl;
                    std::chrono::milliseconds timeout(100);
                    std::this_thread::sleep_for(timeout);
                }
                // 这里必须手动解除
//                my_mutex.unlock();
//                my_recu_mutex.unlock();
#endif
            }
        }

        bool outMsgLULProc(int& command){
#ifdef __WINDOWSJQ_
            EnterCriticalSection(&my_winsec);
            if(!msgRecvQueue.empty()){
                // 消息队列不为空
                int command = msgRecvQueue.front();// 返回第一个元素，不检查元素是否存在
                msgRecvQueue.pop_front();
                LeaveCriticalSection(&my_winsec); //这里别忘了
                return true;
            }
            LeaveCriticalSection(&my_winsec);
#else
//            unique_lock<std::mutex> sbGuard1(my_mutex);
//            unique_lock<std::recursive_mutex> sbGuard1(my_recu_mutex);
            unique_lock<std::timed_mutex> sbGuard1(my_mutex);
            std::chrono::milliseconds timeout(10000000);
            std::this_thread::sleep_for(timeout);
            if(!msgRecvQueue.empty()){
                // 消息队列不为空
                int command = msgRecvQueue.front();// 返回第一个元素，不检查元素是否存在
                msgRecvQueue.pop_front();
                return true;
            }
#endif
            return false;
        }

        // 把数据从消息队列中取出的线程
        void outMsgRecvQueue(){
            int command = 0;
            for(int i = 0; i< 1000; ++i){
                bool result = outMsgLULProc(command);
                if(result){
                    cout << "outMsgRecvQueue()执行、取出一个元素" << command << endl;
                }
                else cout << "outMsgRecvQueue()执行、消息队列为空" << i << endl;
            }
            cout << "end " << endl;
        }

        A(){
#ifdef __WINDOWSJQ_
            InitializeCriticalSection(&my_winsec);
#endif
        }

        /**
         * 假设有一种场景，在testFunc1 需要调用 testFunc2
         */

        void testFunc1(){
            std::lock_guard<std::recursive_mutex> sbGuard(my_recu_mutex);
            testFunc2();
        }

        void testFunc2(){
            std::lock_guard<std::recursive_mutex> sbGuard(my_recu_mutex);
        }
    private:
        std::list<int> msgRecvQueue;
//        std::mutex my_mutex;
        // 递归互斥锁
        std::recursive_mutex my_recu_mutex;
        std::timed_mutex my_mutex;
//        std::recursive_timed_mutex my_recu_mutex


#ifdef __WINDOWSJQ_
        CRITICAL_SECTION my_winsec;  // windows 临界区：用之前必须先初始化
#endif
    };

}

/**
 * 十三、补充知识、线程池浅谈、数量谈、总结
 *
 */
namespace s10{
    class A{
    public:
        // 把收到的消息(玩家命令)放入到一个队列
        void inMsgRecvQueue(){
            for(int i = 0; i< 100000; ++i){
                unique_lock<std::mutex> sbGuard(my_mutex);
                cout << "inMsgRecvQueue()执行、插入一个元素" << i << endl;
                msgRecvQueue.push_back(i);
                // 通知其他某一个线程，将wait()唤醒，但是outMsgRecvQueue正处理一个事务，需要处理一段时间，而不是正卡在wait()
                // 那么此时 notify_one() 可能没有效果，还有可能另外一个线程一直都没有获得锁，这是有可能出现的问题
                my_cond.notify_one();
                my_cond.notify_one();
//                my_cond.notify_all(); // 唤醒其他所有线程
            }
        }

        // 得到通知的时候 我们才去取数据
//        bool outMsgLULProc(int& command){
//
//            if(!msgRecvQueue.empty()){
//                std::unique_lock<std::mutex> sbGuard(my_mutex);
//                if(!msgRecvQueue.empty()){
//                    // 消息队列不为空
//                    int command = msgRecvQueue.front();// 返回第一个元素，不检查元素是否存在
//                    msgRecvQueue.pop_front();
//                    return true;
//                }
//                return false;
//            }
//        }
        // 把数据从消息队列中取出的线程
        void outMsgRecvQueue(){
            int command = 0;
            while(true){
                std::unique_lock<std::mutex> sbGuard(my_mutex);
                // wait 等一个东西，
                // 如果第二个参数的返回值为false，那么wait将解锁互斥量，并堵塞到本行，
                // 堵塞到某个线程调用notify_one()成员函数为止
                // 如果第二个参数返回值为true，wait()直接返回
                // 如果没有第二个参数：那就和第二参数为false类似堵塞到某个线程调用notify_one()成员函数为止

                // 当其他线程用notify_one() 将本wait() 唤醒后，wait就开始恢复干活：
                // a):wait() 会尝试再次获取互斥锁，如果获取不到 流程就会卡在这等待获取，如果获取到 ，就继续执行b
                // b):1、如果wait有第二个参数 就判断这个表达式，如果表达式返回false，那wait又对互斥量解锁，再次休眠
                //    2、如果第二个参数返回值为true，那么就会wait就返回，就走下来了，互斥锁还是被锁住了
                //    3、如果没有第二参数，则 wait返回，流程走下来
                my_cond.wait(sbGuard,[this]{
                    // 解决虚假唤醒
                    if(!msgRecvQueue.empty()) return true;
                    return false;
                });
                // 流程走到这里来 互斥锁一定是锁住的,至少有一个元素
                command = msgRecvQueue.front();
                msgRecvQueue.pop_front();
                cout << "outMsgRecvQueue()执行、取出一个元素" << command << "线程id：" << std::this_thread::get_id() << endl;
                sbGuard.unlock();

                //
                //处理别的事情...
            }
//            for(int i = 0; i< 100000; ++i){
//                bool result = outMsgLULProc(command);
//                if(result){
//                    cout << "outMsgRecvQueue()执行、取出一个元素" << command << endl;
//                }
//                else cout << "outMsgRecvQueue()执行、消息队列为空" << i << endl;
//            }
//            cout << "end " << endl;
        }
    private:
        std::list<int> msgRecvQueue;
        std::mutex my_mutex;
        std::condition_variable my_cond;  // 条件变量对象
    };
}

int main(){

    /**
     * 一、测试双线程读写 对应命名空间 s1
     *
     */
#if 0

    s1::A myObja;
    thread myOutMsgObj(&s1::A::outMsgRecvQueue,&myObja); // 第二个参数是引用 才可以保证线程里调用的是一个对象
    thread myInMsgObj(&s1::A::inMsgRecvQueue,&myObja);
    myOutMsgObj.join();
    myInMsgObj.join();

#endif

    /**
     *  对应 命名空间 s2
     * 二、保护共享数据、操作时、用代码把共享数据锁住、操作数据、解锁
     *      其他想操作共享数据的操作的线程必须等待解锁
     *  1、互斥量（mutex）基本概念
     *     互斥量是一个类对象、理解成一把锁、多个线程尝试用lock()成员函数加锁这把锁头，
     *     只有一个线程能锁定成功（成功的标志是lock()返回了,否则一直阻塞)
     *     互斥量保护数据不多也不少。少了不能达到保护效果，多了效率低下
     *
     *  2、 lock(),unlock(): 先lock,操作共享数据，然后unlock()
     *      lock()和 unlock(),要成对使用，否则很难排查
     *      为了防止忘记unlock(),引入std::lock_guard的类模板，替你 unlock()
     *  3、 lock_guard 取代lock() 和unlock()
     *      实现原理构造函数执行lock，析构函数执行unlock
     *
     *  4、 死锁：都在等对面解锁，当我们程序中需要加多个锁的时候
     *      cpp中：两把锁(两个互斥量)、金锁 、银锁
     *      出现的例子：两个线程 A、B
     *      1、线程A执行的时候 先把金锁锁住，把金锁lock()、然后它准备去lock()银锁
     *      2、线程B执行了，先把银锁锁住、把银锁lock()、然后它准备去lock() 金锁
     *      此时死锁就发生了
     *      死锁的解决方案：一般加锁顺序一样就不会产生死锁
     *  5、std::lock()函数模板
     *      能力：一次锁住两个或者两个以上的互斥量（至少两个，多了不限）
     *      它不存在产生死锁问题：
     *      std::lock() 如果互斥量有一个没锁住、它就在那里等着，等所有互斥量都锁住，才能往下走，如果锁不住，就会把自己锁住的释放掉
     *      要么两个互斥量都锁、要么都不锁住
     *
     *  6、std::lock_guard : 对于多个锁也不想要自己unlock
     */
#if 0
    s2::A myObja;
    thread myOutMsgObj(&s2::A::outMsgRecvQueue,&myObja); // 第二个参数是引用 才可以保证线程里调用的是一个对象
    thread myInMsgObj(&s2::A::inMsgRecvQueue,&myObja);
    myOutMsgObj.join();
    myInMsgObj.join();
#endif


    /**
     * 对应代码空间 s3
     * 1、 unique_lock 取代lock_guard
     *    unique_lock 是一个类模板 工作中一般lock_guard(推荐使用)
     *    unique_lock 更灵活：效率上差一点、内存占用多一点，问题不是很大
     *
     *
     *
     * 2、unique_lock 第二参数
     *     unique_lock 还支持一些lock_guard 不支持的标志
     *     1、std::adopt_lock : 表示这个互斥量已经被lock了（你必须提前lock，否则报异常），通知lock
     *     2、std::try_to_lock 一个线程卡住20s 另一个线程也会卡住，不灵活：unique_lock 第二个参数 try_to_lock 解决这个问题
     *          我们会尝试用mutex的lock(),去锁住这个mutex 但如果没有锁定成功，我也会立即返回、并不会阻塞在那里
     *          用这个try_to_lcok  之前不能用lock 否则会锁住两次
     *     3、std::defer_lock 第二参数
     *          前提：不能先lock(),否则会异常,defer_lock 表示没有给mutex加锁、初始化了一个没有加锁的 mutex，这样就可以调用
     *          unique_lock 的重要成员函数
     * 3、成员函数
     *      1、lock()
     *      2、unlock()  有一些非共享代码要处理 可以先unlock() 然后再lock()
     *      3、try_lock() 如果拿不到锁 返回false 拿到了 返回true  和defer_lock 搭配使用
     *      4、realease() 返回它所管理的mutex指针 并释放所有权 这个unique_lock 和mutex 不再有关系
     *          这个和unlock() 不一样，放弃unique_lock 和 mutex的绑定关系，之后就要自己管理锁
     *          如果之前是加锁 你需要手动解锁
     *          返回值 是原本的互斥量的指针
     *
     * 4、unique_lock 所有权的传递
     *      1、一个mutex 应该只和一个unique_lock 绑定在一起，这个时候unique_lock 就拥有 一个mutex的所有权
     *          可以转移给其他的unique_lock 对象
     *          unique_lock对象这个mutex的所有权不能复制 mutex也没有复制构造函数
     *         移动语义 这个时候就直接把所有权转移到了sbGuard2了
     *        unique_lock<std::mutex> sbGuard1(my_mutex);
              unique_lock<std::mutex> sbGuard2(std::move(sbGuard1));
           2、也可以作为返回值 ：
                unique_lock<std::mutex> sbGuard2 = rtn_unique_lock();


     *
     */
#if 0
    s3::A myObja;
    thread myOutMsgObj(&s3::A::outMsgRecvQueue,&myObja); // 第二个参数是引用 才可以保证线程里调用的是一个对象
    thread myInMsgObj(&s3::A::inMsgRecvQueue,&myObja);
    myOutMsgObj.join();
    myInMsgObj.join();

#endif

    /**
     *  单例设计模式 分析 对应代码空间 s4
     *    1、设计模式大概谈：
     *          一些代码的写法（比较特殊，和常规写法不太一样）：程序灵活，维护起来可能方便，但是别人接管和阅读代码会很痛苦
     *          用设计模式理念写出来的代码很晦涩《head first》，为了应付特别大的项目，把项目的开发经验、模块划分经验，总结
     *          整理成 “ 设计模式 ”，设计模式到中国来，拿着程序往设计模式上套，一个小小的项目，没必要用设计模式，本末倒置
     *          （把简单的写成复杂的，这样不好）
     *    2、单例设计模式：整个项目中，由某个或者某些特殊的类，属于该类的对象，我只能创建一个，多了我创建不了
     *          使用频率高
     *    3、单例设计模式，多线程安全问题分析解决
     *          需要在我们自己创建的线程（而不是主线程）创建MyCAS这个单例类的对象，这种线程可能还不只一个
     *          GetInstance() 要互斥
     *    4、std::call_once():函数模板 C++11 引入的 第二个参数是一个函数名
     *          功能：保证函数只被调用一次 ：具备互斥量的能力，效率上比互斥量消耗的资源更少
     *          call_once() 需要和一个标记结合使用 std::once_flag,
     *          call_once() 通过标记决定对应的函数是否执行， 调用call_once成功够，就把std::once_flag 设置为一种已调用
     *          状态
     */
#if 0
//    s4::edition1::MyCAS* a = s4::edition1::MyCAS::GetInstance();
//    s4::edition1::MyCAS* b = s4::edition1::MyCAS::GetInstance();
//    a->func();
//    b->func();
    // 两个线程入口函数相同,有两条通路 同时执行这个getInstance(),会出问题
//    std::thread myObj1(s4::myThread);
//    std::thread myObj2(s4::myThread);
//    myObj1.join();
//    myObj2.join();
        // call_once
    std::thread myObj1(s4::myThread2);
    std::thread myObj2(s4::myThread2);
    myObj1.join();
    myObj2.join();

#endif

    /**
     * 1、条件变量：condition_variable、wait()、notify_one()、notify_all
     * 2、   线程 A：等待一个条件满足
     *       线程 B：专门往消息队列中扔消息
     * 3、   condition_variable 实际上是一个类，是一个和条件相关的类，说白了就是等待一个条件达成
     *       这个类需要和互斥量配合工作
     * 4、 上述代码深入思考
     * 5、 notify_all()
     */

#if 0

    s5::A myObja;
    thread myOutMsgObj(&s5::A::outMsgRecvQueue,&myObja); // 第二个参数是引用 才可以保证线程里调用的是一个对象
    thread myOutMsgObj2(&s5::A::outMsgRecvQueue,&myObja); // 为了演示 notify_all()
    thread myInMsgObj(&s5::A::inMsgRecvQueue,&myObja);
    myOutMsgObj.join();
    myOutMsgObj2.join();
    myInMsgObj.join();

#endif
    /**
     * 一、 async、future、packaged_task、promise
     * 1、希望线程返回一个结果：std::async,std::future
     *      std::async 是一个函数模板，启动一个异步任务 返回一个std::future（类模板） 对象
     *      std::async 就是自动创建一个线程，并开始执行相应的线程入口函数，
     *      返回值std::future对象里面就有线程入口函数所返回的结果（就是线程返回的结果）
     *      可以通过future对象的成员函数get(), 获取结果：std::future提供了一种访问异步操作结果的机制
     *      （这个结果你可能不能马上拿到，但是在不久的将来，线程函数执行完毕的时候，future对象会保存一个值）
     *      get()函数只能调用一次，不能连续调用多次，否则会有异常，而且应该使用get() 或者wait() 等待子线程结束
     *      如果没有get() 程序会等待子线程执行完 再退出，主线程会走到最后等待，不退出
     *      通过额外向std::async() 传递一个参数 参数类型 为std::launch类型（枚举类型）
     *      a) std::launch::deferred 表示线程入口函数调用被延迟到std::future的wait()或者get()函数调用才执行
     *          如果wait或者get没有被调用 线程会执行吗，答：不仅不会执行，线程实际没有创建
     *      b) std::launch::async 表示调用async函数的时候 就开始创建线程
     * 2、package_task 打包任务（类模板）
     *      模板参数 是可调用对象，通过package_task可以把各种可调用对象包装起来，方便将来作为线程入口函数
     *      package_task包装起来的可调用对象还可以直接调用，也就是说它本身也是可以调用
     *
     * 3、std::promise 类模板
     *      我们能够在某个线程中给它赋值，在其他线程中 把这个值取出来用,实现线程之间的通信
     */
#if 0
    s6::A a;
    int tmp = 12;
    cout << "hello world" << "main thread id: " << std::this_thread::get_id() <<endl;
    // 自动创建一个线程并执行，result 是一个将来值
//    std::future<int> result = std::async(s6::myThread);
    // 以成员函数作为线程入口，第二个参数用的引用 才保住线程里是一个对象 这里好像没有创建线程，因为线程id是一样的
//    std::future<int> result = std::async(&s6::A::myThread, &a, tmp);
    // 表示延迟调用线程函数 直到get或者wait调用
//    std::future<int> result = std::async(std::launch::deferred,&s6::A::myThread, &a, tmp);
    //    表示调用async函数的时候 就开始创建线程,立即执行线程函数
//    std::future<int> result = std::async(std::launch::async,&s6::A::myThread, &a, tmp);
//    cout << "continue.......!" << endl;
//    int def = 0;
    // result.get() 这里会卡住 等待线程函数执行完毕 拿到结果
//    result.wait(); // 等待线程返回，但是不返回结果 类似于join()
//    cout << result.get() << endl;
//    cout << result.get() << endl;  // 不可以连续两次get()

    /**
     * 1、packaged_task 包装可调用对象
     */
//    std::packaged_task<int(int)> mypt(s6::myThread2);  // 把函数s6::myThread2() 包装起来
//    // 可调用对象、参数
//    std::thread t1(std::ref(mypt),1);  // 线程直接开始执行 ，1 是作为线程入口函数的参数
//    t1.join();
//    // 获取返回结果
//    std::future<int> result = mypt.get_future();
//    cout << result.get() << endl;

    /**
     * 2、packaged_task 包装lambda 表达式
     */

//    function<int(int)> ll;
//    ll = [](int x) -> int {
//        cout << x << endl;
//        cout << "myThread start " << "thread id: " << std::this_thread::get_id() << endl;
//        std::chrono::milliseconds dura(5000); // 休息 5 s
//        std::this_thread::sleep_for(dura);
//        cout << "myThread end " << "thread id: " << std::this_thread::get_id() << endl;
//        return 2;
//    };
//    std::packaged_task<int(int)> mypt2(std::ref(ll));

//    std::thread t2(std::ref(mypt2),1);  // 线程直接开始执行 ，1 是作为线程入口函数的参数
//    t2.join();
    // 获取返回结果
//    std::future<int> result2 = mypt2.get_future();
//    cout << result2.get() << endl;
    /**
     * 3、packaged_task 可以直接调用
     */
//    mypt2(20);  // 直接调用，相当于函数调用
//    std::future<int> result3 = mypt2.get_future();
//    cout << result3.get() << endl;

/**
 * 容器packaged_task
 */
//    vector<packaged_task<int(int)>> myTasks;
//    // 把对象 移到容器里
//    myTasks.push_back(std::move(mypt2));
//    std::packaged_task<int(int)> mypt3;
//    auto iter = myTasks.begin();
//    mypt3 = std::move(*iter);
//    myTasks.erase(iter);// 迭代器失效,移除容器里那一项
//    mypt3(10);
//    std::future<int> result = mypt3.get_future();
//    cout << result.get() << endl;
/**
 *   4、std::promise
 */
    std::promise<int> myProm;  // 声明一个对象，保存一个int
    std::thread t1(s6::myThread3,std::ref(myProm),180);
    t1.join(); // 如果你不等 会报异常

    //获取结果值
    std::future<int> result = myProm.get_future();
//    cout  << result.get() << endl;
    std::thread t2(s6::myThread4,std::ref(result));
    t2.join();

#endif

    /**
     * 一、 future其他成员函数、shared_future、atomic
     * 1、 future其他成员函数：std::future_status status wait_for()
     * 2、 share_future （类模板）解决多个线程都像得到结果，他的get() 是复制
     * 3、atomic 原子操作：
     *      互斥量：多线程编程中，保护共享数据：先锁，操作共享数据，开锁
     *      有两个线程对一个变量进行操作atomVal共享变量，一个线程读、一个线程写，即使这样也会出问题
     *      原子操作不需要哦用到互斥量加锁（无锁）技术的多线程并发编程方式
     *      在多线程中，不会被打断的程序执行片段，效率比互斥量高，原子操作是不可分割的状态
     *      互斥量是针对一个代码段，往往是一个代码段，而原子操作一般对某个变量操作
     *      std::atomic 类模板，为了封装某个类型的值
     *      // 读线程
     *      int temVal = atomVal;
     *      //写线程
     *      atomVal = 6;
     *
     */

#if 0
    /**
     * 1、std::future_status status wait_for()
     */
    cout << "hello world" << "main thread id: " << std::this_thread::get_id() <<endl;
//     自动创建一个线程并执行，result 是一个将来值
//    std::future<int> result = std::async(std::launch::async,s7::myThread);
//    std::future<int> result = std::async(std::launch::deferred,s7::myThread);
//    cout << "continue.......!" << endl;
//    std::future_status status = result.wait_for(std::chrono::seconds(6)); // 等待一秒
//    if(status == std::future_status::timeout){  //超时：我想等待1s，希望你返回，但是你没有返回，所以超时
//        //表示线程还没执行完，
//        cout << "超时，线程还没执行完" << endl;
//    }
//    else if(status == std::future_status::ready){
//        cout << " 线程成功返回 " << endl;
//    }
//    else if(status == std::future_status::deferred){
//        cout << "线程被延迟执行" << endl;
//        // 这个线程函数是在主线程执行的，相当于没有创建子线程
//        cout << result.get() << endl;
//    }

/**
 *  2、share_future，（类模板）解决多个线程都像得到结果，他的get() 是复制
 *  可以get多次
 */
//    std::packaged_task<int(int)> mypt(s7::myThread2);
//    std::thread t1(std::ref(mypt),1);
//    t1.join();
//    std::future<int> result = mypt.get_future();
//    std::shared_future<int> result_s(std::move(result));
//    bool ifcanget = result.valid();
//    std::shared_future<int> result_s(result.share());
//    std::shared_future<int> result_s(mypt.get_future()); //通过get_future 构造一个shared_future 对象
//    auto mythreadRes = result_s.get();
//    mythreadRes = result_s.get();
//    ifcanget = result.valid();

//    std::thread t2(s7::myThread4,std::ref(result_s));
//    t2.join();
//    cout << result_s.get() << endl;
//    cout << result_s.get() << endl;
//    cout << result_s.get() << endl;

    /**
     * 3、原子操作：atomic
     */
//    hello worldmain thread id: 1
//    最后的结果：1607245
//    over
//    请按任意键继续. . .
    thread myObj1(s7::myThreadAtomicBool);
    thread myObj2(s7::myThreadAtomicBool);
    std::chrono::seconds dura(5);
    std::this_thread::sleep_for(dura);
    s7::g_ifend = true;
    myObj2.join();
    myObj1.join();

    cout << "最后的结果：" << s7::g_mycout << endl;
    cout << "最后的结果：" << s7::g_atomic << endl;

#endif

    /**
     * atomic 续谈、async 深入谈
     *  1、 atomic 针对 ++、--，+=，&=、|=、^=等一元是可以的，但是对有些操作不是原子操作，所以要注意
     *
     *  2、async
     *      参数详述，async用来创建一异步任务，一般不叫创建一个线程
     *      std::thread() 如果系统资源紧张，那么可能创建线程就会失败，那么执行std::thread(),就会导致整个程序可能崩溃
     *      上述最明显不同的时候，有时候async并不创建线程，而是在get()所在线程中执行线程入口函数
     *      1）如果用std::launch::defferred 作为第一参数
     *          没有创建线程，而是在调用get()时所在线程中执行线程入口函数，如果没有调用则入口函数不会执行
     *      2）如果std::launch::async 作为第一参数，则必须创建一个线程
     *      3）如果同时用 std::launch::async |std::launch::defferred,
     *          意味着可能是std::launch::async
     *             也可能是 std::launch::defferred
     *      4）如果没有默认第一参数 默认是 std::launch::async |std::launch::defferred (不确定)
     *          让系统决定自行决定是同步、还是异步，系统如何决定？
     * 3、std::thread()和 std::async() 的区别
     *      std::thread():一定会创建线程，如果创建失败，程序直接崩溃、返回值不好接，要用全局量或者package_list 或者promise
     *      std::async: 创建异步任务：可能创建线程、可能不创建线程，返回值是std::future<T> 容易拿到返回值，一般不会异常不会
     *      崩溃，因为如果系统紧张，不加额外参数的调用或者std::launch::defferred，那么它不会创建线程，而是在get() 处的线程
     *      执行，如果你强制使用std::launch::async ，可能会导致系统崩溃
     *
     * 4、经验：一个程序里线程数量不宜超过100-200
     *
     * 5、std::async不确定性问题的解决： std::future<int> result = std::async(s8::myThread);
     *      问题焦点在于：异步任务不一定会被执行，要判断有没有被推迟执行，要使用wait_for函数
     *
     */

#if 0
    /**
     * 1、atomic 续谈
     */
//    thread myObj1(s8::myThreadAtomic);
//    thread myObj2(s8::myThreadAtomic);
//
//    myObj2.join();
//    myObj1.join();
//
//    cout << "最后的结果：" << s8::g_mycout << endl;
//    cout << "最后的结果：" << s8::g_atomic << endl;

    /**
     * 2、async 深入谈
     * std::thread() 如果系统资源紧张，那么可能创建线程就会失败，那么执行std::thread(),就会导致整个程序可能崩溃
     *
     */
    std::future<int> result = std::async(s8::myThread); // 不确定情况
//    std::future<int> result = std::async(std::launch::async,s8::myThread);
//    std::future<int> result = std::async(std::launch::async|std::launch::deferred ,s8::myThread); // 不确定情况
    cout << "continue.......!" << endl;
//    std::future_status status = result.wait_for(std::chrono::seconds(0)); // 等待一秒
    std::future_status status = result.wait_for(0s); // 等待一秒

    if(status == std::future_status::deferred){
        cout << "线程被延迟执行,没有开启线程" << endl;
        // 这个线程函数是在主线程执行的，相当于没有创建子线程,这时候手动执行线程函数
        cout << result.get() << endl;
    }
    else if(status == std::future_status::timeout){  //超时：我想等待1s，希望你返回，但是你没有返回，所以超时
        //表示线程还没执行完，
        cout << "超时，线程还没执行完" << endl;
    }
    else if(status == std::future_status::ready){
        cout << " 线程成功返回 " << endl;
    }

#endif
    /**
     * 十二、windows 临界区、其他各种mutex互斥量
     * 1、windows 临界区: 在windwos下的临界区，和C++11 mutex非常类似
     *
     * 2、多次进入临界区试验
     *      在同一个线程中，可以重复进去相同的临界区变量，但是也要相应出来几次，C++11 互斥锁不可以
     * 3、自动析构技术，实现windows 下面的临界区
     *
     * 4、recursive_mutex 递归的独占互斥量,当需要多次加锁的时候会用到
     *      std::mutex 独占互斥量
     *      std::recursive_mutex 递归独占互斥量,允许同一个线程，同一个互斥量多次被lock()
     *      也有lock() 和unlock() 递归测试次数有限制
     *
     * 5、带超时的(递归)互斥量std::timed_mutex和 std::recursive_timed_mutex
     *      等待一段时间
     *      1、try_lock_for(),如果等待一段时间拿到了锁或者超时都会继续走下去
     *      2、try_lock_until() 参数是一个未来的时间点，在未来的时间没到的时间段内，如果拿到了锁 流程就走下来
     *                         如果时间到了，还没拿到锁，也会走下来
     *
     *
     */
#if 0
    s9::A myObja;
    thread myOutMsgObj(&s9::A::outMsgRecvQueue,&myObja); // 第二个参数是引用 才可以保证线程里调用的是一个对象
    thread myInMsgObj(&s9::A::inMsgRecvQueue,&myObja);
    myOutMsgObj.join();
    myInMsgObj.join();

#endif
    /**
     * 十三、补充知识、线程池浅谈、数量谈、总结
     * 1、虚假唤醒
     *      wait(),notify_one(),notify_all()
     *          my_cond.wait(sbGuard,[this]{
                    // 解决虚假唤醒
                    if(!msgRecvQueue.empty()) return true;
                    return false;
                });
        2、atomic<int> a(1);  // 操作原子 就不让任务切换
            void thread(){
                a++;
            }
            while(true){
                cout << a << endl;   // 读atm是原子操作,但是整个操作不是原子操作
                auto atm2 = a;       // 这里是不合法的 没有拷贝构造函数，赋值也不可以
                atomic<int> atm2(atm.load())  // 这里是可以的,读
                auto atm2(atm.load())

                atm2.store(12)       // 写
            }

     */
#if 1

    s10::A myObja;
    thread myOutMsgObj(&s10::A::outMsgRecvQueue,&myObja); // 第二个参数是引用 才可以保证线程里调用的是一个对象
    thread myInMsgObj(&s10::A::inMsgRecvQueue,&myObja);
    myOutMsgObj.join();
    myInMsgObj.join();

#endif
    cout << "main() start " << "main thread id :" << this_thread::get_id() << endl;
    cout << "over " << endl;
//    system("pause");
    return 0;
}
