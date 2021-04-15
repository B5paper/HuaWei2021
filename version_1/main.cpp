#include "types.h"
#include "io.h"
#include "status.h"
// #include "eval.h"
using namespace std;


ServerSpecList servers;
VMSpecList vms;
DayReqList reqs;

int main()
{
    read_from_file("C:\\Users\\Administrator\\Desktop\\training-1.txt", servers, vms, reqs);
    ServerStat server_stat;
    for (int i = 0; i < 10; ++i)
    {
        server_stat.add(i, servers);
    }
    server_stat.update_stat(VMSingle(1, 0, 0, 1), vms);
    return 0;
}