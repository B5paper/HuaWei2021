#include "types.h"
#include "io.h"
#include "status.h"
#include "dispatch.h"
#include <iostream>
#include "matplot.h"
#include "linopt.h"

using namespace std;

int main()
{
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
    
    ServSpecList serv_specs;
    VMSpecList vm_specs;
    DayReqList reqs;
    ServStatList serv_stats;
    DayDispList disps;

    // FILE *f = freopen("C:\\Users\\Administrator\\Desktop\\training-2.txt", "r", stdin);
    read_from_stdin(serv_specs, vm_specs, reqs);
    // fclose(f);
    
    dispatch(serv_specs, vm_specs, reqs, serv_stats, disps);
    write_to_stdout(disps);
    
    // read_serv_vm_specs(serv_specs, vm_specs);
    // while (read_span_reqs(serv_specs, vm_specs, reqs) != -1)
    // {
    //     dispatch_span(serv_specs, vm_specs, reqs, serv_stats, disps);
    //     write_to_stdout(disps);
    // }

    return 0;
}