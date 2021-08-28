/*
 * *@*	Description: 
 * *@*	Version: 
 * *@*	Author: yusheng Gao
 * *@*	Date: 2021-08-19 13:25:46
 * *@*	LastEditors: yusheng Gao
 * *@*	LastEditTime: 2021-08-19 13:25:46
 */
#include <iostream>

#include "coroutine.h"

void co_func1(CScheduler* _pScheduler, void* _pArgs){

    int val = *reinterpret_cast<int*>(_pArgs);
    for(int i = 0; i < 5; ++i){
        std::cout << "co_id=[" << _pScheduler->m_running_id 
                  << "], i=[" << i << "], val=[" << val << "]\n";
        coroutine_yield(_pScheduler);
    }
}

void co_func2(CScheduler* _pScheduler, void* _pArgs){
    std::string val = *reinterpret_cast<std::string*>(_pArgs);
    for(int i = 0; i < 3; ++i){
        std::cout << "co_id=[" << _pScheduler->m_running_id 
                  << "], i=[" << i << "], val=[" << val << "]\n";
        coroutine_yield(_pScheduler);
    }
}


int main(){

    // 创建调度器
    CScheduler* pScheduler = scheduler_create();
    
    int val1 = 111, val2 = 222;
    std::string val3 = "STRING";

    // 创建协程
    int32_t co_1 = coroutine_create(pScheduler, co_func1, &val1);
    int32_t co_2 = coroutine_create(pScheduler, co_func1, &val2);
    int32_t co_3 = coroutine_create(pScheduler, co_func2, &val3);

    // 调度协程
    bool flag = true;
    while(flag)
    {
        flag = false;
        if(coroutine_code(pScheduler, co_1) != COROUTINE_DEAD){
            flag = true;
            coroutine_resume(pScheduler, co_1);
        }
        if(coroutine_code(pScheduler, co_2) != COROUTINE_DEAD){
            flag = true;
            coroutine_resume(pScheduler, co_2);
        }
        if(coroutine_code(pScheduler, co_3) != COROUTINE_DEAD){
            flag = true;
            coroutine_resume(pScheduler, co_3);
        }
    }

    // 关闭调度器
    scheduler_close(pScheduler);

    int v1 = 0b111;
    int v2 = 0111;
    int v3 = 0x111;
    int v4 = 111;

    // #表示输出进制
    printf("%d %#d\n", v1, v1);
    printf("%o %#o\n", v2, v2);
printf("%b", v1);
    // #大写只能作用于X，使16进制的字母全部大写。
    printf("%x %#X\n", v3, v3);

    char x[] = "和";
    printf("sizeof('和')=%ld %c\n", sizeof('和'), x[0]);
    printf("sizeof(x)=%ld\n", sizeof(x));
    return 0;
}