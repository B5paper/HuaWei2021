#include "types.h"
#include "io.h"
#include "status.h"
#include "dispatch.h"
#include <iostream>

using namespace std;

int main()
{
    ServSpecList serv_specs;
    VMSpecList vm_specs;
    DayReqList reqs;
    ServStatList serv_stats;
    DayDispList disps;

    // int min_core = INT32_MAX, min_mem = INT32_MAX;
    // int min_core_vm_type, min_mem_vm_type;
    // for (int i = 0; i < vm_specs.size(); ++i)
    // {
    //     if (vm_specs[i].nodes == 2)
    //         continue;

    //     if (vm_specs[i].cores < min_core)
    //     {
    //         min_core = vm_specs[i].cores;
    //         min_core_vm_type = i;
    //     }

    //     if (vm_specs[i].memcap < min_mem)
    //     {
    //         min_mem = vm_specs[i].memcap;
    //         min_mem_vm_type = i;
    //     }
    // }

    // DayDispList disps;

    freopen("C:\\Users\\administrator\\Desktop\\training-2.txt", "r", stdin);

    // freopen("c:\\users\\administrator\\desktop\\output_3_test.txt", "w", stdout);

    read_from_stdin(serv_specs, vm_specs, reqs);

    dispatch(serv_specs, vm_specs, reqs, serv_stats, disps);

    write_to_stdout(disps);

    return 0;
}