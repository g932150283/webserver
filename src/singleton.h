#ifndef _WEBSERVER_SINGLETON_H_
#define _WEBSERVER_SINGLETON_H_

#include<memory>

namespace webserver{

// 防止多个实例
/**
 * @brief 单例模式封装类
 * 
 * @tparam T 类型 
 * @tparam X 为了创造多个实例对应的Tag
 * @tparam N 同一个Tag创造多个实例索引
 * T 是实际单例对象的类型
 * X 和 N 是为了创建多个实例而引入的标签
 * X 用于标识不同的实例，N 用于同一个标签下创建多个实例时的索引
 */
template<class T, class X = void, int N = 0>
class Singleton {
public:
    /**
     * @brief 返回单例裸指针
     * 返回一个指向单例对象的裸指针。单例对象在函数内部是静态的，确保只有一个实例被创建。
     * 每次调用 GetInstance() 都返回相同的指针，因此确保了只有一个实例存在。
     */
    static T* GetInstance() {
        static T v;
        return &v;
        //return &GetInstanceX<T, X, N>();
    }
};

/**
 * @brief 单例模式智能指针封装类
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
    /**
     * @brief 返回单例智能指针
     * 返回一个指向单例对象的 std::shared_ptr 智能指针。
     * 同样，单例对象在函数内部是静态的，确保只有一个实例被创建。
     * 每次调用 GetInstance() 都返回相同的智能指针，确保只有一个实例存在，并且智能指针管理这个实例的生命周期。
     */
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};










}

#endif