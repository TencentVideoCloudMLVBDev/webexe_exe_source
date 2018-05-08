//
//  TXCTaskQueue.h
//
//  Created by petercliu on 2017/7/16.
//  Copyright © 2017年 tencent. All rights reserved.
//

#ifndef __TXCTaskQueue_H__
#define __TXCTaskQueue_H__

#include <thread>
#include <functional>
#include <queue>
#include <memory>
#include <future>
#include <chrono>

class TXCTaskQueue
{
public:
    TXCTaskQueue();

	void stop();
    
    template<class F, class... Args>
    auto PostTask(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>;
    
    template<class F, class... Args>
    auto DelayPostTask(std::chrono::milliseconds timeout, F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>;
    
    bool BelongsToCurrentThread() const;

	bool IsTaskExcuting();
    
private:
	~TXCTaskQueue();

    void Loop();
    
    struct DelayTask
    {
        DelayTask(std::chrono::steady_clock::time_point ftp, std::function<void()> t){
            fire_time = ftp;
            task = t;
        }
        
        DelayTask(const DelayTask& t) = default;
        DelayTask(DelayTask&& t) = default;
        DelayTask& operator = (DelayTask &&t) = default;
        DelayTask& operator = (const DelayTask &t) = default;
        
        std::chrono::steady_clock::time_point fire_time;
        std::function<void()> task;
    };
    
    struct DelayTaskCmp{
        bool operator() ( DelayTask a, DelayTask b ){
            return a.fire_time> b.fire_time; }
    };
    
    std::unique_ptr<std::thread> _worker_thread;
    std::queue< std::function<void()> > _tasks;
    std::priority_queue< DelayTask, std::vector<DelayTask>, DelayTaskCmp > _delay_tasks;
    std::mutex _queue_mutex;
	std::mutex _task_mutex;
    std::condition_variable _condition;
    bool _stop;
};


template<class F, class... Args>
auto TXCTaskQueue::PostTask(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
	using return_type = typename std::result_of<F(Args...)>::type;

	if (_stop)
		return std::future<return_type>();

	auto task = std::make_shared< std::packaged_task<return_type()> >(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

	std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(_queue_mutex);
		_tasks.emplace([task]() { (*task)(); });
	}
	_condition.notify_one();
	return res;
}


template<class F, class... Args>
auto TXCTaskQueue::DelayPostTask(std::chrono::milliseconds timeout, F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
    
    using return_type = typename std::result_of<F(Args...)>::type;
    
    if(_stop)
        return std::future<return_type>();
    
    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                                                                      );
    std::chrono::steady_clock::time_point fire_tp = std::chrono::steady_clock::now() + timeout;
    
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(_queue_mutex);
        _delay_tasks.emplace(fire_tp, [task](){ (*task)(); });
    }
    _condition.notify_one();
    return res;
}

#endif /* __TXCTaskQueue_H__ */
