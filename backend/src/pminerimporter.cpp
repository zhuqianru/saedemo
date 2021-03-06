#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <cstdio>
#include "io/mgraph.hpp"
#include "pminer.hpp"

using namespace std;

#define GROUP_BASE 0
#define PATENT_BASE 200000LL
#define COMPANY_BASE 45000000LL
#define INVENTOR_BASE 500000000LL

vector<string> split(string s, char c) {
    int last = 0;
    vector<string> v;
    for (int i=0; i<s.size(); i++) {
        if (s[i] == c) {
            v.push_back(s.substr(last, i - last));
            last = i + 1;
        }
    }
    v.push_back(s.substr(last, s.size() - last));
    return v;
}

int convert_to_int(string s) {
    istringstream is;
    is.str(s);
    int i;
    is >> i;
    return i;
}

int main() {
    sae::io::GraphBuilder<uint64_t> builder;
    builder.AddVertexDataType("Patent");
    builder.AddVertexDataType("Inventor");
    builder.AddVertexDataType("Group");
    builder.AddVertexDataType("Company");
    builder.AddEdgeDataType("PatentInventor");
    builder.AddEdgeDataType("PatentGroup");
    builder.AddEdgeDataType("PatentCompany");
    builder.AddEdgeDataType("CompanyGroup");
    builder.AddEdgeDataType("GroupInfluence");

    cerr << "Loading company_ori.txt..." << endl;
    map<int, int> com2group;
    map<int, string> group_logo;
    ifstream company_file("company_ori.txt");
    string company_input;
    while (getline(company_file, company_input)) {
        auto inputs = split(company_input, '\t');
        if (inputs.size() < 7) continue;
        int id = convert_to_int(inputs[0]);
        string name = inputs[1];
        int patCount = convert_to_int(inputs[2]);
        string logo = inputs[3];
        string homepage = inputs[4];
        string terms = inputs[5];
        int gcid = convert_to_int(inputs[6]);

        Company company{id, name, patCount, logo, homepage, terms, gcid};
        com2group[id] = gcid;
        if (logo.size() > 0 && group_logo[gcid].size() == 0) {
            group_logo[gcid] = logo;
        }
        builder.AddVertex(COMPANY_BASE + id, company, "Company");
    }

    cerr << "Loading groupcom.txt..." << endl;
    ifstream group_file("groupcom.txt");
    string group_input;
    while (getline(group_file, group_input)) {
        vector<string> inputs = split(group_input, '\t');
        int id = convert_to_int(inputs[0]);
        string name = inputs[1];
        int patentCount = convert_to_int(inputs[2]);

        Group group{id, name, patentCount, group_logo[id]};
        builder.AddVertex(GROUP_BASE + id, group, "Group");
    }
    group_file.close();

    cerr << "Loading patent.txt ..." << endl;
    map<string, vector<int>> inventor_map;
    ifstream patent_file("patent.txt");
    string patent_input;
    while (getline(patent_file, patent_input)) {
        vector<string> inputs = split(patent_input, '\t');
        // ignore empty inventors
        if (inputs[3].size() == 0) continue;
        int id = convert_to_int(inputs[0]);
        int pn = stoi(inputs[1]);
        string title = inputs[2];
        auto inventors = split(inputs[3].substr(1, inputs[2].size() - 1), '#');
        int year = stoi(inputs[4].substr(0, 4));

        for (auto it = inventors.begin(); it!=inventors.end(); it++) {
            inventor_map[*it].push_back(id);
        }

        Patent patent{id, pn, title, year, inventors};
        builder.AddVertex(PATENT_BASE + id, patent, "Patent");
    }
    patent_file.close();

    cerr << "Creating Patent-Inventor ... " << endl;
    int inventor_id = 0;
    for (auto it = inventor_map.begin(); it!=inventor_map.end(); it++) {
        Inventor inventor{(*it).first};
        builder.AddVertex(INVENTOR_BASE + inventor_id, inventor, "Inventor");

        for (auto vit = (*it).second.begin(); vit != (*it).second.end(); vit++) {
            builder.AddEdge(INVENTOR_BASE + inventor_id, PATENT_BASE + (*vit), PatentInventor(), "PatentInventor");
        }
        inventor_id ++;
    }

    cerr << "Loading pat2com.txt ..." << endl;
    FILE* pat2com_file = fopen("pat2com.txt", "r");
    int pat, com;
    while (fscanf(pat2com_file, "%d\t%d", &pat, &com) != EOF) {
        int group = com2group[com];
        builder.AddEdge(PATENT_BASE + pat, GROUP_BASE + group, PatentGroup(), "PatentGroup");
        builder.AddEdge(PATENT_BASE + pat, COMPANY_BASE + com, PatentCompany(), "PatentCompany");
    }
    fclose(pat2com_file);

    cerr << "Creating Company-Group ..." << endl;
    for (auto it = com2group.begin(); it != com2group.end(); it++) {
        int com = (*it).first;
        int group = (*it).second;
        builder.AddEdge(COMPANY_BASE + com, GROUP_BASE + group, CompanyGroup(), "CompanyGroup");
    }

    cerr << "Loading group influence ..." << endl;
    {
        ifstream influence("C2CTInfluence.txt.filtered");
        GroupInfluence gi;
        int source, target;
        int count;
        while (influence >> source >> target >> gi.topic >> gi.score) {
            if (group_logo.find(source) == group_logo.end() || group_logo.find(target) == group_logo.end()) {
                cerr << "group not found: " << source << ", or, " << target << endl;
            } else {
                builder.AddEdge(GROUP_BASE + source, GROUP_BASE + target, gi, "GroupInfluence");
                count ++;
            }
        }
        cerr << "group influence loaded: " << count << endl;
    }

    cerr << "Saving graph pminer..." << endl;
    builder.Save("pminer");

    return 0;
}
