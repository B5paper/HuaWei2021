#include "types.h"
#include "io.h"
#include "status.h"
#include "dispatch.h"
#include <iostream>

using namespace std;

int main()
{
    ServSpecList serv_specs;
    VMSpecList
    vm_specs;
    DayReqList reqs;
    ServStatList serv_stats;
    DayDispList disps;

    // freopen("C:\\Users\\administrator\\Desktop\\training-2.txt", "r", stdin);

    read_from_stdin(serv_specs, vm_specs, reqs);

    dispatch(serv_specs, vm_specs, reqs, serv_stats, disps);

    write_to_stdout(disps);

    return 0;
}