/*
 * *@*	Description: 
 * *@*	Version: 
 * *@*	Author: yusheng Gao
 * *@*	Date: 2021-08-18 21:01:42
 * *@*	LastEditors: yusheng Gao
 * *@*	LastEditTime: 2021-08-18 21:01:42
 */


#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdint>

#include "coroutine.h"



// 协程调度器创建
CScheduler* scheduler_create(){
    CScheduler* pScheduler = new CScheduler;

    pScheduler->m_valid_num = 0;
    pScheduler->m_max_num = COROUTINE_INIT_MAX_NUM;
    pScheduler->m_running_id = -1;
    pScheduler->m_ppCoRoutine = new CCoRoutine*[COROUTINE_INIT_MAX_NUM];

    memset(pScheduler->m_ppCoRoutine, 0, COROUTINE_INIT_MAX_NUM*sizeof(CCoRoutine*));

    return pScheduler;
}

// 协程调度器关闭
void scheduler_close(CScheduler* _pScheduler){
    for(int i = 0; i < _pScheduler->m_max_num; ++i){
        if(_pScheduler->m_ppCoRoutine[i]){
            coroutine_close(_pScheduler->m_ppCoRoutine[i]);
        }
    }
    delete[] _pScheduler->m_ppCoRoutine;
    _pScheduler->m_ppCoRoutine = nullptr;

    delete _pScheduler;
}

// 协程创建
int32_t coroutine_create(CScheduler* _pScheduler, pFunc _func, void* _pArgs){
    
    // 协程相关数据结构的初始化
    CCoRoutine* pCoRoutine = new CCoRoutine;
    pCoRoutine->m_pFunc = _func;
    pCoRoutine->m_pArgs = _pArgs;
    pCoRoutine->m_pScheduler = _pScheduler;

    pCoRoutine->m_alloc = 0;
    pCoRoutine->m_stack_size = 0;
    pCoRoutine->m_pStack = nullptr;
    pCoRoutine->m_code = COROUTINE_CODE::COROUTINE_READY;

    int32_t pos_in_pool = -1;

    // 协程池扩容
    if(_pScheduler->m_valid_num >= _pScheduler->m_max_num){

        int32_t cur_max_num = _pScheduler->m_max_num;
        CCoRoutine** new_space = new CCoRoutine*[cur_max_num*2];
        memset(new_space, 0, 2*cur_max_num*sizeof(CCoRoutine*));
        memcpy(new_space, _pScheduler->m_ppCoRoutine, cur_max_num*sizeof(CCoRoutine*));

        delete[] _pScheduler->m_ppCoRoutine;
        _pScheduler->m_ppCoRoutine = new_space;

        _pScheduler->m_max_num = 2*cur_max_num;
        pos_in_pool = cur_max_num;

    }else{
        
        for(int d = 0; d < _pScheduler->m_max_num; ++d){
            pos_in_pool = (d + _pScheduler->m_valid_num) % _pScheduler->m_max_num;
            if(_pScheduler->m_ppCoRoutine[pos_in_pool] == nullptr){
                break;
            }
        }
    }

    if(pos_in_pool != -1) {
        _pScheduler->m_ppCoRoutine[pos_in_pool] = pCoRoutine;
        _pScheduler->m_valid_num ++;
    }

    return pos_in_pool;

}

// 协程关闭
void coroutine_close(CCoRoutine* _pCoRoutine){
    delete[] _pCoRoutine->m_pStack;
    delete _pCoRoutine;
}

// 协程执行封装函数
void func_wrapper(CScheduler* _pScheduler){

    // 获得相应协程
    int id = _pScheduler->m_running_id;
    CCoRoutine* pCoRoutine = _pScheduler->m_ppCoRoutine[id];

    // 执行函数
    pCoRoutine->m_pFunc(_pScheduler, pCoRoutine->m_pArgs);

    // 关闭协程
    coroutine_close(pCoRoutine);

    // 更新信息
    _pScheduler->m_ppCoRoutine[id] = nullptr;
    _pScheduler->m_valid_num --;
    _pScheduler->m_running_id = -1;
}

// 协程恢复/调度/运行
int32_t coroutine_resume(CScheduler* _pScheduler, int32_t _id){

    // 当前无协程正在运行, 且当前协程id有效
    assert(_pScheduler->m_running_id == -1 && _id >= 0 && _id < _pScheduler->m_max_num);

    // 获得当前协程
    CCoRoutine* pCoRoutine = _pScheduler->m_ppCoRoutine[_id];
    
    if(pCoRoutine == nullptr) return -1;

    // 当前协程状态码
    switch(pCoRoutine->m_code){
        // 就绪
        case COROUTINE_CODE::COROUTINE_READY:

            // 获取当前上下文环境信息保存到该协程中
            ::getcontext(&pCoRoutine->m_ctx);

            // 设置切换环境
            pCoRoutine->m_ctx.uc_link = &(_pScheduler->m_ctx);

            // 设置共享栈信息
            pCoRoutine->m_ctx.uc_stack.ss_flags = 0;
            pCoRoutine->m_ctx.uc_stack.ss_sp = _pScheduler->m_shared_stack;
            pCoRoutine->m_ctx.uc_stack.ss_size = STACK_SIZE;

            // 更新信息
            _pScheduler->m_running_id = _id;
            pCoRoutine->m_code = COROUTINE_CODE::COROUTINE_RUNNING;

            // 配置协程执行环境
            // makecontext主要的工作就是设置u_context_t中保存的寄存器：函数指针(lr)、堆栈指针(sp)、函数参数(r0,r1, ...) 
            // 这也就是makecontext调用前，必须要先getcontext下的原因(保存原来的环境)。
            // uintptr_t schedulerADDR = reinterpret_cast<uintptr_t>(_pScheduler);
            ::makecontext(&pCoRoutine->m_ctx, 
                          reinterpret_cast<void(*)()> (func_wrapper), 
                          1, 
                          _pScheduler);

            // 保存当前上下文环境信息到_pScheduler->m_ctx,并切换环境为pCoRoutine->m_ctx，即将执行协程
            ::swapcontext(&_pScheduler->m_ctx, &pCoRoutine->m_ctx);

            // 协程执行完毕后，切换环境为_pScheduler->m_ctx，原执行现场恢复

        break;

        // 挂起
        case COROUTINE_CODE::COROUTINE_SUSPEND:

            // 将该协程的栈信息复制到共享栈中
            memcpy(_pScheduler->m_shared_stack+STACK_SIZE-pCoRoutine->m_stack_size, pCoRoutine->m_pStack, pCoRoutine->m_stack_size);
            
            // 更新信息
            _pScheduler->m_running_id = _id;
            pCoRoutine->m_code = COROUTINE_CODE::COROUTINE_RUNNING;

            // 保存现场，并切换环境，继续执行协程
            ::swapcontext(&_pScheduler->m_ctx, &pCoRoutine->m_ctx);

            // 协程执行完毕后，切换环境为_pScheduler->m_ctx，原执行现场恢复
        break;

        // 错误！！！   
        default:
            exit(EXIT_FAILURE);
        break;
    }

    return 0;
}

// 协程挂起
void coroutine_yield(CScheduler* _pScheduler){
    int32_t id = _pScheduler->m_running_id;

    // 当前有协程正运行
    assert(id >= 0);

    // 获得运行协程
    CCoRoutine* pCoRoutine = _pScheduler->m_ppCoRoutine[id];

    // 栈底
    char* bottom = _pScheduler->m_shared_stack + STACK_SIZE;

    // 栈顶
    char top = 0;

    // 栈未溢出
    assert(bottom - &top <= STACK_SIZE);

    // 保存共享栈信息到该协程
    if(pCoRoutine->m_alloc < bottom - &top){
        delete[] pCoRoutine->m_pStack;
        pCoRoutine->m_alloc = bottom - &top;
        pCoRoutine->m_pStack = new char[pCoRoutine->m_alloc];
    }
    pCoRoutine->m_stack_size = bottom - &top;
    memcpy(pCoRoutine->m_pStack, &top, pCoRoutine->m_stack_size);


    // 更新信息
    pCoRoutine->m_code = COROUTINE_CODE::COROUTINE_SUSPEND;
    _pScheduler->m_running_id = -1;

    // 保存现场，并切换环境到调度器[getcontext(&pCoRoutine->m_ctx); setcontext(&_pScheduler->m_ctx);]
    ::swapcontext(&pCoRoutine->m_ctx, &_pScheduler->m_ctx);

    // 调度器执行完毕，切换环境为pCoRoutine->m_ctx，原执行现场恢复
}

// 协程状态码
COROUTINE_CODE coroutine_code(CScheduler* _pScheduler, int32_t _id){

    // 协程id有效
    assert(_id >= 0 && _id < _pScheduler->m_max_num);

    if(_pScheduler->m_ppCoRoutine[_id] == nullptr) return COROUTINE_CODE::COROUTINE_DEAD;

    return _pScheduler->m_ppCoRoutine[_id]->m_code;
}

// 运行中的协程id
int32_t coroutine_running_id(CScheduler* _pScheduler){
    return _pScheduler->m_running_id;
}