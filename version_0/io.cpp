#include "io.h"
#include <iostream>
#include <fstream>
#include <ctime>
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


// Member functions of StringProcesser
string StringProcesser::lstrip(string &str, string &str_to_strip)
{
    return str.substr(str.find(str_to_strip) + str_to_strip.size(), str.npos);
}


string StringProcesser::rstrip(string &str, string &str_to_strip)
{
    return str.substr(0, str.rfind(str_to_strip));
}


vector<string> StringProcesser::split_line(string str, string delimiter, string lstrip_str, string rstrip_str)
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
ReqSeqReader::~ReqSeqReader()
{
    for (int i = 0; i < reqs.size(); ++i)
    {
        delete reqs[i];
    }
}


void ReqSeqReader::read_server_spec_line(string line)
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

    servers.name_id.push_back(iter->second);
    servers.cores.push_back(stoi(strs[1]));
    servers.memcap.push_back(stoi(strs[2]));
    servers.cost.push_back(stoi(strs[3]));
    servers.consume.push_back(stoi(strs[4]));
}


void ReqSeqReader::read_vm_spec_line(string line)
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

    vms.name_id.push_back(iter->second);
    vms.cores.push_back(stoi(strs[1]));
    vms.memcap.push_back(stoi(strs[2]));
    vms.nodes.push_back(stoi(strs[3]));
}


void ReqSeqReader::read_vm_req_line(string line, DayReq *day_req)
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


void ReqSeqReader::read_from_stdin()
{
    // read server info
    string line;
    getline(cin, line);
    int num_servers = stoi(line);
    for (int i = 0; i < num_servers; ++i)
    {
        getline(cin, line);
        read_server_spec_line(line);
    }

    // read virtual machine info
    getline(cin, line);
    int num_vms = stoi(line);
    for (int i = 0; i < num_vms; ++i)
    {
        getline(cin, line);
        read_vm_spec_line(line);
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


void ReqSeqReader::read_from_file(string path)
{
    ifstream ifs;
    ifs.open(path, ios::in);

    // read server info
    string line;
    getline(ifs, line);
    int num_servers = stoi(line);
    for (int i = 0; i < num_servers; ++i)
    {
        getline(ifs, line);
        read_server_spec_line(line);
    }

    // read virtual machine info
    getline(ifs, line);
    int num_vms = stoi(line);
    for (int i = 0; i < num_vms; ++i)
    {
        getline(ifs, line);
        read_vm_spec_line(line);
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


DispSeqWriter::DispSeqWriter(bool is_test)
: is_test(true)
{
    server_names_vec.push_back("NV603");
    server_names_vec.push_back("NV604");

    DayDisp *day_disp = NULL;

    day_disp = new DayDisp;
    day_disp->pur_server_name_ids.push_back(0);
    day_disp->pur_server_name_ids.push_back(1);
    day_disp->pur_server_nums.push_back(1);
    day_disp->pur_server_nums.push_back(1);
    day_disp->req_server_ids.push_back(0);
    day_disp->req_server_ids.push_back(0);
    day_disp->req_server_nodes.push_back(0);
    day_disp->req_server_nodes.push_back(1);
    test_data.push_back(day_disp);

    day_disp = new DayDisp;
    day_disp->req_server_ids.push_back(1);
    day_disp->req_server_nodes.push_back(2);
    test_data.push_back(day_disp);

    day_disp = new DayDisp;
    day_disp->req_server_ids.push_back(1);
    day_disp->req_server_nodes.push_back(1);
    test_data.push_back(day_disp);
}


DispSeqWriter::~DispSeqWriter()
{
    if (is_test)
    {
        for (int i = 0; i < test_data.size(); ++i)
        {
            delete test_data[i];
        }
    }
}


void DispSeqWriter::write_to_stdout(vector<DayDisp*> disps)
{
    for (int i = 0; i < disps.size(); ++i)
    {
        // purchase
        cout << "(purchase, " << disps[i]->pur_server_name_ids.size() << ")" << endl;
        for (int j = 0; j < disps[i]->pur_server_name_ids.size(); ++j)
            cout << "(" << server_id_to_name(disps[i]->pur_server_name_ids[j]) << ", " << disps[i]->pur_server_nums[j] << ")" << endl;

        // migrate
        cout << "(migration, " << disps[i]->mig_vm_ids.size() << ")" << endl;
        for (int j = 0; j < disps[i]->mig_vm_ids.size(); ++j)
        {
            cout << "(" << disps[i]->mig_vm_ids[j] << ", " << disps[i]->mig_server_ids[j];
            if (disps[i]->mig_server_nodes[j] == 2)
                cout << ")" << endl;
            else
                cout << ", " << (disps[i]->mig_server_nodes[j] == 0 ? "A)" : "B)") << endl;
        }

        // process requirements
        for (int j = 0; j < disps[i]->req_server_ids.size(); ++j)
        {
            cout << "(" << disps[i]->req_server_ids[j];
            if (disps[i]->req_server_nodes[j] == 2)
                cout << ")" << endl;
            else
                cout << ", " << (disps[i]->req_server_nodes[j] == 0 ? "A)" : "B)") << endl;
        }
    }
}


int main()
{
    // unsigned int tic = time(NULL);
    // ReqSeqReader req_reader;
    // req_reader.read_from_file("C:\\Users\\wsdlh\\Desktop\\training-1.txt");
    // req_reader.read_from_stdin();
    // unsigned int toc = time(NULL);
    // cout << toc - tic << endl;
    // cout << endl;

    for (auto iter = server_names_map.begin(); iter != server_names_map.end(); ++iter)
    {
        cout << iter->first << " " << iter->second << endl;
    }
    for (int i = 0; i < server_names_vec.size(); ++i)
    {
        cout << i << ": " << server_names_vec[i] << endl;
    }

    DispSeqWriter disp_writer(true);
    disp_writer.test_output();
    return 0;
}