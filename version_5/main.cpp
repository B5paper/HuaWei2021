#include "types.h"
#include "io.h"
#include "status.h"
#include "eval.h"
#include "dispatch.h"
#include <iostream>
#include <ctime>
using namespace std;

// bool check(ServerStatList &server_stat, ServerSpecList &server_specs)
// {
//     for (int i = 0; i < server_stat.ids.size(); ++i)
//     {
//         if (server_stat.cores_used[i] > server_specs.memcap[server_stat.types[i]])
//             return false;
//         if (server_stat.mem_used[i] > server_specs.memcap[server_stat.types[i]])
//             return false;
//         for (int j = 0; j < 2; ++j)
//         {
//             if (server_stat.nodes[j][i]->cores_used > server_specs.memcap[server_stat.types[i]] / 2)
//                 return false;
//             if (server_stat.nodes[j][i]->mem_used > server_specs.memcap[server_stat.types[i]] / 2)
//                 return false;
//         }
//     }
//     return true;
// }

bool check_disp(DayDispList &disps, DayReqList &reqs)
{
    int add_count;
    for (int i = 0; i < reqs.size(); ++i)
    {
        add_count = 0;
        for (int j = 0; j < reqs[i]->op.size(); ++j)
        {
            if (reqs[i]->op[j] == 1)
                ++add_count;
        }
        if (disps.add_nums[i] != add_count)
            cout << "error" << endl;
    }

    return true;
}



int main()
{
    srand(1);
    
    ServerSpecList server_specs;
    VMSpecList vm_specs;
    DayReqList reqs;
    ServerStatList server_stat;
    VMStat vm_stat;
    
    // FILE * pf = freopen("d:\\training-1.txt", "r", stdin);
    // freopen("c:\\users\\administrator\\desktop\\output_3_test.txt", "w", stdout);

    read_from_stdin(server_specs, vm_specs, reqs);
    DayDispList disps(reqs);

    dispatch(server_specs, vm_specs, reqs, server_stat, vm_stat, disps);
    // check(server_stat, server_specs);
    // check_disp(disps, reqs);
    // int serv_type = server_stat.servs[49].type;
    // cout << server_names_vec[serv_type] << endl;

    write_to_stdout(disps);

    return 0;
}