#include "types.h"
#include "io.h"
#include "status.h"
#include "dispatch.h"
#include <iostream>
#include "matplot.h"
#include "linopt.h"
#include "args.h"

using namespace std;

int main()
{
    // set<pair<int, int>> myset;
    // myset.emplace(5, 3);
    // myset.emplace(2, 4);
    // myset.emplace(9, 18);
    // auto iter = myset.find(make_pair(9, 18));
    // cout << iter->first << ", " << iter->second << endl;
    // iter = myset.find(make_pair(2, 5));
    // cout << iter->first << ", " << iter->second << endl;
    // cout << (iter == myset.end()) << endl;


    // test simplex
    // result: x = (8, 4, 0), result: max_val = 28
    // vector<float> c({3, 1, 2});
    // vector<vector<float>> A({{1, 1, 3}, {2, 2, 5}, {4, 1, 2}});
    // vector<float> b({30, 24, 36});
    // LP lp(A, b, c);
    // linpro(lp);

    // test initialize_simplex
    // result: x = (1.555, 1.111), max_val = 2
    // vector<float> c({2, -1});
    // vector<vector<float>> A({{2, -1}, {1, -5}});
    // vector<float> b({2, -4});
    // LP lp(A, b, c);
    // linpro(lp);
    
    // test branch_and_cut
    // result: x = (0, 5), result: max_val = 40
    // vector<float> c({5, 8});
    // vector<vector<float>> A({{1, 1}, {5, 9}});
    // vector<float> b({6, 45});
    // float max_val = 0;
    // vector<int> x = branch_and_cut_max(A, b, c, max_val, 100);

    // test branch_and_cut
    // result: x = (2, 4), result: max_val = 10
    // vector<float> c({1, 2});
    // vector<vector<float>> A({{3, 1}, {-5, -6}, {-1, 0}});
    // vector<float> b({10, -30, -1});
    // float min_val = 0;
    // vector<int> x = branch_and_cut_min(A, b, c, min_val, 10);
    
    // ServSpecList serv_specs;
    VMSpecList vm_specs;
    DayReqList reqs;
    DayDispList disps;

    
    // if (DEBUG)
    // {
    //     FILE *f = freopen("D:\\Documents\\Projects\\2021HWAutoGrader\\data_2\\training-1.txt", "r", stdin);
    //     read_from_stdin(serv_specs, vm_specs, reqs);
    //     fclose(f);
    // }
    // else
    //     read_from_stdin(serv_specs, vm_specs, reqs);
    
    // dispatch(serv_specs, vm_specs, reqs, serv_stats, disps);
    // write_to_stdout(disps);

    FILE *f = NULL;
    if (DEBUG)
    {
        f = freopen("D:\\Documents\\Projects\\2021HWAutoGrader\\data_2\\training-2.txt", "r", stdin);
    }
    
    int num_days, k;
    read_serv_vm_specs(serv_specs, vm_specs);
    read_num_days_k(num_days, k);
    for (int i = 0; i < k; ++i)
        read_one_day_reqs(reqs);
    // 处理前 num_day - k 天数据
    for (int i = 0; i < num_days - k; ++i)
    {
        read_one_day_reqs(reqs);
        dispatch_span(i, num_days, k, serv_specs, vm_specs, reqs, serv_stats, disps);
        if (!DEBUG)
            disps.output_day(i, i+1);  // 输出第 1 天数据
    }

    // 最后 k 天不再读入，直接处理并输出
    for (int idx_day = num_days - k; idx_day < num_days; ++idx_day)
    {
        dispatch_span(idx_day, num_days, k, serv_specs, vm_specs, reqs, serv_stats, disps);
    }

    if (!DEBUG)
    {
        disps.output_day(num_days - k, num_days);
    }

    if (DEBUG)
    {
        fclose(f);
    }

    return 0;
}