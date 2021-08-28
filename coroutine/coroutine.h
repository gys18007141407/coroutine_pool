/*
 * *@*	Description: 
 * *@*	Version: 
 * *@*	Author: yusheng Gao
 * *@*	Date: 2021-08-18 20:30:31
 * *@*	LastEditors: yusheng Gao
 * *@*	LastEditTime: 2021-08-18 20:30:31
 */

#ifndef COROUTINE_H
#define COROUTINE_H


#include <ucontext.h>   // 保存上下文
#include <cstddef>
#include <memory>

// 协程栈大小
#define STACK_SIZE (1024*1024)

// 协程池中最大协程数
#define COROUTINE_INIT_MAX_NUM (16)

// 协程状态码
enum COROUTINE_CODE{
    COROUTINE_READY,
    COROUTINE_RUNNING,
    COROUTINE_SUSPEND,
    COROUTINE_DEAD
};

// 协程池调度器前置声明
struct CScheduler;

// 协程函数指针
typedef void (*pFunc) (CScheduler*, void*);

// 协程的有关数据结构
typedef struct CCoRoutine{

    // 当前协程上下文
    ucontext_t m_ctx;

    // 当前协程函数指针
    pFunc m_pFunc;

    // 当前协程函数参数
    void* m_pArgs;

    // 当前协程所属协程池的调度器
    CScheduler* m_pScheduler;

    // 当前协程的状态码
    COROUTINE_CODE m_code;

    // 当前协程的执行栈
    char* m_pStack;

    // 当前协程已经分配的内存大小
    int32_t m_alloc;

    // 当前协程的执行栈大小
    int32_t m_stack_size;

}CCoRoutine;

// 协程池调度器
typedef struct CScheduler{

    // 协程共享栈
    char m_shared_stack[STACK_SIZE];

    // 主协程上下文
    ucontext_t m_ctx;

    // 有效的协程个数
    int32_t m_valid_num;

    // 可管理的最大协程数量
    int32_t m_max_num;

    // 当前正在运行的协程id（协程池中的index）
    int32_t m_running_id;

    // 协程池
    CCoRoutine** m_ppCoRoutine;

}CScheduler;



// 协程调度器创建
CScheduler* scheduler_create();

// 协程调度器关闭
void scheduler_close(CScheduler*);

// 协程创建
int32_t coroutine_create(CScheduler*, pFunc, void*);

// 协程关闭
void coroutine_close(CCoRoutine*);

// 协程恢复/调度/运行
int32_t coroutine_resume(CScheduler*, int32_t);

// 协程挂起
void coroutine_yield(CScheduler*);

// 协程状态码
COROUTINE_CODE coroutine_code(CScheduler*, int32_t);

// 运行中的协程id
int32_t coroutine_running_id(CScheduler*);


#endif