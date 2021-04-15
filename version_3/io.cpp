#include "io.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <unordered_set>
using namespace std;

#define BUFFERSIZE 100

// util functions
string server_id_to_name(int id)
{
    return server_names_vec[id];
}


string vm_id_to_name(int id)
{
    return vm_names_vec[id];
}


string lstrip(string &str, string &str_to_strip)
{
    return str.substr(str.find(str_to_strip) + str_to_strip.size(), str.npos);
}


string rstrip(string &str, string &str_to_strip)
{
    return str.substr(0, str.rfind(str_to_strip));
}


vector<string> split_line(string str, string delimiter, string lstrip_str, string rstrip_str)
{
    vector<string> strs;
    str = lstrip(str, lstrip_str);
    str = rstrip(str, rstrip_str);
    vector<int> idx_start, len_substr;
    int idx_temp = 0;
    idx_start.push_back(idx_temp);
    while (true)
    {
        idx_temp = str.find(delimiter, idx_temp+1);
        if (idx_temp == str.npos)
        {
            len_substr.push_back(str.npos);
            break;
        }
        len_substr.push_back(idx_temp - idx_start[idx_start.size()-1]);
        idx_start.push_back(idx_temp + delimiter.size());
    }

    for (int i = 0; i < idx_start.size(); ++i)
    {
        strs.push_back(str.substr(idx_start[i], len_substr[i]));
    }
    return strs;
}


// Member functions of ReqSeqReader
ServerSpec read_server_spec_line(string line)
{
    vector<string> strs = split_line(line, ", ", "(", ")");

    auto iter = server_names_map.find(strs[0]);
    if (iter == server_names_map.end())
    {
        server_names_map.insert(make_pair(strs[0], server_names_id_top));
        ++server_names_id_top;
        server_names_vec.push_back(strs[0]);
    }
    iter = server_names_map.find(strs[0]);

    return ServerSpec(iter->second, stoi(strs[1]), stoi(strs[2]), stoi(strs[3]), stoi(strs[4]));
    // servers.name_id.push_back(iter->second);
    // servers.cores.push_back(stoi(strs[1]));
    // servers.memcap.push_back(stoi(strs[2]));
    // servers.cost.push_back(stoi(strs[3]));
    // servers.consume.push_back(stoi(strs[4]));
}


VMSpec read_vm_spec_line(string line)
{
    vector<string> strs = split_line(line, ", ", "(", ")");
    
    auto iter = vm_names_map.find(strs[0]);
    if (iter == vm_names_map.end())
    {
        vm_names_map.insert(make_pair(strs[0], vm_names_id_top));
        ++vm_names_id_top;
        vm_names_vec.push_back(strs[0]);
    }
    iter = vm_names_map.find(strs[0]);

    return VMSpec(iter->second, stoi(strs[1]), stoi(strs[2]), stoi(strs[3])+1);

    // vms.name_id.push_back(iter->second);
    // vms.cores.push_back(stoi(strs[1]));
    // vms.memcap.push_back(stoi(strs[2]));
    // vms.nodes.push_back(stoi(strs[3]));
}


void read_vm_req_line(string line, DayReq *day_req)
{
    vector<string> strs = split_line(line, ", ", "(", ")");

    if (strs[0][0] == 'd')
    {
        day_req->op.push_back(0);
        day_req->name_id.push_back(-1);
        day_req->id.push_back(stoi(strs[1]));
    }
    else
    {
        day_req->op.push_back(1);
        day_req->name_id.push_back(vm_names_map.find(strs[1])->second);
        day_req->id.push_back(stoi(strs[2]));
    }
}


void read_from_stdin(ServerSpecList &servers, VMSpecList &vms, DayReqList &reqs)
{
    // read server info
    string line;
    getline(cin, line);
    int num_servers = stoi(line);
    ServerSpec server_spec;
    for (int i = 0; i < num_servers; ++i)
    {
        getline(cin, line);
        server_spec = read_server_spec_line(line);
        servers.name_id.push_back(server_spec.name_id);
        servers.cores.push_back(server_spec.cores);
        servers.memcap.push_back(server_spec.memcap);
        servers.cost.push_back(server_spec.cost);
        servers.consume.push_back(server_spec.consume);
    }

    // read virtual machine info
    getline(cin, line);
    int num_vms = stoi(line);
    VMSpec vm_spec;
    for (int i = 0; i < num_vms; ++i)
    {
        getline(cin, line);
        vm_spec = read_vm_spec_line(line);
        vms.name_id.push_back(vm_spec.name_id);
        vms.cores.push_back(vm_spec.cores);
        vms.memcap.push_back(vm_spec.memcap);
        vms.nodes.push_back(vm_spec.nodes);
    }

    // read requirements
    getline(cin, line);
    int num_days = stoi(line);
    DayReq *day_req = NULL;
    for (int i = 0; i < num_days; ++i)
    {
        day_req = new DayReq;
        getline(cin, line);
        int num_seqs = stoi(line);
        for (int j = 0; j < num_seqs; ++j)
        {
            getline(cin, line);
            read_vm_req_line(line, day_req);
        }
        reqs.push_back(day_req);
    }
}


void read_from_file(string path, ServerSpecList &servers, VMSpecList &vms, DayReqList &reqs)
{
    ifstream ifs;
    ifs.open(path, ios::in);

    // read server info
    string line;
    getline(ifs, line);
    int num_servers = stoi(line);
    ServerSpec server_spec;
    for (int i = 0; i < num_servers; ++i)
    {
        getline(ifs, line);
        server_spec = read_server_spec_line(line);
        servers.name_id.push_back(server_spec.name_id);
        servers.cores.push_back(server_spec.cores);
        servers.memcap.push_back(server_spec.memcap);
        servers.cost.push_back(server_spec.cost);
        servers.consume.push_back(server_spec.consume);
    }

    // read virtual machine info
    getline(ifs, line);
    int num_vms = stoi(line);
    VMSpec vm_spec;
    for (int i = 0; i < num_vms; ++i)
    {
        getline(ifs, line);
        vm_spec = read_vm_spec_line(line);
        vms.name_id.push_back(vm_spec.name_id);
        vms.cores.push_back(vm_spec.cores);
        vms.memcap.push_back(vm_spec.memcap);
        vms.nodes.push_back(vm_spec.nodes);
    }

    // read requirements
    getline(ifs, line);
    int num_days = stoi(line);
    DayReq *day_req = NULL;
    for (int i = 0; i < num_days; ++i)
    {
        day_req = new DayReq;
        getline(ifs, line);
        int num_seqs = stoi(line);
        for (int j = 0; j < num_seqs; ++j)
        {
            getline(ifs, line);
            read_vm_req_line(line, day_req);
        }
        reqs.push_back(day_req);
    }

    ifs.close();
}


// void test_output()
// {
//     server_names_vec.push_back("NV603");
//     server_names_vec.push_back("NV604");

//     vector<DayDisp*> test_data;
//     DayDisp *day_disp = NULL;

//     day_disp = new DayDisp;
//     day_disp->pur_server_name_ids.push_back(0);
//     day_disp->pur_server_name_ids.push_back(1);
//     day_disp->pur_server_nums.push_back(1);
//     day_disp->pur_server_nums.push_back(1);
//     day_disp->req_server_ids.push_back(0);
//     day_disp->req_server_ids.push_back(0);
//     day_disp->req_server_nodes.push_back(0);
//     day_disp->req_server_nodes.push_back(1);
//     test_data.push_back(day_disp);

//     day_disp = new DayDisp;
//     day_disp->req_server_ids.push_back(1);
//     day_disp->req_server_nodes.push_back(2);
//     test_data.push_back(day_disp);

//     day_disp = new DayDisp;
//     day_disp->req_server_ids.push_back(1);
//     day_disp->req_server_nodes.push_back(1);
//     test_data.push_back(day_disp);

//     write_to_stdout(test_data);

//     for (int i = 0; i < test_data.size(); ++i)
//     {
//         delete test_data[i];
//     }
// }

void disp_pur_server(int type, DayDispList &disps, int num)
{
    DayDisp *disp_back = disps.disp_list.back();
    int idx = -1;
    for (int i = 0; i < disp_back->pur_server_name_ids.size(); ++i)
    {
        if (disp_back->pur_server_name_ids[i] == type)
        {
            idx = i;
            break;
        }
    }

    if (idx != -1)
    {
        disp_back->pur_server_nums[idx] += num;
    }
    else
    {
        disp_back->pur_server_name_ids.push_back(type);
        disp_back->pur_server_nums.push_back(1);
    }
}

void disp_mig_vm(int vm_id, int server_id, DayDispList &disps)
{

}

void disp_add_vm(int server_id, int server_node, DayDispList &disps)
{

}

void write_to_stdout(DayDispList &disps)
{
    for (int i = 0; i < disps.disp_list.size(); ++i)
    {
        // purchase
        unordered_set<int> serv_type_nums;
        int serv_type_num = 0;
        for (int j = 0; j < disps.disp_list[i]->pur_server_name_ids.size(); ++j)
        {
            if (serv_type_nums.find(disps.disp_list[i]->pur_server_name_ids[j]) != serv_type_nums.end())
            {
                ++serv_type_num;
            }
            else
            {
                serv_type_nums.insert(disps.disp_list[i]->pur_server_name_ids[j]);
                ++serv_type_num;
            }
        }
        // cout << "(purchase, " << serv_nums << ")" << endl;
        cout << "(purchase, " << serv_type_num << ")" << endl;
        for (int j = 0; j < disps.disp_list[i]->pur_server_name_ids.size(); ++j)
            cout << "(" << server_id_to_name(disps.disp_list[i]->pur_server_name_ids[j]) << ", " << disps.disp_list[i]->pur_server_nums[j] << ")" << endl;
            // cout << "(" << server_id_to_name(disps.disp_list[i]->pur_server_name_ids[j]) << ", " << disps.disp_list[i]->pur_server_nums[j] << ")" << endl;        

        // migrate
        // cout << "(migration, 0)" << endl;
        cout << "(migration, 0)" << endl;
        // cout << "(migration, " << disps.disp_list[i]->mig_vm_ids.size() << ")" << endl;
        // for (int j = 0; j < disps.disp_list[i]->mig_vm_ids.size(); ++j)
        // {
        //     cout << "(" << disps.disp_list[i]->mig_vm_ids[j] << ", " << disps.disp_list[i]->mig_server_ids[j];
        //     if (disps.disp_list[i]->mig_server_nodes[j] == 2)
        //         cout << ")" << endl;
        //     else
        //         cout << ", " << (disps.disp_list[i]->mig_server_nodes[j] == 0 ? "A)" : "B)") << endl;
        // }

        // process requirements
        for (int j = 0; j < disps.disp_list[i]->req_server_ids.size(); ++j)
        {
            cout << "(" << disps.disp_list[i]->req_server_ids[j];
            if (disps.disp_list[i]->req_server_nodes[j] == 2)
                cout << ")" << endl;
            else
                cout << ", " << (disps.disp_list[i]->req_server_nodes[j] == 0 ? "A)" : "B)") << endl;
        }

        
    }
}


void DayDispList::clear()
{
    for (int i = 0; i < disp_list.size(); ++i)
    {
        delete disp_list[i];
    }
    disp_list.clear();
}


DayDispList::~DayDispList()
{
    clear();
}

// int main()
// {
//     // test 1: test output function
//     // test_output();

//     // test 2: read from stdin
//     // ServerSpecList servers;
//     // VMSpecList vms;
//     // DayReqList reqs;
//     // read_from_stdin(servers, vms, reqs);

//     // test 3: read from file
//     ServerSpecList servers;
//     VMSpecList vms;
//     DayReqList reqs;
//     unsigned int tic = time(NULL);
//     read_from_file("C:\\Users\\Administrator\\Desktop\\training-1.txt", servers, vms, reqs);
//     unsigned int toc = time(NULL);
//     cout << toc - tic << endl;
//     cout << endl;

//     for (auto iter = server_names_map.begin(); iter != server_names_map.end(); ++iter)
//     {
//         cout << iter->first << " " << iter->second << endl;
//     }
//     for (int i = 0; i < server_names_vec.size(); ++i)
//     {
//         cout << i << ": " << server_names_vec[i] << endl;
//     }

//     return 0;
// }