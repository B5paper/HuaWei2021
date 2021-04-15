#include "types_2.h"
#include "io_2.h"
#include "status_2.h"
#include "dispatch_2.h"
#include <iostream>

using namespace std;

int main()
{
    ServSpecList serv_specs;
    VMSpecList vm_specs;
    DayReqList reqs;
    ServStatList serv_stats;
    // DayDispList disps;


    // freopen("c:\\users\\wsdlh\\desktop\\training-1.txt", "r", stdin);

    // freopen("c:\\users\\administrator\\desktop\\output_3_test.txt", "w", stdout);

    read_from_stdin(serv_specs, vm_specs, reqs);
    DayDispList disps(reqs);

    dispatch_2(serv_specs, vm_specs, reqs, serv_stats, disps);

    write_to_stdout(disps);

    return 0;
}