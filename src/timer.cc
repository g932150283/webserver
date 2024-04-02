#include "timer.h"
#include "util.h"

namespace webserver {

/**
 * @brief Timer::Comparator::operator() 是 Timer 类中的 Comparator 结构体的函数调用运算符重载，用于比较 Timer 智能指针的优先级。
 * @param lhs 一个 Timer 智能指针，用于比较。
 * @param rhs 另一个 Timer 智能指针，用于比较。
 * @return 如果 lhs 的 m_next 小于 rhs 的 m_next，或者如果 lhs 为空且 rhs 不为空，则返回 true；否则返回 false。
 */
bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
    // 检查 lhs 和 rhs 是否都为空 比较定时器的智能指针的大小(按执行时间排序)
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    if (lhs->m_next < rhs->m_next) {
        return true;
    }
    if (rhs->m_next < lhs->m_next) {
        return false;
    }
    return lhs.get() < rhs.get();
}


// 只能由TimerManager创建Timer
Timer::Timer(uint64_t ms, std::function<void()> cb,
             bool recurring, TimerManager* manager)
    :m_recurring(recurring)
    ,m_ms(ms)
    ,m_cb(cb)
    ,m_manager(manager) {
        // 执行时间为当前时间+执行周期
    m_next = webserver::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next)
    :m_next(next) {
}

// 取消定时器，防止其回调函数被执行。
bool Timer::cancel() {
    /*
    这段代码定义了Timer类的成员函数cancel，其作用是取消当前定时器，防止其回调函数在未来被调用。
    如果定时器成功取消，函数返回true；如果定时器已经被取消或不存在，函数返回false。

    首先通过锁定TimerManager的互斥锁来确保线程安全。
    然后，它检查当前定时器是否有效（即回调函数是否已经设置）。
    如果有效，它会将回调函数设为nullptr以表示定时器已被取消，并从TimerManager的定时器集合中移除该定时器。
    如果定时器已经是无效状态（即回调函数已经是nullptr），则函数直接返回false，表示没有定时器被取消。
    这种设计确保了定时器的正确取消，并防止了无效的回调执行。
    */
    // 获取对应TimerManager的互斥锁，以保证线程安全。
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);

    // 检查定时器的回调函数是否已经设置（即定时器是否有效）。
    if(m_cb) {
        // 将定时器的回调函数设置为nullptr，表示定时器已被取消。
        m_cb = nullptr;

        // 在TimerManager的定时器集合中找到当前定时器的迭代器。
        // 使用shared_from_this获取当前对象的std::shared_ptr。
        auto it = m_manager->m_timers.find(shared_from_this());

        // 从TimerManager的定时器集合中移除当前定时器。
        m_manager->m_timers.erase(it);

        // 定时器成功取消，返回true。
        return true;
    }

    // 如果定时器的回调函数已经是nullptr（即定时器已经被取消或无效），则返回false。
    return false;
}


// 重新设置定时器的到期时间。
bool Timer::refresh() {
    /*
    这段代码定义了Timer类的成员函数refresh，旨在重新设置定时器，以使其从当前时间开始再次计算到期时间。
    如果操作成功，函数返回true；如果定时器无效或不存在于管理器中，函数返回false。

    此函数通过锁定TimerManager的互斥锁来确保线程安全地操作定时器集合。
    它首先检查定时器是否有效（即回调函数m_cb是否非空），然后确认定时器是否仍存在于TimerManager的定时器集合中。
    如果这些检查通过，函数将从集合中移除定时器，重新计算其到期时间，并将其再次添加到集合中。
    这样，定时器就被“刷新”了，即其倒计时重新开始。
    这种机制允许动态调整定时器的触发时间，适应可能发生的时间变化或新的需求。
    */
    // 获取对应TimerManager的互斥锁，以保证线程安全。
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);

    // 检查定时器的回调函数是否存在。如果回调函数为空（m_cb），表示定时器已被取消或无效，因此返回false。
    if(!m_cb) {
        return false;
    }

    // 在TimerManager的定时器集合中查找当前定时器的迭代器。
    // 使用shared_from_this来获取当前对象的std::shared_ptr。
    auto it = m_manager->m_timers.find(shared_from_this());

    // 如果定时器不存在于TimerManager的集合中，表示定时器无效或已被移除，因此返回false。
    if(it == m_manager->m_timers.end()) {
        return false;
    }

    // 从TimerManager的定时器集合中移除当前定时器。
    m_manager->m_timers.erase(it);

    // 重新计算定时器的到期时间。m_next设置为当前时间加上定时器原始的间隔时间m_ms。
    m_next = webserver::GetCurrentMS() + m_ms;

    // 将定时器重新插入到TimerManager的定时器集合中。
    m_manager->m_timers.insert(shared_from_this());

    // 定时器成功刷新，返回true。
    return true;
}


// 重置定时器的超时时间。 from_now是否从当前时间开始计算
bool Timer::reset(uint64_t ms, bool from_now) {
    /*
    这段代码定义了Timer类的成员函数reset，其功能是重置定时器的超时时间。
    根据参数from_now，重置操作要么基于当前时间（from_now为true），要么基于原始设定的超时时间（from_now为false）。
    如果操作成功，函数返回true；如果定时器无效或不存在于管理器中，函数返回false。

    这个函数通过锁定TimerManager的互斥锁来确保线程安全地操作定时器集合。
    首先检查是否真的需要重置（基于新的间隔时间和是否从现在开始计时）。
    如果需要重置，函数会从定时器集合中移除当前定时器，计算新的起始时间，更新定时器的间隔时间和下一个到期时间，然后再将定时器添加回定时器集合。
    这样，定时器就按照新的设置被重置，准备再次触发。


    
    */
    // 如果新的超时时间与当前设置相同，并且不是基于当前时间重置，则无需操作，直接返回true。
    if(ms == m_ms && !from_now) {
        return true;
    }

    // 获取对应TimerManager的互斥锁，以保证线程安全。
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);

    // 检查定时器的回调函数是否存在。如果回调函数为空（m_cb），表示定时器已被取消或无效，因此返回false。
    if(!m_cb) {
        return false;
    }

    // 在TimerManager的定时器集合中查找当前定时器的迭代器。
    auto it = m_manager->m_timers.find(shared_from_this());

    // 如果定时器不存在于TimerManager的集合中，表示定时器无效或已被移除，因此返回false。
    if(it == m_manager->m_timers.end()) {
        return false;
    }

    // 从TimerManager的定时器集合中移除当前定时器。
    m_manager->m_timers.erase(it);

    // 计算新的起始时间。
    uint64_t start = 0;
    if(from_now) {
        // 如果from_now为true，起始时间为当前时间。
        start = webserver::GetCurrentMS();
    } else {
        // 如果from_now为false，起始时间为上一个到期时间减去原间隔时间。
        start = m_next - m_ms;
    }

    // 更新定时器的间隔时间和下一个到期时间。
    m_ms = ms;
    m_next = start + m_ms;

    // 将定时器重新添加到TimerManager的定时器集合中。
    m_manager->addTimer(shared_from_this(), lock);

    // 定时器成功重置，返回true。
    return true;
}


// TimerManager的构造函数。
TimerManager::TimerManager() {
    // 初始化成员变量m_previouseTime为当前时间。
    // webserver::GetCurrentMS()是一个假定的函数，用于获取当前的系统时间，单位为毫秒。
    m_previouseTime = webserver::GetCurrentMS();
}

// TimerManager的析构函数。
TimerManager::~TimerManager() {
    // 析构函数体为空。
    // 在这里，你可以添加必要的清理代码，比如释放资源、关闭文件等。
    // 如果TimerManager持有需要手动释放或关闭的资源，在这里进行处理。
    /*
    在构造函数中，m_previouseTime成员变量被初始化为当前的系统时间。这个时间值主要用于检测系统时间的变化，包括可能的时钟回滚情况。

析构函数在这段代码中是空的，这意味着没有特别的清理工作需要在TimerManager对象被销毁时执行。
这是常见的，特别是当所有的资源都是自动管理的（例如，通过智能指针）。
然而，如果TimerManager在其生命周期内获取了需要手动管理的资源（如动态分配的内存、打开的文件句柄、网络连接等），
那么在析构函数中应该添加适当的代码来释放或关闭这些资源，以避免资源泄露。



    */
}


// 添加定时器
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {

    /*
    在TimerManager中创建并添加一个新的定时器对象
    在定时器管理器中添加一个新的定时器。
    函数首先创建一个新的Timer对象，然后使用一个写锁来确保在添加定时器到管理器中时的线程安全。
    最后，函数将新创建的定时器添加到管理器中，并返回该定时器的智能指针。
    */
    // 创建一个新的Timer对象，使用智能指针管理，避免内存泄漏。Timer的构造函数接收四个参数：
    // 1. ms：定时器超时时间，单位为毫秒。
    // 2. cb：定时器超时时调用的回调函数，类型为std::function<void()>，表示无参数、无返回值的函数。
    // 3. recurring：一个布尔值，指示定时器是否应该重复触发。如果为true，定时器超时后会自动重新设置。
    // 4. this：指向当前TimerManager对象的指针，表示定时器属于这个TimerManager。
    // Timer只能由TimerManager创建
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    
    // 创建一个写锁，用于同步对共享资源的访问。RWMutexType是一个读写互斥锁类型，
    // WriteLock是其写锁的构造函数，m_mutex是TimerManager中定义的一个读写互斥锁变量。
    // 创建写锁时，自动锁定互斥量，离开作用域时自动释放互斥量，防止死锁。
    RWMutexType::WriteLock lock(m_mutex);

    // 将新创建的定时器添加到定时器管理器中。addTimer是另一个成员函数，不是这个函数的递归调用。
    // 它接收两个参数：刚刚创建的定时器智能指针和当前的写锁。这个内部addTimer函数负责将定时器插入到管理器的适当位置。
    addTimer(timer, lock);

    // 返回新创建的定时器的智能指针。调用方可以使用这个智能指针与定时器进行交互，例如取消定时器。
    return timer;
}


// 定义一个静态回调函数，用于带有条件检查的定时器。
// 该函数首先尝试从一个弱智能指针中获取一个强引用，以确定关联对象的存活状态。
static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    /*
    这段代码定义了一个名为OnTimer的静态函数。此函数旨在作为条件定时器的回调函数，
    其中包含一个条件检查机制，确保只有在特定条件满足时，用户定义的回调cb才会被执行。

    这个设计允许定时器在执行回调之前进行条件检查，确保只有在关联的条件对象仍然存在（即未被销毁）时，回调函数才会被执行。
    这种机制在处理那些依赖于特定资源或状态的定时任务时非常有用，可以自动避免因资源已释放或状态已改变而导致的错误执行。
    */

    // 尝试从弱智能指针weak_cond中获取一个强智能指针tmp。
    // 如果关联的对象已经被销毁，weak_cond.lock()将返回一个空指针。
    /*
    weak_ptr是一种弱引用，它不会增加所指对象的引用计数，也不会阻止所指对象被销毁。
    shared_ptr是一种强引用，它会增加所指对象的引用计数，直到所有shared_ptr都被销毁才会释放所指对象的内存。
    在这段代码中，weak_cond是一个weak_ptr类型的对象，
    通过调用它的lock()方法可以得到一个shared_ptr类型的对象tmp，如果weak_ptr已经失效，则lock()方法返回一个空的shared_ptr对象
    */
    std::shared_ptr<void> tmp = weak_cond.lock();

    // 检查tmp是否非空，即检查关联的对象是否仍然存活。
    // 如果tmp非空，说明关联的对象仍然存在，条件满足。
    if(tmp) {
        // 在条件满足的情况下，执行用户定义的回调函数cb。
        // cb是一个无参数、无返回值的函数，其具体行为由用户在设置定时器时定义。
        cb();
    }
    // 如果tmp为空，即关联的对象已被销毁，函数将不执行任何操作，回调cb也不会被调用。
    // 这提供了一种自然的方式来防止在对象生命周期结束后执行无效的回调，避免潜在的错误和资源泄漏。
}



// 添加条件定时器
// 此函数创建一个定时器，该定时器在超时后会首先检查一个条件，只有在条件满足时才执行回调。
Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond, bool recurring) {
    /*
    添加一个带有条件的定时器，这种定时器在触发时会首先检查某个条件是否满足，然后再决定是否执行回调函数。

    在这个函数中，addTimer是TimerManager类的另一个成员函数，用于添加定时器。
    std::bind用于创建一个新的回调函数，它将OnTimer函数与weak_cond和cb绑定在一起。
    OnTimer应该是一个静态函数或者某种形式的回调处理函数，
    它会首先尝试从weak_cond中获取一个强引用（std::shared_ptr），
    如果成功，说明关联的条件对象仍然存在，然后它会调用cb。
    如果weak_cond已经过期（即关联的对象已被销毁），则不会调用cb。
    这样，定时器的回调只有在特定条件满足时才会执行，从而提供了额外的灵活性。
    */

    

    // 使用addTimer函数添加一个新的定时器。这里使用std::bind来创建一个新的回调函数，
    // 该回调首先检查weak_cond指向的对象是否仍然存在（即条件是否满足），
    // 然后才调用原始的回调函数cb。
    // 参数ms表示定时器超时的毫秒数。
    // 参数cb是原始的回调函数，即在条件满足时需要执行的函数。
    // 参数weak_cond是一个弱智能指针，用于监测条件对象的存活状态。
    // 参数recurring指示定时器是否应该重复设置。
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}


// 获取距离下一个定时器事件发生的时间。
// 最近一个定时器执行的时间间隔
uint64_t TimerManager::getNextTimer() {
    /*
    首先通过读锁来确保线程安全地访问定时器集合，
    然后检查定时器集合是否为空，
    如果为空则返回一个特定的最大值表示当前没有定时器需要处理。
    如果定时器集合不为空，函数会计算距离下一个定时器事件发生的时间，并返回这个时间值。

    此函数的返回值可以用于决定TimerManager需要在多长时间后检查并处理下一个定时器事件。
    特别地，返回值为0表示有定时器事件需要立即处理，而返回~0ull表示当前没有任何定时器事件需要处理。
    这个机制可以帮助实现定时器的高效调度和处理。
    */
    // 使用读锁来同步对定时器集合m_timers的访问。
    // 这确保了在多线程环境中对定时器集合的安全访问。
    RWMutexType::ReadLock lock(m_mutex);

    // 将m_tickled标志重置为false。
    // m_tickled用于指示是否有定时器事件即将发生。
    m_tickled = false;

    // 如果定时器集合为空，则返回一个特殊值(~0ull)，表示当前没有定时器需要处理。
    if(m_timers.empty()) {
        return ~0ull; // 使用~0ull作为特殊值，表示无定时器。
    }

    // 获取集合中的第一个（即最近的）定时器事件。
    const Timer::ptr& next = *m_timers.begin();

    // 获取当前的时间（以毫秒为单位）。
    uint64_t now_ms = webserver::GetCurrentMS();

    // 如果当前时间已经超过或等于下一个定时器事件的预定时间，
    // 则表示下一个定时器事件应该立即执行，因此返回0。
    if(now_ms >= next->m_next) {
        return 0;
    } else {
        // 否则，计算并返回距离下一个定时器事件发生的时间。
        return next->m_next - now_ms;
    }
}

// 定时器执行列表
// 收集所有已过期的定时器回调函数。
void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) {
    /*
    收集所有已过期的定时器回调，并根据需要重新安排循环定时器。
    
    检查定时器集合是否为空，检测系统时钟是否发生了回滚，收集所有已过期的定时器，
    将过期定时器的回调函数添加到提供的向量中，并根据需要重新安排循环定时器。
    通过这种方式，TimerManager能够高效地管理大量的定时器，确保它们按时触发并在不再需要时被清理。
    */
    // 获取当前时间（毫秒）。
    uint64_t now_ms = webserver::GetCurrentMS();
    // 用于存储所有已过期的定时器。
    std::vector<Timer::ptr> expired;

    {
        // 使用读锁来检查定时器集合是否为空。
        RWMutexType::ReadLock lock(m_mutex);
        if(m_timers.empty()) {
            // 如果定时器集合为空，则直接返回。
            return;
        }
    }

    // 使用写锁来修改定时器集合。
    RWMutexType::WriteLock lock(m_mutex);
    if(m_timers.empty()) {
        // 再次检查定时器集合是否为空，因为在获取写锁之前的状态可能已经改变。
        return;
    }

    // 检测系统时钟是否回滚。
    bool rollover = detectClockRollover(now_ms);
    // 如果没有发生时钟回滚，并且集合中的第一个定时器尚未过期，则直接返回。
    if(!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        return;
    }

    // 创建一个临时定时器对象，以当前时间为过期时间。
    Timer::ptr now_timer(new Timer(now_ms));
    // 根据是否发生时钟回滚来确定搜索的起始位置。
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    // 寻找所有在当前时间点或之前就已经过期的定时器。
    while(it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    // 将所有已过期的定时器从定时器集合中移动到`expired`向量中。
    expired.insert(expired.begin(), m_timers.begin(), it);
    // 从定时器集合中移除所有已过期的定时器。
    m_timers.erase(m_timers.begin(), it);
    // 为收集的回调预留足够的空间。
    cbs.reserve(expired.size());

    // 遍历所有已过期的定时器。
    for(auto& timer : expired) {
        // 将定时器的回调函数添加到回调向量中。
        cbs.push_back(timer->m_cb);
        // 如果定时器是循环的，则重新计算其下一个触发时间并重新添加到定时器集合中。
        if(timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            // 如果定时器不是循环的，则清除其回调函数以释放相关资源。
            timer->m_cb = nullptr;
        }
    }
}


// 这个成员函数用于向TimerManager中添加一个新的Timer对象。
// 参数val是要添加的Timer对象的智能指针，lock是一个写锁引用，保证了线程安全地修改定时器容器。
void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    /*
    专门用于将新的定时器对象插入到定时器管理器中的容器。
    此函数负责将新的定时器对象按照预定的顺序添加到TimerManager管理的容器中，
    并根据定时器插入的位置决定是否需要执行额外的操作（如调整最近的定时事件）。
    通过在适当的时候释放写锁，函数优化了线程同步的开销，保证了既高效又安全的操作。
    */
    // 将新的Timer对象插入到定时器管理器的容器m_timers中，并获取插入位置的迭代器。
    // m_timers是一个排序容器，保证了定时器的顺序。
    auto it = m_timers.insert(val).first;

    // 判断新插入的定时器是否位于容器的最前端，且m_tickled标志为假。
    // m_tickled是一个布尔标志，用于指示是否需要触发对定时器的处理。
    bool at_front = (it == m_timers.begin()) && !m_tickled;

    // 如果新的定时器是第一个元素，并且之前没有设置m_tickled标志，
    // 则设置m_tickled为true，表示定时器容器的最前端有变动，可能需要处理。
    if(at_front) {
        m_tickled = true;
    }

    // 释放之前获取的写锁。因为下面的操作可能不需要对定时器容器进行修改，
    // 所以在执行可能会阻塞的操作之前释放锁是一个好习惯。
    lock.unlock();

    // 如果新的定时器位于容器的最前端，表示最近的定时事件有变动，
    // 则调用onTimerInsertedAtFront函数。这个函数通常用于执行一些必要的操作，
    // 比如重新设置定时器，以确保最新的定时事件得到处理。
    /* 触发onTimerInsertedAtFront()
	 * onTimerInsertedAtFront()在IOManager中就是做了一次tickle()的操作 */
    if(at_front) {
        onTimerInsertedAtFront();
    }
}


// 检测系统时钟是否发生了回滚。
// 检测服务器时间是否被调后了 21:00 -> 20:50 调后了
bool TimerManager::detectClockRollover(uint64_t now_ms) {
    /*
    检测系统时钟是否发生了回滚。
    时钟回滚可能发生在某些系统中，特别是在使用相对时间计时的系统中调整系统时间时。
    如果检测到回滚，函数返回true；否则返回false。

    这个函数通过比较当前时间now_ms与上一次记录的时间m_previouseTime来判断是否发生了时钟回滚。
    如果当前时间小于上一次记录的时间，并且这两个时间的差值超过了一个预设的阈值（这里使用1小时作为阈值），则认为发生了时钟回滚。
    该阈值用于排除正常的时间误差，只有在显著的时间差异出现时才认定为时钟回滚。
    在每次检测后，函数会更新m_previouseTime为当前时间，以便进行下一次的回滚检测。
    */
    // 初始化回滚标志为false。
    bool rollover = false;

    // 检查当前时间是否小于上一次记录的时间，且当前时间与上一次记录的时间差距超过1小时（60分钟*60秒*1000毫秒）。
    // 这种情况表明可能发生了时钟回滚。
    if(now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000)) {
        // 如果满足上述条件，则设置回滚标志为true。
        rollover = true;
    }

    // 更新上一次记录的时间为当前时间，以便下一次调用时使用。
    m_previouseTime = now_ms;

    // 返回时钟回滚检测结果。
    return rollover;
}

// 检查TimerManager是否至少拥有一个定时器。
// 是否有定时器
bool TimerManager::hasTimer() {
    /*
    检查定时器管理器是否至少拥有一个定时器。
    通过检查定时器容器m_timers是否为空来实现的。
    如果m_timers不为空，即至少包含一个定时器，函数返回true；
    否则返回false。

    这个函数利用了读写锁（RWMutexType::ReadLock）来保护m_timers容器，确保在并发环境下的线程安全。
    读锁允许多个线程同时读取m_timers，但在写入时会阻塞，直到所有的读锁都被释放。
    这种锁策略是高效的，特别是在读操作远多于写操作的场景中。
    通过这种方式，TimerManager可以准确地提供关于其是否有活动定时器的信息，这对于调度和管理定时任务至关重要。
    */
    // 使用读锁来同步对定时器容器m_timers的访问。
    // 这确保了在多线程环境中对定时器容器的安全访问。
    RWMutexType::ReadLock lock(m_mutex);

    // 检查定时器容器m_timers是否为空。
    // 如果不为空（即至少有一个定时器），则返回true。
    // 如果为空（即没有任何定时器），则返回false。
    return !m_timers.empty();
}


}
