#include "types.h"
#include "io.h"
#include "status.h"
#include "dispatch.h"
#include "args.h"
#include <iostream>

using namespace std;

int main()
{
    srand(123);

    DayReqList reqs;
    DayDispList disps;

    FILE *f = NULL;
    if (DEBUG)
    {
        f = freopen("D:/task_2/training-1.txt", "r", stdin);
    }
    
    int num_days, k;
    
    read_serv_vm_specs(serv_specs, vm_specs);
    read_num_days_k(num_days, k);
    
    for (int i = 0; i < k; ++i)
        read_one_day_reqs(reqs);

    dispatch_span(0, num_days, k, reqs, disps);
    if (!DEBUG)
        disps.output_day(0, 1);  // 输出第 1 天数据

    // 处理前 num_day - k 天数据
    for (int i = 1; i < num_days - k + 1; ++i)
    {
        read_one_day_reqs(reqs);
        dispatch_span(i, num_days, k, reqs, disps);
        if (!DEBUG)
            disps.output_day(i, i+1);  // 输出第 1 天数据
    }

    // 最后 k 天不再读入，直接处理并输出
    for (int i = num_days - k + 1; i < num_days; ++i)
    {
        dispatch_span(i, num_days, k, reqs, disps);
        if (!DEBUG)
            disps.output_day(i, i + 1);
    }


    if (DEBUG)
    {
        fclose(f);
    }

    return 0;
}