#include "types.h"
#include "io.h"
#include "status.h"
#include "eval.h"
#include "dispatch.h"
#include <iostream>
#include <ctime>
using namespace std;

void test()
{
    static int count = 0;
    count++;
    cout << count << endl;
}


int main()
{
    ServerSpecList server_specs;
    VMSpecList vm_specs;
    DayReqList reqs;
    ServerStat server_stat;
    VMStat vm_stat;
    ServerStatRecorder rec;
    DayDispList disps;

    ulong tic = time(NULL);
    read_from_file("C:\\Users\\Administrator\\Desktop\\training-1.txt", server_specs, vm_specs, reqs);

    int rtv = 0;
    for (int days_processed = 0; days_processed < reqs.size(); )
    {
        disps.clear();
        rtv = dispatch(server_specs, vm_specs, reqs, server_stat, vm_stat, rec, disps);
        if (rtv == -1)
        {
            cout << "error occured." << endl;
            return -1;
        }
        days_processed += rtv;
        write_to_stdout(disps);
    }
    ulong toc = time(NULL);

    cout << "time: " << toc - tic << endl;

    ulong cost = get_cost(rec, server_specs);
    cout << "the total cost is: " << cost << endl;

    return 0;
}