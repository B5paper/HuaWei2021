#ifndef BFD
#define BFD

#include "types.h"
#include "io.h"
#include "status.h"
#include <algorithm>
#include "dispatch.h"
using namespace std;


struct Residual
{
	int vm_type;
	int server_id;
	int server_node;
	float res;

	Residual(int vm_type, int server_id, int server_node, float res)
	: vm_type(vm_type), server_id(server_id), server_node(server_node), res(res) {} 
};

struct ServIdNode
{
	int serv_id;
	int serv_node;

	ServIdNode(int id, int node): serv_id(id), serv_node(node) {}
};

ServIdNode vm_select_server(int vm_id, int vm_type, ServerStatList &server_stats, ServerSpecList &server_specs, VMSpecList &vm_specs);


bool bfd_2(AssignScheme &assign_scheme, vector<int> &vm_ids, vector<int> &vm_types, ServerStatList &server_stat, ServerSpecList &server_specs, VMSpecList &vm_specs);

// vector<int> purchase_server(vector<int> &vm_types, vector<int> &server_types, ServerSpecList &server_specs, VMSpecList &vm_specs);

template<typename T> std::vector<int> argsort(const std::vector<T>& array)
{
	const int array_len(array.size());
	std::vector<int> array_index(array_len, 0);
	for (int i = 0; i < array_len; ++i)
		array_index[i] = i;

	std::sort(array_index.begin(), array_index.end(),
		[&array](int pos1, int pos2) {return (array[pos1] < array[pos2]);});

	return array_index;
}
#endif