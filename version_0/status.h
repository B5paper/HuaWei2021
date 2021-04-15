#ifndef STATUS
#define STATUS

#include <vector>
using namespace std;

struct NodeStat
{
    int cores_used;
    int cores_ream;
    int mem_used;
    int mem_ream;
};


struct ServerStat
{
    vector<int> types;
    vector<int> ids;
    vector<int> cores_used;
    vector<int> cores_ream;
    vector<int> mem_used;
    vector<int> mem_ream;
    vector<NodeStat*> node_a;
    vector<NodeStat*> node_b;
};

struct VMStat
{
    vector<int> types;
    vector<int> vm_ids;
    vector<int> server_ids;
    vector<int> nodes;  // 0 for node A, 1 for node B, 2 for both
};


#endif