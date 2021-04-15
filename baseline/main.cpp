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

    // FILE * pf = freopen("C:\\Users\\Administrator\\Desktop\\training-1.txt", "r", stdin);
    // freopen("c:\\users\\administrator\\desktop\\output_3_test.txt", "w", stdout);
    read_from_stdin(serv_specs, vm_specs, reqs);
    DayDispList disps(reqs);

    dispatch(serv_specs, vm_specs, reqs, serv_stats, disps);

    write_to_stdout(disps);
    return 0;
}