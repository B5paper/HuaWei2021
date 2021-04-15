#include "types.h"
#include "io.h"
#include "status.h"
#include "eval.h"
#include "dispatch.h"
#include <iostream>
#include <ctime>
using namespace std;

int main()
{
    ServerSpecList server_specs;
    VMSpecList vm_specs;
    DayReqList reqs;
    ServerStat server_stat;
    VMStat vm_stat;
    ServerStatRecorder rec;
    DayDispList disps;

    read_from_file("C:\\Users\\Administrator\\Desktop\\training-1.txt", server_specs, vm_specs, reqs);
    // read_from_stdin(server_specs, vm_specs, reqs);

    int rtv = 0;
    for (int days_processed = 0; days_processed < reqs.size(); )
    {
        disps.clear();
        rtv = dispatch(server_specs, vm_specs, reqs, server_stat, vm_stat, rec, disps);
        days_processed += rtv;
        write_to_stdout(disps);
    }

    return 0;
}