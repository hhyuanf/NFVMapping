//
//  main.cpp
//  mapping
//
//  Created by Jingyuan Fan on 9/30/14.
//  Copyright (c) 2014 hhyaunf. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <set>
#include <stack>
#include<iterator>
#include <unistd.h>

using namespace std;

// Parameters
const int NumberOfRequest = 300;
const int MaxVirtualNode = 3; // the actual maximum virtual nodes needs to plus 2, see the traffic generator function
const int ComputeDemand = 30;
const int StorageDemand = 30;
const int MemoryDemand = 30;
const int BandwidthDemand = 3; // 0->10Gbps, 1->40Gbps, 2->100Gbps, 3->200Gbps, 4->400Gbps, 5->1Tbps

const int ComputeCapacity = 4000;
const int StorageCapacity = 4000;
const int MemoryCapacity = 4000;
const int SpectrumCapacity = 1280;

// Fixed setting
const int INF = 1000000;
const int NumberOfTrial = 1;
const int NumberOfPhyNode = 14; //NSFNET
const int NumberOfPhyLink = 21; //NSFNET
//const int NumberOfPhyNode = 24; //USmesh
//const int NumberOfPhyLink = 43; // USmesh
const double Slot = 12.5;
const int K_PATH = 1;
const double BPSK = 1.6;
const double QPSK = 3.2;
const double QAM = 6.4;
const int Function = 8;
const double Threshold = 0.95;

const int Low = 90;
const int High = 95;

double Seed = 1111.0, Pre_Seed=1111.0;						//Seed for random number generation

ofstream output, fbdata;

struct PhysicalNode
{
	double compute;
	double storage;
	double memory;
    vector<int> function;
	double availability;
};
PhysicalNode PNode[NumberOfPhyNode];


struct PhysicalLink
{
	int distance;
	int spectrum[SpectrumCapacity];
    
	//double availability;
};
PhysicalLink PLink[NumberOfPhyNode][NumberOfPhyNode];

struct VirtualNode
{
	int compute;
	int storage;
	int memory;
	int map;
	int bmap;
    int function;
    vector<int> backup;
};

struct BackupNode
{
    int compute;
    int storage;
    int memory;
    int map;
    int bmap;
    int function[2];
};

struct VirtualLink
{
	int source;
	int destination;
	int bandwidth;
    
	int route[K_PATH][NumberOfPhyNode];
	int hop[K_PATH];
	int distance[K_PATH];
	double modulation[K_PATH];
	int start[K_PATH];
	int width[K_PATH];
	double p[K_PATH]; // link mapping probability
    
	int map;
	int bmap;
};

struct VirtualRequest
{
	int numofvn;
	int numofvl;
	VirtualNode VNode[NumberOfPhyNode];
	VirtualLink VLink[NumberOfPhyLink];
	int success;
    int numofBackup;
    int numofBackuplink;
    
	BackupNode BNode[MaxVirtualNode+2];
	vector<VirtualLink> BLink;
    
	double availability;
};
VirtualRequest VirtualRequest[NumberOfRequest];

//uniform random number generator
double uni_rv()
{
    double rd_k = 16807.0;
    double rd_m = 2.147483647e9;
    double rv;
    
    Seed=fmod((rd_k*Seed),rd_m);
    rv=Seed/rd_m;
    return(rv);
}

//Following variables and functions are defined to find K-alternate shortest routes
int Network[NumberOfPhyNode][NumberOfPhyNode];					//Physical network topology
int Auxiliary[NumberOfPhyNode][NumberOfPhyNode];					//An auxiliary graph
int Spath[NumberOfPhyNode][NumberOfPhyNode][K_PATH];					//Distances of K-alternate routes

enum check{perm,tent};			//States of a node
struct path{											//Records a Path in a linked list
    int node;
    path* next;
};

struct indnode											//Status of a node
{
 	int pre;											// Predecessor
 	int hop;											// Length between the nodes
 	check label;										// Enumeration for permanent and tentative labels
}state[NumberOfPhyNode];

class route{
public:
	
	path *head, *tail, *inter;
	route();
	void destroy(void);
	void copy (const route& rtemp);
	void insert (int x);
	void display(void);
	void initialize();
	int extract();
    
};
route temp;

struct copies{
	route dtemp;
	copies *next;
};
class vector_route{
public:
	copies *first, *last, *middle;
	int count;
	vector_route();
	void insert(route r);
	void display(void);
	route extract();
	bool isempty();
	void destroy(void);
	void initialize();
};
vector_route v_rt, final;

int LoadTopology()
{
	int i, j;
	i = 0;
	j = 0;
    
	ifstream readTopology;
    
	readTopology.open("Network-NSF.txt");
	if(readTopology.fail())
	{
		cout<<"Load Topology Fails!";
		exit(1);
	}
    
	while(!readTopology.eof())
	{
		readTopology >> PLink[i][j].distance;
		
		if(PLink[i][j].distance>0)
			Network[i][j] = 1;
		else
			Network[i][j] = 0;
        
		j++;
		
		if(j==NumberOfPhyNode)
		{
			j = 0;
			i++;
		}
	}
    
	/*
     for(i=0;i<NumberOfPhyNode;i++)
     {
     for(j=0;j<NumberOfPhyNode;j++)
     {
     output<<Network[i][j]<<"		";
     }
     output<<endl;
     }
     */
    
	return 0;
}

int Initialization()
{
	// Physical Node Initialization
	for(int i=0;i<NumberOfPhyNode;i++)
	{
		PNode[i].compute = ComputeCapacity;
		PNode[i].storage = StorageCapacity;
		PNode[i].memory = MemoryCapacity;
        
        // assign functions to each physical node
        int numberofFunction = rand() % (3 - 1 + 1) + 4;
        PNode[i].function.reserve(numberofFunction);
        int t = 0;
        int flag = 0;
        //int count = 0;
        while (t < numberofFunction) {
            int f = rand() % (Function - 1 + 1) + 1;
            flag = 0;
            for (int p = 0; p < PNode[i].function.size(); p++) {
                if (f == PNode[i].function[p]) {
                    flag = 1;
                    break;
                }
            }
            if (flag == 1) continue;
            else {
                PNode[i].function.push_back(f);
                t++;
            }
        }
		PNode[i].availability = (rand()%(High - Low + 1) + Low) / 100.0;
	}
    
	// Physical Link Initialization
	for(int i=0;i<NumberOfPhyNode;i++)
		for(int j=0;j<NumberOfPhyNode;j++)
		{
			//PLink[i][j].availability = 0.9;
            
			for(int k=0;k<SpectrumCapacity;k++)
				PLink[i][j].spectrum[k] = 0;
		}
    
	// Virtual Request Initialization
	for(int i=0;i<NumberOfRequest;i++)
	{
		VirtualRequest[i].numofvn = 0;
		VirtualRequest[i].numofvl = 0;
		VirtualRequest[i].success = 1;
		VirtualRequest[i].availability = 0;
        VirtualRequest[i].numofBackup = 0;
        VirtualRequest[i].numofBackuplink = 0;
        
		// Virtual Node Initialization
		for(int j=0;j<NumberOfPhyNode;j++)
		{
			VirtualRequest[i].VNode[j].compute = 0;
			VirtualRequest[i].VNode[j].storage = 0;
			VirtualRequest[i].VNode[j].memory = 0;
			VirtualRequest[i].VNode[j].map = -1;
            VirtualRequest[i].VNode[j].function = -1;
            VirtualRequest[i].VNode[j].backup.reserve(MaxVirtualNode + 2);
            
		}
        for (int j = 0; j < MaxVirtualNode + 2; j++) {
            VirtualRequest[i].BNode[j].compute = 0;
			VirtualRequest[i].BNode[j].storage = 0;
			VirtualRequest[i].BNode[j].memory = 0;
			VirtualRequest[i].BNode[j].map = -1;
            VirtualRequest[i].BNode[j].function[0] = 0;
            VirtualRequest[i].BNode[j].function[1] = 0;
        }
        
		// Virtual Link Initialization
		for(int j=0;j<NumberOfPhyLink;j++)
		{
			VirtualRequest[i].VLink[j].source = -1;
			VirtualRequest[i].VLink[j].destination = -1;
			VirtualRequest[i].VLink[j].bandwidth = 0;
			VirtualRequest[i].VLink[j].map = -1;
            
			for(int k=0;k<K_PATH;k++)
			{
				for(int l=0;l<NumberOfPhyNode;l++)
					VirtualRequest[i].VLink[j].route[k][l] = -1;
                
				VirtualRequest[i].VLink[j].hop[k] = 0;
				VirtualRequest[i].VLink[j].distance[k] = 0;
				VirtualRequest[i].VLink[j].modulation[k] = 0;
				VirtualRequest[i].VLink[j].start[k] = -1;
				VirtualRequest[i].VLink[j].width[k] = 0;
				VirtualRequest[i].VLink[j].p[k] = 0;
			}
            
//			VirtualRequest[i].BLink[j].source = -1;
//			VirtualRequest[i].BLink[j].destination = -1;
//			VirtualRequest[i].BLink[j].bandwidth = 0;
//			VirtualRequest[i].BLink[j].map = -1;
//            
//			for(int k=0;k<K_PATH;k++)
//			{
//				for(int l=0;l<NumberOfPhyNode;l++)
//					VirtualRequest[i].BLink[j].route[k][l] = -1;
//                
//				VirtualRequest[i].BLink[j].hop[k] = 0;
//				VirtualRequest[i].BLink[j].distance[k] = 0;
//				VirtualRequest[i].BLink[j].modulation[k] = 0;
//				VirtualRequest[i].BLink[j].start[k] = -1;
//				VirtualRequest[i].BLink[j].width[k] = 0;
//				VirtualRequest[i].BLink[j].p[k] = 0;
//			}
            
			
		}
	}
	return 0;
}

int TrafficGenerator()
{
	for(int i=0;i<NumberOfRequest;i++)
	{
		// The number of virtual nodes in a virtual infrastructure request
//		VirtualRequest[i].numofvn = rand()%3+3;
        VirtualRequest[i].numofvn = 6;
        //int pp = 0;
        
		// The compute, storage and memory demand of each virtual node
		for(int j=0;j<VirtualRequest[i].numofvn;j++)
		{
			VirtualRequest[i].VNode[j].compute = (int)(uni_rv()*ComputeDemand);
			VirtualRequest[i].VNode[j].storage = (int)(uni_rv()*StorageDemand);
			VirtualRequest[i].VNode[j].memory = (int)(uni_rv()*MemoryDemand);
            int flag = 0;

            while (flag == 0) {
            //int func = (int)(uni_rv()*(Function+1));
                int func = rand() % (Function - 1 + 1) + 1;
            //cout<<func;
                for (int p = 0; p < j; p++) {
                    if (func == VirtualRequest[i].VNode[p].function) {
                        flag = 1;
                        break;
                    }
                }
                if(flag == 1) {
                    flag = 0;
                    continue;
                }
                else {
                    VirtualRequest[i].VNode[j].function = func;
                    break;
                }
            }
            //VirtualRequest[i].VNode[j].function = rand() % (Function - 1 + 1) + 1;
		}
//        
//        VirtualRequest[i].numofBackup = VirtualRequest[i].numofvn - 1;
//        for (int j = 0; )
//        VirtualRequest[i].BNode[j].compute = VirtualRequest[i].VNode[j].compute;
//        VirtualRequest[i].BNode[j].storage = VirtualRequest[i].VNode[j].storage;
//        VirtualRequest[i].BNode[j].memory = VirtualRequest[i].VNode[j].memory;
        
        
		// The virtual (ring) topology of a virtual infrastructure request
		if(VirtualRequest[i].numofvn==2)
		{
			VirtualRequest[i].numofvl = 1;
			VirtualRequest[i].VLink[0].source = 0;
			VirtualRequest[i].VLink[0].destination = 1;
			VirtualRequest[i].VLink[0].bandwidth = (int)(uni_rv()*BandwidthDemand);
            
//			VirtualRequest[i].BLink[0].source = VirtualRequest[i].VLink[0].source;
//			VirtualRequest[i].BLink[0].destination = VirtualRequest[i].VLink[0].destination;
//			VirtualRequest[i].BLink[0].bandwidth = VirtualRequest[i].VLink[0].bandwidth;
		}
		else
		{
			VirtualRequest[i].numofvl = VirtualRequest[i].numofvn;
			
			// 0 to n-1 form a linear array
			for(int j=0;j<VirtualRequest[i].numofvl-1;j++)
			{
				VirtualRequest[i].VLink[j].source = j;
				VirtualRequest[i].VLink[j].destination = j + 1;
				VirtualRequest[i].VLink[j].bandwidth = (int)(uni_rv()*BandwidthDemand);
                
//				VirtualRequest[i].BLink[j].source = VirtualRequest[i].VLink[j].source;
//				VirtualRequest[i].BLink[j].destination = VirtualRequest[i].VLink[j].source;
//				VirtualRequest[i].BLink[j].bandwidth = VirtualRequest[i].VLink[j].source;
			}
            
			// form the ring
			VirtualRequest[i].VLink[VirtualRequest[i].numofvl-1].source = VirtualRequest[i].numofvl-1;
			VirtualRequest[i].VLink[VirtualRequest[i].numofvl-1].destination = 0;
			VirtualRequest[i].VLink[VirtualRequest[i].numofvl-1].bandwidth = (int)(uni_rv()*BandwidthDemand);
            
//			VirtualRequest[i].BLink[VirtualRequest[i].numofvl-1].source = VirtualRequest[i].VLink[VirtualRequest[i].numofvl-1].source;
//			VirtualRequest[i].BLink[VirtualRequest[i].numofvl-1].destination = VirtualRequest[i].VLink[VirtualRequest[i].numofvl-1].source;
//			VirtualRequest[i].BLink[VirtualRequest[i].numofvl-1].bandwidth = VirtualRequest[i].VLink[VirtualRequest[i].numofvl-1].source;
		}
        
		// The bandwidth demand of each virtual link
		for(int j=0;j<VirtualRequest[i].numofvl;j++)
		{
			if(VirtualRequest[i].VLink[j].bandwidth==0)
				VirtualRequest[i].VLink[j].bandwidth = 10;
			else if (VirtualRequest[i].VLink[j].bandwidth==1)
				VirtualRequest[i].VLink[j].bandwidth = 40;
			else if (VirtualRequest[i].VLink[j].bandwidth==2)
				VirtualRequest[i].VLink[j].bandwidth = 100;
			else if (VirtualRequest[i].VLink[j].bandwidth==3)
				VirtualRequest[i].VLink[j].bandwidth = 200;
			else if (VirtualRequest[i].VLink[j].bandwidth==4)
				VirtualRequest[i].VLink[j].bandwidth = 400;
			else if (VirtualRequest[i].VLink[j].bandwidth==5)
				VirtualRequest[i].VLink[j].bandwidth = 1000;
		}
        
	}
    
	/*
     for(int i=0;i<NumberOfRequest;i++)
     {
     output<<"Virtual Request "<<i<<": "<<endl;
     output<<"# of virtual node: "<<VirtualRequest[i].numofvn<<", # of virtual links: "<<VirtualRequest[i].numofvl<<endl;
     
     // output virtual node information
     for(int j=0;j<VirtualRequest[i].numofvn;j++)
     output<<"vn"<<j<<": "<<VirtualRequest[i].VNode[j].compute<<" (compute); "
     <<endl<<"     "<<VirtualRequest[i].VNode[j].storage<<" (storage); "
     <<endl<<"     "<<VirtualRequest[i].VNode[j].memory<<" (memory); "<<endl;
     
     // output virtual link information
     for(int j=0;j<VirtualRequest[i].numofvl;j++)
     output<<"vl("<<j<<"): "<<VirtualRequest[i].VLink[j].source<<" -> "
     <<VirtualRequest[i].VLink[j].destination<<", "<<VirtualRequest[i].VLink[j].bandwidth<<endl;
     
     output<<endl;
     }
     */
    
	return 0;
}

int ReserveNodeResource(int d, int i)
{
	// reserve node resources
	PNode[VirtualRequest[d].VNode[i].map].compute = PNode[VirtualRequest[d].VNode[i].map].compute - VirtualRequest[d].VNode[i].compute;
	PNode[VirtualRequest[d].VNode[i].map].storage = PNode[VirtualRequest[d].VNode[i].map].storage - VirtualRequest[d].VNode[i].storage;
	PNode[VirtualRequest[d].VNode[i].map].memory = PNode[VirtualRequest[d].VNode[i].map].memory - VirtualRequest[d].VNode[i].memory;
	
	return 0;
}

int ReleaseNodeResource(int d)
{
	// release node resource
	for(int i=0;i<VirtualRequest[d].numofvn;i++)
		if(VirtualRequest[d].VNode[i].map!=-1)
		{
			PNode[VirtualRequest[d].VNode[i].map].compute = PNode[VirtualRequest[d].VNode[i].map].compute + VirtualRequest[d].VNode[i].compute;
			PNode[VirtualRequest[d].VNode[i].map].storage = PNode[VirtualRequest[d].VNode[i].map].storage + VirtualRequest[d].VNode[i].storage;
			PNode[VirtualRequest[d].VNode[i].map].memory = PNode[VirtualRequest[d].VNode[i].map].memory + VirtualRequest[d].VNode[i].memory;
		}
    
	return 0;
}

bool checkFunc(int j, int d, int i) {
    int func = VirtualRequest[d].VNode[i].function;
    for (int q = 0; q < PNode[j].function.size(); q++) {
        if (PNode[j].function[q] == func)
            return true;
    }
    return false;
}

int ReserveBackupNodeResource(int d);

int distance(int backupNode, int virtualN1, int virtualN2) {
    int spec1 = 0;
    int spec2 = 0;
    for (int j = 0; j < NumberOfPhyLink; j++) {
        for (int i = 0; i < SpectrumCapacity; i++) {
            if (Network[virtualN1][j] == 1 && PLink[virtualN1][j].spectrum[i] == 0)
                spec1++;
        }
    }
    for (int j = 0; j < NumberOfPhyLink; j++) {
        for (int i = 0; i < SpectrumCapacity; i++) {
            if (Network[virtualN2][j] == 1 && PLink[virtualN2][j].spectrum[i] == 0)
                spec2++;
        }
    }
//    for (int i = 0; i < SpectrumCapacity; i++) {
//        if (PLink[backupNode][virtualN2].spectrum[i] == 0)
//            spec2++;
//    }
    return spec1 < spec2 ? spec1:spec2 ;
}

struct myclass {
    bool operator() (int i,int j) { return (i > j);}
} myobject;

bool checkAv(int d) {
    int numofNode = VirtualRequest[d].numofvn;
    int cnumofNode = numofNode;
    int *vec = new int[VirtualRequest[d].numofvn];
    for (int i = 0; i < VirtualRequest[d].numofvn; i++) {
        vec[i] = -1;
    }
    double av = 1.0;
    for (int i = 0; i < cnumofNode; i++) {
        av = av * PNode[VirtualRequest[d].VNode[i].map].availability;
    }
    if (av >= Threshold) return true;
    
    int resource[NumberOfPhyNode];
    for(int i=0;i<NumberOfPhyNode;i++)
        resource[i] = PNode[i].compute + PNode[i].storage + PNode[i].memory;
    //for(int i=0;i<NumberOfPhyNode;i++)
		for(int j=0;j<VirtualRequest[d].numofvn;j++)
		{
			//if(VirtualRequest[d].VNode[j].map==i)
			//{
				resource[VirtualRequest[d].VNode[j].map] = 0;
                //i++;
				//break;
			//}
        }
		//}
    
    double newAv = 0.0;
    int count = 0;
    int *rank = new int[cnumofNode]; // index of physical nodes
    double *av_array = new double[cnumofNode];
    double *av_temp = new double[cnumofNode]; // corresponding availability
    for (int i = 0; i < cnumofNode; i++) {
        rank[i] = i;
        av_array[i] = PNode[VirtualRequest[d].VNode[i].map].availability;
        av_temp[i] = av_array[i];
    }

    int cc = 1;
    while (newAv < Threshold && count < cnumofNode - 1) {
        assert(newAv <= 1.0);
        newAv = 1.0;
        numofNode = cnumofNode - count;
//        if (count == 0) {
//            for (int i = 0; i < numofNode; i++) {
//                rank[i] = i;
//                av_array[i] = PNode[VirtualRequest[d].VNode[i].map].availability;
//                av_temp[i] = av_array[i];
//            }
//        }
        
        //sort
        int temp;
        double temp_d;
        for(int i=0;i<numofNode-1;i++) {
            for(int j=i+1;j<numofNode;j++) {
                if(av_array[j]<av_array[i])
                {
                    temp_d = av_array[i];
                    av_array[i] = av_array[j];
                    av_array[j] = temp_d;
                    temp_d = av_temp[i];
                    av_temp[i] = av_temp[j];
                    av_temp[j] = temp_d;
                    temp = rank[i];
                    rank[i] = rank[j];
                    rank[j] = temp;
                    temp = vec[i];
                    vec[i] = vec[j];
                    vec[j] = temp;
                    
                }
            }
        }
//        if(av_temp[0]>av_temp[1]) {
//            temp = 0;
//        }
        int node1 = -1;
        int node2 = -1;
        int flag = 0;
        int i = 0;
        int j = 0;
        for (i = 0; i < numofNode - 1; i++) {
            if (flag == 1) break;
            for (j = i+1; j < numofNode; j++) {
//                std::cout<<i;
//                std::cout<<j;
                int func1 = VirtualRequest[d].VNode[rank[i]].function;
                int func2 = VirtualRequest[d].VNode[rank[j]].function;
                //int func2 = 3;
                vector<int> record;
                for (int q = 0; q < NumberOfPhyNode; q++) {
                    if (resource[q] == 0) continue;
                    vector<int> func;
                    for (int h = 0; h < PNode[q].function.size(); h++) {
                        func.push_back(PNode[q].function[h]);
                    }
                    if (find(func.begin(), func.end(), func1) != func.end()) {
                        if (find(func.begin(), func.end(), func2) != func.end()) {
                            if (PNode[q].compute > VirtualRequest[d].VNode[rank[i]].compute + VirtualRequest[d].VNode[rank[j]].compute &&
                                PNode[q].memory > VirtualRequest[d].VNode[rank[i]].memory + VirtualRequest[d].VNode[rank[j]].memory &&
                                PNode[q].storage > VirtualRequest[d].VNode[rank[i]].storage + VirtualRequest[d].VNode[rank[j]].storage) {
                                record.push_back(q);
                            }
                        }
                    }
                }
                if (record.size() > 0) {
                    // find the max
                    int max = record[0];
                    for (int p = 1; p < record.size(); p++) {
                        if (distance(max, VirtualRequest[d].VNode[rank[i]].map,VirtualRequest[d].VNode[rank[j]].map) < distance(record[p],VirtualRequest[d].VNode[rank[i]].map, VirtualRequest[d].VNode[rank[j]].map))
                            max = record[p];
//                        if (resource[p] > resource[max]) {
//                            max = p;
//                        }
                    }

                    node1 = i;
                    node2 = j;
                    VirtualRequest[d].BNode[count].map = max;
                    VirtualRequest[d].numofBackup++;
                    int n1l = rank[node1]-1<0?rank[cnumofNode-1]:rank[node1]-1;
                    int n1r = rank[node1]+1==cnumofNode?0:rank[node1]+1;
                    int n2l = rank[node2]-1<0?rank[cnumofNode-1]:rank[node2]-1;
                    int n2r = rank[node2]+1==cnumofNode?0:rank[node2]+1;
                    int a = 4;
                    if (cnumofNode == 2) {
                        VirtualRequest[d].numofBackuplink = VirtualRequest[d].numofBackuplink + 2;
                        VirtualLink v1;
                        VirtualLink v2;
                        v1.source = rank[node1];
                        v1.destination = max;
                        v1.bandwidth = VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v1.source].bandwidth ? VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v1.source].bandwidth;
                        v1.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v1.route[k][l] = -1;
                                v1.hop[k] = 0;
                                v1.distance[k] = 0;
                                v1.modulation[k] = 0;
                                v1.start[k] = -1;
                                v1.width[k] = 0;
                                v1.p[k] = 0;
                            }
                        }
                        
                        v2.source = rank[node2];
                        v2.destination = max;
                        v2.bandwidth = VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v2.source].bandwidth ? VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v2.source].bandwidth;
                        v2.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v2.route[k][l] = -1;
                                v2.hop[k] = 0;
                                v2.distance[k] = 0;
                                v2.modulation[k] = 0;
                                v2.start[k] = -1;
                                v2.width[k] = 0;
                                v2.p[k] = 0;
                            }
                        }
                        VirtualRequest[d].BLink.push_back(v1);
                        VirtualRequest[d].BLink.push_back(v2);

                    }
                    else if (n1l == rank[node2]) {
                        VirtualRequest[d].numofBackuplink = VirtualRequest[d].numofBackuplink + 2;
                        VirtualLink v1;
                        VirtualLink v2;
                        v1.source = n1r;
                        v1.destination = max;
                        v1.bandwidth = VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v1.source].bandwidth ? VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v1.source].bandwidth;
                        v1.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v1.route[k][l] = -1;
                                v1.hop[k] = 0;
                                v1.distance[k] = 0;
                                v1.modulation[k] = 0;
                                v1.start[k] = -1;
                                v1.width[k] = 0;
                                v1.p[k] = 0;
                            }
                        }
                        
                        v2.source = rank[node2]-1 > 0?rank[node2] - 1: rank[cnumofNode-1];
                        v2.destination = max;
                        v2.bandwidth = VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v2.source].bandwidth ? VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v2.source].bandwidth;
                        v2.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v2.route[k][l] = -1;
                                v2.hop[k] = 0;
                                v2.distance[k] = 0;
                                v2.modulation[k] = 0;
                                v2.start[k] = -1;
                                v2.width[k] = 0;
                                v2.p[k] = 0;
                            }
                        }
                        VirtualRequest[d].BLink.push_back(v1);
                        VirtualRequest[d].BLink.push_back(v2);
                    }
                    
                    else if (n1r == rank[node2]) {
                        VirtualRequest[d].numofBackuplink = VirtualRequest[d].numofBackuplink + 2;
                        VirtualLink v1;
                        VirtualLink v2;
                        v1.source = n1l;
                        v1.destination = max;
                        v1.bandwidth = VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v1.source].bandwidth ? VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v1.source].bandwidth;
                        v1.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v1.route[k][l] = -1;
                                v1.hop[k] = 0;
                                v1.distance[k] = 0;
                                v1.modulation[k] = 0;
                                v1.start[k] = -1;
                                v1.width[k] = 0;
                                v1.p[k] = 0;
                            }
                        }
                        
                        v2.source = rank[node2]+1 == cnumofNode? 0:rank[node2]+1;
                        v2.destination = max;
                        v2.bandwidth = VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v2.source].bandwidth ? VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v2.source].bandwidth;
                        v2.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v2.route[k][l] = -1;
                                v2.hop[k] = 0;
                                v2.distance[k] = 0;
                                v2.modulation[k] = 0;
                                v2.start[k] = -1;
                                v2.width[k] = 0;
                                v2.p[k] = 0;
                            }
                        }
                        VirtualRequest[d].BLink.push_back(v1);
                        VirtualRequest[d].BLink.push_back(v2);
                    }
                    else if (n1l == n2r && n1r == n2l) {
                        VirtualRequest[d].numofBackuplink = VirtualRequest[d].numofBackuplink + 2;
                        VirtualLink v1;
                        VirtualLink v2;
                        v1.source = n1l;
                        v1.destination = max;
                        v1.bandwidth = VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v1.source].bandwidth ? VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v1.source].bandwidth;
                        v1.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v1.route[k][l] = -1;
                                v1.hop[k] = 0;
                                v1.distance[k] = 0;
                                v1.modulation[k] = 0;
                                v1.start[k] = -1;
                                v1.width[k] = 0;
                                v1.p[k] = 0;
                            }
                        }
                        
                        v2.source = n1r;
                        v2.destination = max;
                        v2.bandwidth = VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v2.source].bandwidth ? VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v2.source].bandwidth;
                        v2.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v2.route[k][l] = -1;
                                v2.hop[k] = 0;
                                v2.distance[k] = 0;
                                v2.modulation[k] = 0;
                                v2.start[k] = -1;
                                v2.width[k] = 0;
                                v2.p[k] = 0;
                            }
                        }
                        VirtualRequest[d].BLink.push_back(v1);
                        VirtualRequest[d].BLink.push_back(v2);
                    }
                    else if (n1l == n2r) {
                        VirtualRequest[d].numofBackuplink = VirtualRequest[d].numofBackuplink + 3;
                        VirtualLink v1;
                        VirtualLink v2;
                        VirtualLink v3;
                        v1.source = n1l;
                        v1.destination = max;
                        v1.bandwidth = VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v1.source].bandwidth ? VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v1.source].bandwidth;
                        v1.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v1.route[k][l] = -1;
                                v1.hop[k] = 0;
                                v1.distance[k] = 0;
                                v1.modulation[k] = 0;
                                v1.start[k] = -1;
                                v1.width[k] = 0;
                                v1.p[k] = 0;
                            }
                        }
                        
                        v2.source = n1r;
                        v2.destination = max;
                        v2.bandwidth = VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v2.source].bandwidth ? VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v2.source].bandwidth;
                        v2.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v2.route[k][l] = -1;
                                v2.hop[k] = 0;
                                v2.distance[k] = 0;
                                v2.modulation[k] = 0;
                                v2.start[k] = -1;
                                v2.width[k] = 0;
                                v2.p[k] = 0;
                            }
                        }
                        v3.source = n2l;
                        v3.destination = max;
                        v1.bandwidth = VirtualRequest[d].VLink[v3.source - 1 >= 0?v3.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v3.source].bandwidth ? VirtualRequest[d].VLink[v3.source - 1 >= 0?v3.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v3.source].bandwidth;
                        v3.map = -1;
                        for(int k=0;k<K_PATH;k++) {
                            for(int l=0;l<NumberOfPhyNode;l++) {
                                v3.route[k][l] = -1;
                                v3.hop[k] = 0;
                                v3.distance[k] = 0;
                                v3.modulation[k] = 0;
                                v3.start[k] = -1;
                                v3.width[k] = 0;
                                v3.p[k] = 0;
                            }
                        }

                        VirtualRequest[d].BLink.push_back(v1);
                        VirtualRequest[d].BLink.push_back(v2);
                        VirtualRequest[d].BLink.push_back(v3);

                    }
                        else if (n1r == n2l) {
                            VirtualRequest[d].numofBackuplink = VirtualRequest[d].numofBackuplink + 3;
                            VirtualLink v1;
                            VirtualLink v2;
                            VirtualLink v3;
                            v1.source = n1l;
                            v1.destination = max;
                            v1.bandwidth = VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v1.source].bandwidth ? VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v1.source].bandwidth;
                            v1.map = -1;
                            for(int k=0;k<K_PATH;k++) {
                                for(int l=0;l<NumberOfPhyNode;l++) {
                                    v1.route[k][l] = -1;
                                    v1.hop[k] = 0;
                                    v1.distance[k] = 0;
                                    v1.modulation[k] = 0;
                                    v1.start[k] = -1;
                                    v1.width[k] = 0;
                                    v1.p[k] = 0;
                                }
                            }
                            
                            v2.source = n1r;
                            v2.destination = max;
                            v2.bandwidth = VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v2.source].bandwidth ? VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v2.source].bandwidth;
                            v2.map = -1;
                            for(int k=0;k<K_PATH;k++) {
                                for(int l=0;l<NumberOfPhyNode;l++) {
                                    v2.route[k][l] = -1;
                                    v2.hop[k] = 0;
                                    v2.distance[k] = 0;
                                    v2.modulation[k] = 0;
                                    v2.start[k] = -1;
                                    v2.width[k] = 0;
                                    v2.p[k] = 0;
                                }
                            }
                            v3.source = n2r;
                            v3.destination = max;
                            v3.bandwidth = VirtualRequest[d].VLink[v3.source - 1 >= 0?v3.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v3.source].bandwidth ? VirtualRequest[d].VLink[v3.source - 1 >= 0?v3.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v3.source].bandwidth;
                            v3.map = -1;
                            for(int k=0;k<K_PATH;k++) {
                                for(int l=0;l<NumberOfPhyNode;l++) {
                                    v3.route[k][l] = -1;
                                    v3.hop[k] = 0;
                                    v3.distance[k] = 0;
                                    v3.modulation[k] = 0;
                                    v3.start[k] = -1;
                                    v3.width[k] = 0;
                                    v3.p[k] = 0;
                                }
                            }
                            
                                VirtualRequest[d].BLink.push_back(v1);
                                VirtualRequest[d].BLink.push_back(v2);
                                VirtualRequest[d].BLink.push_back(v3);
                                
                            }
                            else {
                                VirtualRequest[d].numofBackuplink = VirtualRequest[d].numofBackuplink + 4;
                                VirtualLink v1;
                                VirtualLink v2;
                                VirtualLink v3;
                                VirtualLink v4;
                                v1.source = n1l;
                                v1.destination = max;
                                v1.bandwidth = VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v1.source].bandwidth ? VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v1.source].bandwidth;
                                v1.map = -1;
                                for(int k=0;k<K_PATH;k++) {
                                    for(int l=0;l<NumberOfPhyNode;l++) {
                                        v1.route[k][l] = -1;
                                        v1.hop[k] = 0;
                                        v1.distance[k] = 0;
                                        v1.modulation[k] = 0;
                                        v1.start[k] = -1;
                                        v1.width[k] = 0;
                                        v1.p[k] = 0;
                                    }
                                }
                                
                                v2.source = n1r;
                                v2.destination = max;
                                v2.bandwidth = VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v2.source].bandwidth ? VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v2.source].bandwidth;
                                v2.map = -1;
                                for(int k=0;k<K_PATH;k++) {
                                    for(int l=0;l<NumberOfPhyNode;l++) {
                                        v2.route[k][l] = -1;
                                        v2.hop[k] = 0;
                                        v2.distance[k] = 0;
                                        v2.modulation[k] = 0;
                                        v2.start[k] = -1;
                                        v2.width[k] = 0;
                                        v2.p[k] = 0;
                                    }
                                }
                                v3.source = n2r;
                                v3.destination = max;
                                v3.bandwidth = VirtualRequest[d].VLink[v3.source - 1 >= 0?v3.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v3.source].bandwidth ? VirtualRequest[d].VLink[v3.source - 1 >= 0?v3.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v3.source].bandwidth;
                                v3.map = -1;
                                for(int k=0;k<K_PATH;k++) {
                                    for(int l=0;l<NumberOfPhyNode;l++) {
                                        v3.route[k][l] = -1;
                                        v3.hop[k] = 0;
                                        v3.distance[k] = 0;
                                        v3.modulation[k] = 0;
                                        v3.start[k] = -1;
                                        v3.width[k] = 0;
                                        v3.p[k] = 0;
                                    }
                                }
                                    v4.source = n2l;
                                    v4.destination = max;
                                    v4.bandwidth = VirtualRequest[d].VLink[v4.source - 1 >= 0?v4.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v4.source].bandwidth ? VirtualRequest[d].VLink[v4.source - 1 >= 0?v4.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v4.source].bandwidth;
                                    v4.map = -1;
                                    for(int k=0;k<K_PATH;k++) {
                                        for(int l=0;l<NumberOfPhyNode;l++) {
                                            v4.route[k][l] = -1;
                                            v4.hop[k] = 0;
                                            v4.distance[k] = 0;
                                            v4.modulation[k] = 0;
                                            v4.start[k] = -1;
                                            v4.width[k] = 0;
                                            v4.p[k] = 0;
                                        }
                                    }

                                    
                                    VirtualRequest[d].BLink.push_back(v1);
                                    VirtualRequest[d].BLink.push_back(v2);
                                    VirtualRequest[d].BLink.push_back(v3);
                                    VirtualRequest[d].BLink.push_back(v4);
                                    

                            }

                        
                        
//                    VirtualLink v1;
//                    VirtualLink v2;
//                    v1.source = rank[node1];
//                    v1.destination = max;
//                    v1.bandwidth = VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v1.source].bandwidth ? VirtualRequest[d].VLink[v1.source - 1 >= 0?v1.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v1.source].bandwidth;
//                    v1.map = -1;
//                    for(int k=0;k<K_PATH;k++) {
//                        for(int l=0;l<NumberOfPhyNode;l++) {
//                            v1.route[k][l] = -1;
//                            v1.hop[k] = 0;
//                            v1.distance[k] = 0;
//                            v1.modulation[k] = 0;
//                            v1.start[k] = -1;
//                            v1.width[k] = 0;
//                            v1.p[k] = 0;
//                        }
//                    }
//
//                    v2.source = rank[node2];
//                    v2.destination = max;
//                    v2.bandwidth = VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth > VirtualRequest[d].VLink[v2.source].bandwidth ? VirtualRequest[d].VLink[v2.source - 1 >= 0?v2.source - 1:VirtualRequest[i].numofvl-1].bandwidth : VirtualRequest[d].VLink[v2.source].bandwidth;
//                    v2.map = -1;
//                    for(int k=0;k<K_PATH;k++) {
//                        for(int l=0;l<NumberOfPhyNode;l++) {
//                            v2.route[k][l] = -1;
//                            v2.hop[k] = 0;
//                            v2.distance[k] = 0;
//                            v2.modulation[k] = 0;
//                            v2.start[k] = -1;
//                            v2.width[k] = 0;
//                            v2.p[k] = 0;
//                        }
//                    }

                    
                    
                    VirtualRequest[d].VNode[rank[i]].backup.push_back(max);
                    VirtualRequest[d].VNode[rank[j]].backup.push_back(max);
                    VirtualRequest[d].BNode[count].function[0] = VirtualRequest[d].VNode[rank[i]].function;
                    VirtualRequest[d].BNode[count].function[1] = VirtualRequest[d].VNode[rank[j]].function;
                    VirtualRequest[d].BNode[count].storage = VirtualRequest[d].VNode[rank[i]].storage + VirtualRequest[d].VNode[rank[j]].storage;
                    VirtualRequest[d].BNode[count].memory = VirtualRequest[d].VNode[rank[i]].memory + VirtualRequest[d].VNode[rank[j]].memory;
                    VirtualRequest[d].BNode[count].compute = VirtualRequest[d].VNode[rank[i]].compute + VirtualRequest[d].VNode[rank[j]].compute;
                    resource[max] = 0;
                    count++;
                    flag = 1;
                    break;
                }
                else continue;
            }
        }
        if (flag == 1) {
            if (vec[node1] == -1 && vec[node2] == -1) {
                vec[node1] = cc;
                vec[node2] = cc;
                cc++;
                av_array[node1] = 1 - (1 - PNode[VirtualRequest[d].BNode[count-1].map].availability) * (1 - av_temp[node1] * av_temp[node2]);
                av_array[node2] = av_array[node1];
                assert(av_temp[node1] <= av_temp[node2]);
                if (av_temp[node1] <= av_temp[node2]) {
                    if (node2 != cnumofNode - count) {
                        int tmp;
                        double tmp_d;
                        tmp = rank[node2];
                        rank[node2] = rank[cnumofNode - count];
                        rank[cnumofNode - count] = tmp;
                        tmp = vec[node2];
                        vec[node2] = vec[cnumofNode - count];
                        vec[cnumofNode - count] = tmp;
                        tmp_d = av_array[node2];
                        av_array[node2] = av_array[cnumofNode - count];
                        av_array[cnumofNode - count] = tmp_d;
                        tmp_d = av_temp[node2];
                        av_temp[node2] = av_temp[cnumofNode - count];
                        av_temp[cnumofNode - count] = tmp_d;
                    }
                }
            }
            else if (vec[node1] != -1 && vec[node2] == -1) {
                vec[node2] = vec[node1];
                double neg_backup = 1.0;
                for (int ii = 0; ii < VirtualRequest[d].VNode[rank[node1]].backup.size(); ii++) {
                    if (VirtualRequest[d].VNode[rank[node1]].backup[ii] == VirtualRequest[d].BNode[count-1].map) {
                        continue;
                    }
                    else neg_backup = neg_backup * (1 - PNode[VirtualRequest[d].VNode[rank[node1]].backup[ii]].availability);
                }
//                double pri = 1.0;
//                for (int i = 0; i < cnumofNode; i++) {
//                    if (vec[i] == vec[node1]) {
//                        if (rank[i] != rank[node1] && rank[i] != rank[node2]) {
//                            pri = pri * av_temp[i];
//                        }
//                    }
//                }
                av_array[node2] = av_array[node1] * (1 - (1 - av_temp[node2]) * (1 - PNode[VirtualRequest[d].BNode[count-1].map].availability)) + PNode[VirtualRequest[d].BNode[count-1].map].availability * (1 - av_temp[node1]) * neg_backup * av_array[node2] / (1 - (1 - av_temp[node2]) * neg_backup);
                av_array[node1] = av_array[node2];
//                assert(av_temp[node1] <= av_temp[node2]);
                if (av_temp[node1] <= av_temp[node2]) {
                    if (node2 != cnumofNode - count) {
                        int tmp;
                        double tmp_d;
                        tmp = rank[node2];
                        rank[node2] = rank[cnumofNode - count];
                        rank[cnumofNode - count] = tmp;
                        tmp = vec[node2];
                        vec[node2] = vec[cnumofNode - count];
                        vec[cnumofNode - count] = tmp;
                        tmp_d = av_array[node2];
                        av_array[node2] = av_array[cnumofNode - count];
                        av_array[cnumofNode - count] = tmp_d;
                        tmp_d = av_temp[node2];
                        av_temp[node2] = av_temp[cnumofNode - count];
                        av_temp[cnumofNode - count] = tmp_d;
                    }
                }
            }
            else if (vec[node2] != -1 && vec[node1] == -1) {
                vec[node1] = vec[node2];
                double neg_backup = 1.0;
                for (int ii = 0; ii < VirtualRequest[d].VNode[rank[node2]].backup.size(); ii++) {
                    if (VirtualRequest[d].VNode[rank[node2]].backup[ii] == VirtualRequest[d].BNode[count-1].map) {
                        continue;
                    }
                    else neg_backup = neg_backup * (1 - PNode[VirtualRequest[d].VNode[rank[node2]].backup[ii]].availability);
                }
//                double pri = 1.0;
//                for (int i = 0; i < cnumofNode; i++) {
//                    if (vec[i] == vec[node2]) {
//                        if (rank[i] != rank[node2] && rank[i] != rank[node1]) {
//                            pri = pri * av_temp[i];
//                        }
//                    }
//                }
                av_array[node1] = av_array[node2] * (1 - (1 - av_temp[node1]) * (1 - PNode[VirtualRequest[d].BNode[count-1].map].availability)) + PNode[VirtualRequest[d].BNode[count-1].map].availability * (1 - av_temp[node2]) * neg_backup * av_array[node2] / (1 - (1 - av_temp[node2]) * neg_backup);
                av_array[node2] = av_array[node1];
//                assert(av_temp[node1] <= av_temp[node2]);
                if (av_temp[node1] <= av_temp[node2]) {
                    if (node2 != cnumofNode - count) {
                        int tmp;
                        double tmp_d;
                        tmp = rank[node2];
                        rank[node2] = rank[cnumofNode - count];
                        rank[cnumofNode - count] = tmp;
                        tmp = vec[node2];
                        vec[node2] = vec[cnumofNode - count];
                        vec[cnumofNode - count] = tmp;
                        tmp_d = av_array[node2];
                        av_array[node2] = av_array[cnumofNode - count];
                        av_array[cnumofNode - count] = tmp_d;
                        tmp_d = av_temp[node2];
                        av_temp[node2] = av_temp[cnumofNode - count];
                        av_temp[cnumofNode - count] = tmp_d;
                    }
                }
            }
            else if (vec[node1] != -1 && vec[node2] != -1) {
                                //left not working
//                int lSize = VirtualRequest[d].VNode[rank[node1]].backup.size();
                double left = 0.0;
                double neg_backup_l = 1.0;
                for (int ii = 0; ii < VirtualRequest[d].VNode[rank[node1]].backup.size(); ii++) {
                    if (VirtualRequest[d].VNode[rank[node1]].backup[ii] == VirtualRequest[d].BNode[count-1].map) {
                        continue;
                    }
                    else neg_backup_l = neg_backup_l * (1 - PNode[VirtualRequest[d].VNode[rank[node1]].backup[ii]].availability);
                }
                left = (1 - av_temp[node1]) * neg_backup_l * av_array[node1] / (1 - (1 - av_temp[node1]) * neg_backup_l);
                
                double right = 0.0;
                double neg_backup_r = 1.0;
                for (int ii = 0; ii < VirtualRequest[d].VNode[rank[node2]].backup.size(); ii++) {
                    if (VirtualRequest[d].VNode[rank[node2]].backup[ii] == VirtualRequest[d].BNode[count-1].map) {
                        continue;
                    }
                    else neg_backup_r = neg_backup_r * (1 - PNode[VirtualRequest[d].VNode[rank[node2]].backup[ii]].availability);
                }
                right = (1 - av_temp[node2]) * neg_backup_r * av_array[node2] / (1 - (1 - av_temp[node2]) * neg_backup_r);
                
                
//                int leftS = 0;
//                if (lSize == 2) {
//                    int ii;
//                    for (ii = 0; ii < cnumofNode; ii++) {
//                        if (vec[ii] == vec[node1] && rank[ii] != rank[node1]) {
//                            left = av_temp[ii];
//                            leftS++;
//                        }
//                    }
//                    if (leftS != 1) {
//                        int jj;
//                        for (jj = 0; jj < cnumofNode; jj++) {
//                            if (vec[jj] == vec[node1] && rank[jj] != rank[node1] && rank[jj] != rank[ii]) {
//                                break;
//                            }
//                        }
//                        left = 1 - (1 - PNode[VirtualRequest[d].VNode[rank[jj]].map].availability) * (1- av_temp[ii] * av_temp[jj]);
//                    }
//                    left = left * (1 - av_temp[node1]) * (1 - PNode[VirtualRequest[d].VNode[rank[node1]].backup[0]].availability);
//                }
//                else if (lSize == 3){
//                    // at least one of backups and primary node works
//                    double atLeast = 1.0;
//                    for (int ii = 0; ii < VirtualRequest[d].VNode[rank[node1]].backup.size(); ii++) {
//                        atLeast = atLeast * (1 - PNode[VirtualRequest[d].VNode[rank[node1]].backup[ii]].availability);
//                    }
//                    for (int ii = 0; ii < cnumofNode; ii++) {
//                        if (vec[ii] == vec[node1] && rank[ii] != rank[node1]) {
//                            atLeast = atLeast * av_temp[ii];
//                        }
//                    }
//                    double neg_backup = 1.0;
//                    for (int ii = 0; ii < VirtualRequest[d].VNode[rank[node1]].backup.size(); ii++) {
//                        if (VirtualRequest[d].VNode[rank[node1]].backup[ii] == VirtualRequest[d].BNode[count-1].map) {
//                            continue;
//                        }
//                        else neg_backup = neg_backup * (1 - PNode[VirtualRequest[d].VNode[rank[node1]].backup[ii]].availability);
//                    }
//                    left = atLeast * neg_backup;
//                }
                
//                int rSize = VirtualRequest[d].VNode[rank[node2]].backup.size();
//                double right = 0.0;
//                int rightS = 0;
//                if (rSize == 2) {
//                    int ii;
//                    for (ii = 0; ii < cnumofNode; ii++) {
//                        if (vec[ii] == vec[node2] && rank[ii] != rank[node2]) {
//                            right = av_temp[ii];
//                            rightS++;
//                        }
//                    }
//                    if (rightS != 1) {
//                        int jj;
//                        for (jj = 0; jj < cnumofNode; jj++) {
//                            if (vec[jj] == vec[node2] && rank[jj] != rank[node2] && rank[jj] != rank[ii]) {
//                                break;
//                            }
//                        }
//                        right = 1 - (1 - PNode[VirtualRequest[d].VNode[rank[jj]].map].availability) * (1- av_temp[ii] * av_temp[jj]);
//                    }
//                    right = right * (1 - av_temp[node2]) * (1 - PNode[VirtualRequest[d].VNode[rank[node2]].backup[0]].availability);
//                }
//                else if (rSize == 3){
//                    // at least one of backups and primary node works
//                    double atLeast = 1 - av_temp[node2];
//                    for (int ii = 0; ii < VirtualRequest[d].VNode[rank[node2]].backup.size(); ii++) {
//                        atLeast = atLeast * (1 - PNode[VirtualRequest[d].VNode[rank[node2]].backup[ii]].availability);
//                    }
//                    for (int ii = 0; ii < cnumofNode; ii++) {
//                        if (vec[ii] == vec[node2] && rank[ii] != rank[node2]) {
//                            atLeast = atLeast * av_temp[ii];
//                        }
//                    }
//                    double neg_backup = 1.0;
//                    for (int ii = 0; ii < VirtualRequest[d].VNode[rank[node1]].backup.size(); ii++) {
//                        if (VirtualRequest[d].VNode[rank[node2]].backup[ii] == VirtualRequest[d].BNode[count-1].map) {
//                            continue;
//                        }
//                        else neg_backup = neg_backup * (1 - PNode[VirtualRequest[d].VNode[rank[node2]].backup[ii]].availability);
//                    }
//                    right = atLeast * neg_backup;
//                    
//                }
                av_array[node1] = av_array[node1] * av_array[node2] + PNode[VirtualRequest[d].BNode[count-1].map].availability * (left * av_array[node2] + av_array[node1] * right + right * left);
                if (vec[node1] < vec[node2]) {
                    for (int i = 0; i < cnumofNode; i++) {
                        if (vec[i] == vec[node2]) {
                            vec[i] = vec[node1];
                        }
                    }
                    
                }
                else {
                    for (int i = 0; i < cnumofNode; i++) {
                        if (vec[i] == vec[node1]) {
                            vec[i] = vec[node2];
                        }
                    }
                }
//                assert(av_temp[node1] <= av_temp[node2]);
                if (av_temp[node1] <= av_temp[node2]) {
                    if (node2 != cnumofNode - count) {
                        int tmp;
                        double tmp_d;
                        tmp = rank[node2];
                        rank[node2] = rank[cnumofNode - count];
                        rank[cnumofNode - count] = tmp;
//                        tmp = vec[node2];
//                        vec[node2] = vec[cnumofNode - count];
//                        vec[cnumofNode - count] = tmp;
                        tmp_d = av_array[node2];
                        av_array[node2] = av_array[cnumofNode - count];
                        av_array[cnumofNode - count] = tmp_d;
                        tmp_d = av_temp[node2];
                        av_temp[node2] = av_temp[cnumofNode - count];
                        av_temp[cnumofNode - count] = tmp_d;
                    }
                }

            }
            for (int kk = 0; kk < cnumofNode; kk++) {
                if (av_array[kk] >= 1) {
                    av_array[kk] = 0.999999999999999;
                }
            }
            //calculate newAv
            for (int ii = 0; ii < cnumofNode - count; ii++) {
                //assert(av_array[ii] <= 1);
                newAv = newAv * av_array[ii];
            }
            if (newAv >= Threshold) return true;
            else continue;
        }
        else return false;
    }
    if (newAv >= Threshold) return true;
    else return false;
}

int SpecCheck(int PhyN) {
    int min = 10000;
    for (int i = 0; i < NumberOfPhyNode; i++) {
        int total = 0;
        for(int k=0;k<SpectrumCapacity;k++) {
            if (PLink[i][PhyN].spectrum[k] == 0 && Network[i][PhyN] == 1) {
                total++;
            }
        }
        for(int k=0;k<SpectrumCapacity;k++) {
            if (PLink[PhyN][i].spectrum[k] == 0 && Network[PhyN][i] == 1) {
                total++;
            }
        }
        if (total < min) {
            min = total;
        }
    }
    return min;
}

int VirtualNodeMapping(int d)
{
	int success = 1;
    
	// sort the physical node in a descending order according to their available resources
	int rank[NumberOfPhyNode];
	int resource[NumberOfPhyNode];
    
	// initialization
	for(int i=0;i<NumberOfPhyNode;i++)
	{
		rank[i] = i;
		resource[i] = 0;
	}
    
	// calculate the available resource
	for(int i=0;i<NumberOfPhyNode;i++)
		resource[i] = PNode[i].compute + PNode[i].storage + PNode[i].memory;
    
	// sort the physical node
	int temp = 0;
    
	for(int i=0;i<NumberOfPhyNode-1;i++)
		for(int j=i;j<NumberOfPhyNode;j++)
			if(resource[rank[j]]>resource[rank[i]])
			{
				temp = rank[i];
				rank[i] = rank[j];
				rank[j] = temp;
			}
    
	// virtual node mapping
	for(int i=0;i<VirtualRequest[d].numofvn;i++)
	{
        vector<int> record;
        record.reserve(NumberOfPhyLink);
		for(int j=0;j<NumberOfPhyNode;j++)
		{
			int flag = 1; // every physical node is a candidate
            
			// virtual nodes from the same VT cannot be mapped to the same physical node
			for(int k=0;k<i;k++)
			{
				if(VirtualRequest[d].VNode[k].map==rank[j])
				{
					flag = 0;
					break;
				}
			}
            
			// check if the physical node has enough available resources
			if(flag)
			{
				if((PNode[rank[j]].compute<VirtualRequest[d].VNode[i].compute)
                   ||(PNode[rank[j]].storage<VirtualRequest[d].VNode[i].storage)
                   ||(PNode[rank[j]].memory<VirtualRequest[d].VNode[i].memory) || !checkFunc(rank[j], d, i))
				{
					flag = 0;
				}
			}
            
			// if the above two constraints are satisfied, premap the vn to the qualifed pn and reserve node resources
			if(flag)
			{
				record.push_back(rank[j]);
//                VirtualRequest[d].VNode[i].map = rank[j];
//				ReserveNodeResource(d, i);
//				break;
			}
		}
        int max = record[0];
        
        for (int ii = 1; ii < record.size(); ii++) {
            if (SpecCheck(max) < SpecCheck(record[ii])){
                max = record[ii];
            }
        }
        VirtualRequest[d].VNode[i].map = max;
	}
    
	// if any of the virtual nodes cannot be mapped, the request is rejected and release the reserved node resources
	for(int i=0;i<VirtualRequest[d].numofvn;i++) {
        if(VirtualRequest[d].VNode[i].map==-1)
		{
			success = 0;
			ReleaseNodeResource(d);
			break;
		}
    }
	if (success != 0) {
        if (checkAv(d) == false) {
            cout<<"av not achieved"<<endl;
            success = 0;
            ReleaseNodeResource(d);
        }
        else {
            ReserveBackupNodeResource(d);
        }
    }
    else {
        cout<<"node resource not enough"<<endl;
    }
	   
	VirtualRequest[d].success = success;
    
	return success;
}



//Finds K alternate routes between pair of nodes
void Routing(int r, int l){
	// Added by Zilong
	int arr[NumberOfPhyNode][NumberOfPhyNode][K_PATH][NumberOfPhyNode];
	vector<int> K_routes[NumberOfPhyNode][NumberOfPhyNode][K_PATH];		//K-alternate routes
	int sournd = VirtualRequest[r].VNode[VirtualRequest[r].VLink[l].source].map;
	int destnd = VirtualRequest[r].VNode[VirtualRequest[r].VLink[l].destination].map;
	// Added by Zilong
    
	int no_path=0, k=0;
	struct indnode *q;
	for(q=&state[0];q<&state[NumberOfPhyNode];q++)
 	{
        q->pre= -1;
        q->hop = INF;
        q->label=tent;
 	}
	state[sournd].hop=0;
	state[sournd].label=perm;
	temp.insert(sournd);
	v_rt.insert(temp);
	temp.initialize();
	bool tag=false;
	k=sournd;
	do{
		for(q=&state[0];q<&state[NumberOfPhyNode];q++)
 		{
 			q->label=tent;
 		}
		temp = v_rt.extract();
		temp.inter=temp.head;
		while(temp.inter!=NULL){
			state[temp.inter->node].label=perm;
			temp.inter=temp.inter->next;
		}
		route dummy;
		dummy.copy(temp);
		k = temp.extract();
		tag=true;
		for(int i=0;i<NumberOfPhyNode;i++){
			route temp1;
			temp1.copy(dummy);
 			if(Network[k][i]!=0 && state[i].label==tent){
                
				temp1.insert(i);
				if( i!=destnd){
					v_rt.insert(temp1);
					tag=true;
 		        }
				else{
					final.insert(temp1);
					no_path++;
					tag=true;
				}
			}
			else{
				temp1.destroy();
                
			}
		}
		dummy.destroy();
		temp.destroy();
	}while(no_path!=K_PATH && !v_rt.isempty() );
    
	vector<int> v[K_PATH];
	temp.initialize();
	no_path=0;
	while(!final.isempty()){
		temp = final.extract();
		temp.inter = temp.head;
		while(temp.inter!=NULL){
			v[no_path].push_back(temp.inter->node);
			temp.inter = temp.inter->next;
		}
		no_path++;
	}
    
	for(int j=0;j<K_PATH;j++){
		for(int i=v[j].size()-1; i>=0; i--){
			K_routes[sournd][destnd][j].push_back(v[j][i]);
		}
		v[j].clear();
	}
    
    
	// Added by Zilong
	for(int i=0;i<NumberOfPhyNode;i++)
		for(int j=0;j<NumberOfPhyNode;j++)
			for(int k=0;k<K_PATH;k++)
				for(int l=0;l<NumberOfPhyNode;l++)
					arr[i][j][k][l] = -1;
    
	for(int k=0;k<K_PATH;k++)
		std::copy(K_routes[sournd][destnd][k].begin(), K_routes[sournd][destnd][k].end(), arr[sournd][destnd][k]);
    
	for(int k=0;k<K_PATH;k++)
		for(int j=0;j<NumberOfPhyNode;j++)
		{
			if(arr[sournd][destnd][k][j]!=-1)
			{
				VirtualRequest[r].VLink[l].route[k][j] = arr[sournd][destnd][k][j];
				VirtualRequest[r].VLink[l].hop[k] = VirtualRequest[r].VLink[l].hop[k] + 1;
			}
		}
	// Added by Zilong
    
	
	temp.destroy();
	v_rt.destroy();
	final.destroy();
	v_rt.initialize();
	final.initialize();
	temp.initialize();
}

int Modulation(int r, int l)
{
	for(int k=0;k<K_PATH;k++)
	{
		// calculate the path distance
		for(int j=0;j<VirtualRequest[r].VLink[l].hop[k]-1;j++)
		{
			VirtualRequest[r].VLink[l].distance[k] = VirtualRequest[r].VLink[l].distance[k] + PLink[VirtualRequest[r].VLink[l].route[k][j]][VirtualRequest[r].VLink[l].route[k][j+1]].distance;
		}
        
		// assign the modulation format
		if((VirtualRequest[r].VLink[l].distance[k]-1000)<=0)
			VirtualRequest[r].VLink[l].modulation[k] = QAM;
		else if(((VirtualRequest[r].VLink[l].distance[k]-1000)>0)&&((VirtualRequest[r].VLink[l].distance[k]-3000)<=0))
			VirtualRequest[r].VLink[l].modulation[k] = QPSK;
		else if(((VirtualRequest[r].VLink[l].distance[k]-3000)>0)&&((VirtualRequest[r].VLink[l].distance[k]-8000)<=0))
			VirtualRequest[r].VLink[l].modulation[k] = BPSK;
		else
			VirtualRequest[r].VLink[l].modulation[k] = 1;
        
		// calculate the required spectrum width
		VirtualRequest[r].VLink[l].width[k] = ceil( (VirtualRequest[r].VLink[l].bandwidth/VirtualRequest[r].VLink[l].modulation[k])/Slot);
	}
	
	return 0;
}

int SpectrumAllocation(int r, int l)
{
	int success = 1;
	double maxprobability = 0;
    
	for(int k=0;k<K_PATH;k++)
	{
		int flag = 1; // flag for the lowest start spectrum slot
        
		// find the bitmap along the path
		int bitmap[SpectrumCapacity];
        
		// initialize bitmap
		for(int i=0;i<SpectrumCapacity;i++)
			bitmap[i] = 0;
        
		// find the bitmap
		for(int i=0;i<SpectrumCapacity;i++)
			for(int j=0;j<VirtualRequest[r].VLink[l].hop[k]-1;j++)
			{
				if(PLink[VirtualRequest[r].VLink[l].route[k][j]][VirtualRequest[r].VLink[l].route[k][j+1]].spectrum[i]==1)
					bitmap[i] = 1;
			}
        
		// calculate the link mapping probability
		for(int i=0;i<SpectrumCapacity;i++)
		{
			int availableslot = 0;
            
			for(int j=i;j<i+VirtualRequest[r].VLink[l].width[k];j++)
				availableslot = availableslot + bitmap[j];
            
			if(availableslot==0)
			{
				VirtualRequest[r].VLink[l].p[k] = VirtualRequest[r].VLink[l].p[k] + 1;
                
				if(flag)
				{
					VirtualRequest[r].VLink[l].start[k] = i;
					flag = 0;
				}
			}
		}
        
		VirtualRequest[r].VLink[l].p[k] = VirtualRequest[r].VLink[l].p[k]/(SpectrumCapacity-VirtualRequest[r].VLink[l].width[k]+1);
        
		// choose the path with the max link mapping probability
		if(VirtualRequest[r].VLink[l].p[k]>maxprobability)
		{
			VirtualRequest[r].VLink[l].map = k;
			maxprobability = VirtualRequest[r].VLink[l].p[k];
		}
	}
    
	if(maxprobability==0)
		success = 0;
    
	return success;
}

int ReserveLinkResource(int d, int i)
{
	// reserve link resources
	for(int j=0;j<VirtualRequest[d].VLink[i].hop[VirtualRequest[d].VLink[i].map]-1;j++)
		for(int s=VirtualRequest[d].VLink[i].start[VirtualRequest[d].VLink[i].map];s<VirtualRequest[d].VLink[i].start[VirtualRequest[d].VLink[i].map]+VirtualRequest[d].VLink[i].width[VirtualRequest[d].VLink[i].map];s++)
		{
			PLink[VirtualRequest[d].VLink[i].route[VirtualRequest[d].VLink[i].map][j]][VirtualRequest[d].VLink[i].route[VirtualRequest[d].VLink[i].map][j+1]].spectrum[s] = 1;
			PLink[VirtualRequest[d].VLink[i].route[VirtualRequest[d].VLink[i].map][j+1]][VirtualRequest[d].VLink[i].route[VirtualRequest[d].VLink[i].map][j]].spectrum[s] = 1;
		}
    
	return 0;
}

int ReleaseLinkResource(int d)
{
	// release link resources
	for(int i=0;i<VirtualRequest[d].numofvl;i++)
		if(VirtualRequest[d].VLink[i].map!=-1)
		{
			for(int j=0;j<VirtualRequest[d].VLink[i].hop[VirtualRequest[d].VLink[i].map]-1;j++)
				for(int s=VirtualRequest[d].VLink[i].start[VirtualRequest[d].VLink[i].map];s<VirtualRequest[d].VLink[i].start[VirtualRequest[d].VLink[i].map]+VirtualRequest[d].VLink[i].width[VirtualRequest[d].VLink[i].map];s++)
				{
					PLink[VirtualRequest[d].VLink[i].route[VirtualRequest[d].VLink[i].map][j]][VirtualRequest[d].VLink[i].route[VirtualRequest[d].VLink[i].map][j+1]].spectrum[s] = 0;
					PLink[VirtualRequest[d].VLink[i].route[VirtualRequest[d].VLink[i].map][j+1]][VirtualRequest[d].VLink[i].route[VirtualRequest[d].VLink[i].map][j]].spectrum[s] = 0;
				}
		}
    
	return 0;
}

void BackupRouting(int r, int l);
int BackupModulation(int d, int i);
int BackupSpectrumAllocation(int r, int l);
int ReserveBackupNodeResource(int d);
int ReleaseBackupNodeResource(int d);
int ReserveBackupLinkResource(int d, int i);
int ReleaseBackupLinkResource(int d);

int VirtualLinkMapping(int d)
{
	int success = 1;
    
	for(int i=0;i<VirtualRequest[d].numofvl;i++)
	{
		Routing(d, i);
        
		Modulation(d, i);
        
		// if virtual link can be provisioned, reserve the according link resources
		if(SpectrumAllocation(d, i))
			ReserveLinkResource(d, i);
        
		// if virtual link CANNOT be provisioned, release the reserved node AND link resource
		else if(SpectrumAllocation(d, i)==0)
		{
			success = 0;
			ReleaseNodeResource(d);
			ReleaseLinkResource(d);
            VirtualRequest[d].success = success;
            return success;
			//break;
		}
	}
    
	for(int i=0;i<VirtualRequest[d].numofBackuplink;i++)
	{
		BackupRouting(d, i);
        
		BackupModulation(d, i);
        
		// if virtual link can be provisioned, reserve the according link resources
		if(BackupSpectrumAllocation(d, i))
			ReserveBackupLinkResource(d, i);
        
		// if virtual link CANNOT be provisioned, release the reserved node AND link resource
		else if(BackupSpectrumAllocation(d, i)==0)
		{
			success = 0;
			ReleaseBackupNodeResource(d);
			ReleaseBackupLinkResource(d);
			break;
		}
	}

    
	VirtualRequest[d].success = success;
	
	return success;
}

int DisplayPhysicalResource(int d)
{
	// display the virtual infrastructure mapping information
	output<<"#######################################################"<<endl;
    
	output<<"Virtual Request #"<<d<<", with "<<VirtualRequest[d].numofvn<<" virtual nodes, and "
    <<VirtualRequest[d].numofvl<<" virtual links"<<endl;
    
	if(VirtualRequest[d].success)
	{
		output<<"---------------------------------------------------"<<endl;
		output<<"virtual node mapping"<<endl;
		for(int i=0;i<VirtualRequest[d].numofvn;i++)
			output<<"vn"<<i<<"->pn"<<VirtualRequest[d].VNode[i].map
			<<", "<<VirtualRequest[d].VNode[i].compute<<" (compute)"
			<<", "<<VirtualRequest[d].VNode[i].storage<<" (storage)"
			<<", "<<VirtualRequest[d].VNode[i].memory<<" (memory)"<<endl;
		output<<"---------------------------------------------------"<<endl;
		output<<"virtual link mapping"<<endl;
		for(int i=0;i<VirtualRequest[d].numofvl;i++)
		{
			output<<"vl"<<i<<" ("<<VirtualRequest[d].VLink[i].source<<", "
            <<VirtualRequest[d].VLink[i].destination<<"), "
            <<VirtualRequest[d].VLink[i].bandwidth<<" Gbps"<<endl;
            
			for(int j=0;j<VirtualRequest[d].VLink[i].hop[VirtualRequest[d].VLink[i].map];j++)
				output<<VirtualRequest[d].VLink[i].route[VirtualRequest[d].VLink[i].map][j]<<"->";
			output<<", start from "<<VirtualRequest[d].VLink[i].start[VirtualRequest[d].VLink[i].map]
            <<", with width "<<VirtualRequest[d].VLink[i].width[VirtualRequest[d].VLink[i].map]<<endl;
			
			/*
             output<<"all k-shortest path information"<<endl;
             for(int k=0;k<K_PATH;k++)
             {
             for(int j=0;j<VirtualRequest[d].VLink[i].hop[k];j++)
             output<<VirtualRequest[d].VLink[i].route[k][j]<<"->";
             output<<", distance "<<VirtualRequest[d].VLink[i].distance[k]<<" km, "
             <<VirtualRequest[d].VLink[i].modulation[k]<<" b/s/Hz, "
             <<VirtualRequest[d].VLink[i].width[k]<<" slots "
             <<VirtualRequest[d].VLink[i].p[k]<<" percent";
             output<<endl;
             }
             */
		}
	}
	else
		output<<"The request is BLOCKED!!!"<<endl;
	output<<"#######################################################"<<endl;
    
	/*
     // display physical resources
     output<<"*******************************************************"<<endl;
     output<<"Physical Node Resource Information:"<<endl;
     for(int i=0;i<NumberOfPhyNode;i++)
     output<<"pn"<<i<<": "<<PNode[i].compute<<" (compute), "
     <<PNode[i].storage<<" (storage), "
     <<PNode[i].memory<<" (memory)"<<endl;
     output<<"---------------------------------------------------"<<endl;
     output<<"Physical Link Resource Information:"<<endl;
     for(int i=0;i<NumberOfPhyNode;i++)
     for(int j=i;j<NumberOfPhyNode;j++)
     if(Network[i][j]>0)
     {
     output<<i<<"--"<<j<<":";
     for(int l=0;l<SpectrumCapacity;l++)
     output<<PLink[i][j].spectrum[l];
     output<<endl;
     }
     output<<"*******************************************************"<<endl;
     */
	
	
    
	return 0;
}

int ReserveBackupNodeResource(int d)
{
	// reserve node resources
    for (int i = 0; i < VirtualRequest[d].numofBackup; i++) {
	PNode[VirtualRequest[d].BNode[i].map].compute = PNode[VirtualRequest[d].BNode[i].map].compute - VirtualRequest[d].BNode[i].compute;
	PNode[VirtualRequest[d].BNode[i].map].storage = PNode[VirtualRequest[d].BNode[i].map].storage - VirtualRequest[d].BNode[i].storage;
	PNode[VirtualRequest[d].BNode[i].map].memory = PNode[VirtualRequest[d].BNode[i].map].memory - VirtualRequest[d].BNode[i].memory;
    
	// share the backup node resources if their primary are disjoint
//	for(int i=0;i<d;i++)
//	{
//		int flag = 1;
//        
//		// check the node overlapping
//		for(int j=0;j<VirtualRequest[i].numofvn;j++)
//			for(int k=0;k<VirtualRequest[d].numofvn;k++)
//				if(VirtualRequest[i].VNode[j].map==VirtualRequest[d].VNode[k].map)
//				{
//					flag = 0;
//					break;
//				}
//        
//		// check the link overlapping
//		for(int j=0;j<VirtualRequest[i].numofvl;j++)
//			for(int m=0;m<VirtualRequest[i].VLink[j].hop[0]-1;m++)
//				for(int k=0;k<VirtualRequest[d].numofvl;k++)
//					for(int n=0;n<VirtualRequest[d].VLink[k].hop[0]-1;n++)
//						if(((VirtualRequest[i].VLink[j].route[0][m]==VirtualRequest[d].VLink[k].route[0][n])&&(VirtualRequest[i].VLink[j].route[0][m+1]==VirtualRequest[d].VLink[k].route[0][n+1]))||((VirtualRequest[i].VLink[j].route[0][m]==VirtualRequest[d].VLink[k].route[0][n+1])&&(VirtualRequest[i].VLink[j].route[0][m+1]==VirtualRequest[d].VLink[k].route[0][n])))
//						{
//							flag = 0;
//							break;
//						}
//		
//		// if disjoint, then share the node resources
//		if(flag)
//		{
//			PNode[VirtualRequest[d].BNode[i].map].compute = PNode[VirtualRequest[d].BNode[i].map].compute + VirtualRequest[i].BNode[i].compute;
//			PNode[VirtualRequest[d].BNode[i].map].storage = PNode[VirtualRequest[d].BNode[i].map].storage + VirtualRequest[i].BNode[i].storage;
//			PNode[VirtualRequest[d].BNode[i].map].memory = PNode[VirtualRequest[d].BNode[i].map].memory + VirtualRequest[i].BNode[i].memory;
//            
//			break;
//		}
//	}
    }
	
	return 0;
}

int ReleaseBackupNodeResource(int d)
{
	// release node resource
	for(int i=0;i<VirtualRequest[d].numofBackup;i++) {
		//if(VirtualRequest[d].BNode[i].map!=-1)
		//{
			PNode[VirtualRequest[d].BNode[i].map].compute = PNode[VirtualRequest[d].BNode[i].map].compute + VirtualRequest[d].BNode[i].compute;
			PNode[VirtualRequest[d].BNode[i].map].storage = PNode[VirtualRequest[d].BNode[i].map].storage + VirtualRequest[d].BNode[i].storage;
			PNode[VirtualRequest[d].BNode[i].map].memory = PNode[VirtualRequest[d].BNode[i].map].memory + VirtualRequest[d].BNode[i].memory;
		//}
    }
    
	return 0;
}

int BackupVirtualNodeMapping(int d)
{
	int success = 1;
    
	// sort the physical node in a descending order according to their available resources
	int rank[NumberOfPhyNode];
	int resource[NumberOfPhyNode];
    
	// initialization
	for(int i=0;i<NumberOfPhyNode;i++)
	{
		rank[i] = i;
		resource[i] = 0;
	}
    
	// calculate the available resource
	for(int i=0;i<NumberOfPhyNode;i++)
		resource[i] = PNode[i].compute + PNode[i].storage + PNode[i].memory;
    
	// make sure the node disjoint between primary and backup
	for(int i=0;i<NumberOfPhyNode;i++)
		for(int j=0;j<VirtualRequest[d].numofvn;j++)
		{
			if(VirtualRequest[d].VNode[j].map==i)
			{
				resource[i] = 0;
				break;
			}
		}
    
	// sort the physical node
	int temp = 0;
    
	for(int i=0;i<NumberOfPhyNode-1;i++)
		for(int j=i;j<NumberOfPhyNode;j++)
			if(resource[rank[j]]>resource[rank[i]])
			{
				temp = rank[i];
				rank[i] = rank[j];
				rank[j] = temp;
			}
    
	// virtual node mapping
	for(int i=0;i<VirtualRequest[d].numofvn;i++)
	{
		for(int j=0;j<NumberOfPhyNode;j++)
		{
			int flag = 1; // every physical node is a candidate
            
			// virtual nodes from the same VT cannot be mapped to the same physical node
			for(int k=0;k<i;k++)
			{
				if(VirtualRequest[d].BNode[k].map==rank[j])
				{
					flag = 0;
					break;
				}
			}
            
			// check if the physical node has enough available resources
			if(flag)
			{
				if((PNode[rank[j]].compute<VirtualRequest[d].BNode[i].compute)
                   ||(PNode[rank[j]].storage<VirtualRequest[d].BNode[i].storage)
                   ||(PNode[rank[j]].memory<VirtualRequest[d].BNode[i].memory))
				{
					flag = 0;
				}
			}
            
			// if the above two constraints are satisfied, premap the vn to the qualifed pn and reserve node resources
			if(flag)
			{
				VirtualRequest[d].BNode[i].map = rank[j];
				ReserveNodeResource(d, i);
				break;
			}
		}
	}
    
	// if any of the virtual nodes cannot be mapped, the request is rejected and release the reserved node resources
	for(int i=0;i<VirtualRequest[d].numofvn;i++)
		if(VirtualRequest[d].BNode[i].map==-1)
		{
			success = 0;
			ReleaseNodeResource(d);
			break;
		}
    
	VirtualRequest[d].success = success;
    
	return success;
}

void BackupRouting(int r, int l){
	// Added by Zilong
	int arr[NumberOfPhyNode][NumberOfPhyNode][K_PATH][NumberOfPhyNode];
	vector<int> K_routes[NumberOfPhyNode][NumberOfPhyNode][K_PATH];		//K-alternate routes
	int sournd = VirtualRequest[r].VNode[VirtualRequest[r].BLink[l].source].map;
	int destnd = VirtualRequest[r].BLink[l].destination;
    
	// save Network[][]
	int tempnet[NumberOfPhyNode][NumberOfPhyNode];
	for(int i=0;i<NumberOfPhyNode;i++)
		for(int j=0;j<NumberOfPhyNode;j++)
			tempnet[i][j] = Network[i][j];
    
	// ensure node disjoint of the primary and backup links
	for(int i=0;i<NumberOfPhyNode;i++)
		for(int j=0;j<NumberOfPhyNode;j++)
			for(int x=0;x<VirtualRequest[r].numofvl;x++)
				for(int y=0;y<VirtualRequest[r].VLink[x].hop[0]-1;y++)
				{
					if(((i==VirtualRequest[r].VLink[x].route[0][y])&&(i==VirtualRequest[r].VLink[x].route[0][y+1]))||((j==VirtualRequest[r].VLink[x].route[0][y])&&(i==VirtualRequest[r].VLink[x].route[0][y+1])))
						Network[i][j] = 0;
				}
    
    
	// Added by Zilong
    
	int no_path=0, k=0;
	struct indnode *q;
	for(q=&state[0];q<&state[NumberOfPhyNode];q++)
 	{
        q->pre= -1;
        q->hop = INF;
        q->label=tent;
 	}
	state[sournd].hop=0;
	state[sournd].label=perm;
	temp.insert(sournd);
	v_rt.insert(temp);
	temp.initialize();
	bool tag=false;
	k=sournd;
	do{
		for(q=&state[0];q<&state[NumberOfPhyNode];q++)
 		{
 			q->label=tent;
 		}
		temp = v_rt.extract();
		temp.inter=temp.head;
		while(temp.inter!=NULL){
			state[temp.inter->node].label=perm;
			temp.inter=temp.inter->next;
		}
		route dummy;
		dummy.copy(temp);
		k = temp.extract();
		tag=true;
		for(int i=0;i<NumberOfPhyNode;i++){
			route temp1;
			temp1.copy(dummy);
 			if(Network[k][i]!=0 && state[i].label==tent){
                
				temp1.insert(i);
				if( i!=destnd){
					v_rt.insert(temp1);
					tag=true;
 		        }
				else{
					final.insert(temp1);
					no_path++;
					tag=true;
				}
			}
			else{
				temp1.destroy();
                
			}
		}
		dummy.destroy();
		temp.destroy();
	}while(no_path!=K_PATH && !v_rt.isempty() );
    
	vector<int> v[K_PATH];
	temp.initialize();
	no_path=0;
	while(!final.isempty()){
		temp = final.extract();
		temp.inter = temp.head;
		while(temp.inter!=NULL){
			v[no_path].push_back(temp.inter->node);
			temp.inter = temp.inter->next;
		}
		no_path++;
	}
    
	for(int j=0;j<K_PATH;j++){
		for(int i=v[j].size()-1; i>=0; i--){
			K_routes[sournd][destnd][j].push_back(v[j][i]);
		}
		v[j].clear();
	}
    
    
	// Added by Zilong
	for(int i=0;i<NumberOfPhyNode;i++)
		for(int j=0;j<NumberOfPhyNode;j++)
			for(int k=0;k<K_PATH;k++)
				for(int l=0;l<NumberOfPhyNode;l++)
					arr[i][j][k][l] = -1;
    
	for(int k=0;k<K_PATH;k++)
		std::copy(K_routes[sournd][destnd][k].begin(), K_routes[sournd][destnd][k].end(), arr[sournd][destnd][k]);
    
	for(int k=0;k<K_PATH;k++)
		for(int j=0;j<NumberOfPhyNode;j++)
		{
			if(arr[sournd][destnd][k][j]!=-1)
			{
				VirtualRequest[r].BLink[l].route[k][j] = arr[sournd][destnd][k][j];
				VirtualRequest[r].BLink[l].hop[k] = VirtualRequest[r].BLink[l].hop[k] + 1;
			}
		}
    
	// restore the PN and PL
	for(int i=0;i<NumberOfPhyNode;i++)
		for(int j=0;j<NumberOfPhyNode;j++)
			Network[i][j] = tempnet[i][j];
	// Added by Zilong
    
	
	temp.destroy();
	v_rt.destroy();
	final.destroy();
	v_rt.initialize();
	final.initialize();
	temp.initialize();
}

int BackupModulation(int r, int l)
{
	for(int k=0;k<K_PATH;k++)
	{
		// calculate the path distance
		for(int j=0;j<VirtualRequest[r].BLink[l].hop[k]-1;j++)
		{
			VirtualRequest[r].BLink[l].distance[k] = VirtualRequest[r].BLink[l].distance[k] + PLink[VirtualRequest[r].BLink[l].route[k][j]][VirtualRequest[r].BLink[l].route[k][j+1]].distance;
		}
        
		// assign the modulation format
		if((VirtualRequest[r].BLink[l].distance[k]-1000)<=0)
			VirtualRequest[r].BLink[l].modulation[k] = QAM;
		else if(((VirtualRequest[r].BLink[l].distance[k]-1000)>0)&&((VirtualRequest[r].BLink[l].distance[k]-3000)<=0))
			VirtualRequest[r].BLink[l].modulation[k] = QPSK;
		else if(((VirtualRequest[r].BLink[l].distance[k]-3000)>0)&&((VirtualRequest[r].BLink[l].distance[k]-8000)<=0))
			VirtualRequest[r].BLink[l].modulation[k] = BPSK;
		else
			VirtualRequest[r].BLink[l].modulation[k] = 1;
        
		// calculate the required spectrum width
		VirtualRequest[r].BLink[l].width[k] = ceil( (VirtualRequest[r].BLink[l].bandwidth/VirtualRequest[r].BLink[l].modulation[k])/Slot);
	}
	
	return 0;
}

int BackupSpectrumAllocation(int r, int l)
{
    int success = 1;
	double maxprobability = 0;
    
	for(int k=0;k<K_PATH;k++)
	{
		int flag = 1; // flag for the lowest start spectrum slot
        
		// share the backup networking resources if their primary are disjoint
//		for(int i=0;i<r;i++)
//		{
//			int flag = 1;
//            
//			// check the node overlapping
//			for(int j=0;j<VirtualRequest[i].numofvn;j++)
//				for(int kk=0;kk<VirtualRequest[r].numofvn;kk++)
//					if(VirtualRequest[i].VNode[j].map==VirtualRequest[r].VNode[kk].map)
//					{
//						flag = 0;
//						break;
//					}
//            
//			// check the link overlapping
//			for(int j=0;j<VirtualRequest[i].numofvl;j++)
//				for(int m=0;m<VirtualRequest[i].VLink[j].hop[0]-1;m++)
//					for(int kk=0;kk<VirtualRequest[r].numofvl;kk++)
//						for(int n=0;n<VirtualRequest[r].VLink[k].hop[0]-1;n++)
//							if(((VirtualRequest[i].VLink[j].route[0][m]==VirtualRequest[r].VLink[kk].route[0][n])&&(VirtualRequest[i].VLink[j].route[0][m+1]==VirtualRequest[r].VLink[kk].route[0][n+1]))||((VirtualRequest[i].VLink[j].route[0][m]==VirtualRequest[r].VLink[kk].route[0][n+1])&&(VirtualRequest[i].VLink[j].route[0][m+1]==VirtualRequest[r].VLink[kk].route[0][n])))
//							{
//								flag = 0;
//								break;
//							}
//            
//			// if disjoint, then share the networking resources
//			if(flag)
//			{
//				for(int j=0;j<VirtualRequest[i].numofvl;j++)
//					for(int m=0;m<VirtualRequest[i].VLink[j].hop[0]-1;m++)
//						for(int s=VirtualRequest[i].VLink[m].start[VirtualRequest[i].VLink[m].map];s<VirtualRequest[i].VLink[m].start[VirtualRequest[i].VLink[m].map]+VirtualRequest[i].VLink[m].width[VirtualRequest[i].VLink[m].map];s++)
//							PLink[VirtualRequest[i].VLink[j].route[0][m]][VirtualRequest[i].VLink[j].route[0][m+1]].spectrum[s] = 1;
//			}
//		}
        
		// find the bitmap along the path
		int bitmap[SpectrumCapacity];
        
		// initialize bitmap
		for(int i=0;i<SpectrumCapacity;i++)
			bitmap[i] = 0;
        
		// find the bitmap
		for(int i=0;i<SpectrumCapacity;i++)
			for(int j=0;j<VirtualRequest[r].BLink[l].hop[k]-1;j++)
			{
				if(PLink[VirtualRequest[r].BLink[l].route[k][j]][VirtualRequest[r].BLink[l].route[k][j+1]].spectrum[i]==1)
					bitmap[i] = 1;
			}
        
		// calculate the link mapping probability
		for(int i=0;i<SpectrumCapacity;i++)
		{
			int availableslot = 0;
            
			for(int j=i;j<i+VirtualRequest[r].BLink[l].width[k];j++)
				availableslot = availableslot + bitmap[j];
            
			if(availableslot==0)
			{
				VirtualRequest[r].BLink[l].p[k] = VirtualRequest[r].BLink[l].p[k] + 1;
                
				if(flag)
				{
					VirtualRequest[r].BLink[l].start[k] = i;
					flag = 0;
				}
			}
		}
        
		VirtualRequest[r].BLink[l].p[k] = VirtualRequest[r].BLink[l].p[k]/(SpectrumCapacity-VirtualRequest[r].BLink[l].width[k]+1);
        
		// choose the path with the max link mapping probability
		if(VirtualRequest[r].BLink[l].p[k]>maxprobability)
		{
			VirtualRequest[r].BLink[l].map = k;
			maxprobability = VirtualRequest[r].BLink[l].p[k];
		}
	}
    
	if(maxprobability==0)
		success = 0;
    
	return success;
}

int ReserveBackupLinkResource(int d, int i)
{
	// reserve link resources
	for(int j=0;j<VirtualRequest[d].BLink[i].hop[VirtualRequest[d].BLink[i].map]-1;j++)
		for(int s=VirtualRequest[d].BLink[i].start[VirtualRequest[d].BLink[i].map];s<VirtualRequest[d].BLink[i].start[VirtualRequest[d].BLink[i].map]+VirtualRequest[d].BLink[i].width[VirtualRequest[d].BLink[i].map];s++)
		{
			PLink[VirtualRequest[d].BLink[i].route[VirtualRequest[d].BLink[i].map][j]][VirtualRequest[d].BLink[i].route[VirtualRequest[d].BLink[i].map][j+1]].spectrum[s] = 1;
			PLink[VirtualRequest[d].BLink[i].route[VirtualRequest[d].BLink[i].map][j+1]][VirtualRequest[d].BLink[i].route[VirtualRequest[d].BLink[i].map][j]].spectrum[s] = 1;
		}
    
	return 0;
}

int ReleaseBackupLinkResource(int d)
{
	// release link resources
	for(int i=0;i<VirtualRequest[d].numofBackuplink;i++)
		if(VirtualRequest[d].BLink[i].map!=-1)
		{
			for(int j=0;j<VirtualRequest[d].BLink[i].hop[VirtualRequest[d].BLink[i].map]-1;j++)
				for(int s=VirtualRequest[d].BLink[i].start[VirtualRequest[d].BLink[i].map];s<VirtualRequest[d].BLink[i].start[VirtualRequest[d].BLink[i].map]+VirtualRequest[d].BLink[i].width[VirtualRequest[d].BLink[i].map];s++)
				{
					PLink[VirtualRequest[d].BLink[i].route[VirtualRequest[d].BLink[i].map][j]][VirtualRequest[d].BLink[i].route[VirtualRequest[d].BLink[i].map][j+1]].spectrum[s] = 0;
					PLink[VirtualRequest[d].BLink[i].route[VirtualRequest[d].BLink[i].map][j+1]][VirtualRequest[d].BLink[i].route[VirtualRequest[d].BLink[i].map][j]].spectrum[s] = 0;
				}
		}
    
	return 0;
}

int BackupVirtualLinkMapping(int d)
{
	int success = 1;
    
	for(int i=0;i<VirtualRequest[d].numofBackuplink;i++)
	{
		BackupRouting(d, i);
        
		BackupModulation(d, i);
        
		// if virtual link can be provisioned, reserve the according link resources
		if(BackupSpectrumAllocation(d, i))
			ReserveBackupLinkResource(d, i);
        
		// if virtual link CANNOT be provisioned, release the reserved node AND link resource
		else if(BackupSpectrumAllocation(d, i)==0)
		{
			success = 0;
			ReleaseBackupNodeResource(d);
			ReleaseBackupLinkResource(d);
			break;
		}
	}
    
	VirtualRequest[d].success = success;
	
	return success;
}

//double CalculateReliability(int i)
//{
//	double ap = 1;
//	double ab = 1;
//	double abj[NumberOfRequest];
//	double phi = 0;
//    
//	for(int j=0;j<i;j++)
//		abj[j] = 1;
//    
//	// calculate the primary availability
//	for(int j=0;j<VirtualRequest[i].numofvn;j++)
//		ap = ap * PNode[VirtualRequest[i].VNode[j].map].availability;
//    
//	for(int j=0;j<VirtualRequest[i].numofvl;j++)
//		for(int k=0;k<VirtualRequest[i].VLink[j].hop[0]-1;k++)
//			ap = ap * PLink[VirtualRequest[i].VLink[j].route[0][k]][VirtualRequest[i].VLink[j].route[0][k+1]].availability;
//    
//	// calculate the backup availability
//	for(int j=0;j<VirtualRequest[i].numofvn;j++)
//		ab = ab * PNode[VirtualRequest[i].BNode[j].map].availability;
//    
//	for(int j=0;j<VirtualRequest[i].numofvl;j++)
//		for(int k=0;k<VirtualRequest[i].BLink[j].hop[0]-1;k++)
//			ab = ab * PLink[VirtualRequest[i].BLink[j].route[0][k]][VirtualRequest[i].BLink[j].route[0][k+1]].availability;
//    
//	// calculate the probability to use the backup resources
//	for(int m=0;m<i;m++)
//		for(int j=0;j<VirtualRequest[i].numofvl;j++)
//			for(int k=0;k<VirtualRequest[i].BLink[j].hop[0]-1;k++)
//				abj[m] = abj[m] * PLink[VirtualRequest[m].BLink[j].route[0][k]][VirtualRequest[m].BLink[j].route[0][k+1]].availability;
//    
//	// determine phi
//	for(int m=0;m<i;m++)
//		phi = phi + (abj[m]*(1-abj[m]))/(abj[m]+ab);
//    
//	phi = 1 - phi;
//    
//	VirtualRequest[i].reliability = ap + (1 - ap) * ab * phi;
//    
//	return 0;
//}

int main()
{
    
	output.open("cnlb-output");
	if(output.fail())
	{
		cout<<"Output Error";
		exit(1);
	}
    
	fbdata.open("cnlb-firstblock.xls");
	if(fbdata.fail())
	{
		cout<<"First-block Error";
		exit(1);
	}
    
	LoadTopology();
    
	double nodereject[NumberOfTrial];
	double linkreject[NumberOfTrial];
	double accept[NumberOfTrial];
    
	double nrr = 0; // node reject ratio
	double lrr = 0; // link reject ratio
	double acceptratio = 0;
	double block = 0;
    int bb = 0;
    int bb1 = 0;
    int bb2 = 0;
    int bb3 = 0;
    int bl = 0;
    
    
	for(int t=0;t<NumberOfTrial;t++)
	{
		nodereject[t] = 0;
		linkreject[t] = 0;
		accept[t] = 0;
	}
    
	for(int t=0;t<NumberOfTrial;t++)
	{
        srand(time(0));
		Initialization();
		TrafficGenerator();
		
		int flag = 1;
        int request = 0;
        
        
		for(int i=0;i<NumberOfRequest;i++)
		{
            //usleep(1000000);
            srand(time(0));
            //if (request%100 == 0)
            cout<<request<<endl;
            request++;
            
			if(VirtualNodeMapping(i))
			{
				if(VirtualLinkMapping(i)) {
					accept[t] = accept[t] + 1;
                    for (int p = 0; p < VirtualRequest[i].numofBackup; p++) {
                        bb1 = bb1 + VirtualRequest[i].BNode[p].compute;
                        bb2 = bb2 + VirtualRequest[i].BNode[p].storage;
                        bb3 = bb3 + VirtualRequest[i].BNode[p].memory;
                        
                    }
                    bl = bl + VirtualRequest[i].numofBackuplink;
                    bb = bb + VirtualRequest[i].numofBackup;
                    //cout<<"s"<<endl;
                }
                
				else
				{
					linkreject[t] = linkreject[t] + 1;
                    cout<<"link reject"<<endl;
                    
					if(flag)
					{
						fbdata<<i<<endl;
						flag = 0;
					}
				}
			}
			else
			{
				nodereject[t] = nodereject[t] + 1;
                cout<<"node reject"<<endl;
                
				if(flag)
				{
					fbdata<<i<<endl;
					flag = 0;
				}
			}
            
			//BackupVirtualNodeMapping(i);
            
			//BackupVirtualLinkMapping(i);
			
			DisplayPhysicalResource(i);
            
			//CalculateReliability(i);
		}
        
		nodereject[t] = nodereject[t]/NumberOfRequest;
		linkreject[t] = linkreject[t]/NumberOfRequest;
		accept[t] = accept[t]/NumberOfRequest;
        
		nrr = nrr + nodereject[t];
		lrr = lrr + linkreject[t];
		acceptratio = acceptratio + accept[t];
		block = block + 1 - accept[t];
	}
    
	nrr = nrr/NumberOfTrial;
	lrr = lrr/NumberOfTrial;
	acceptratio = acceptratio/NumberOfTrial;
	block = block/NumberOfTrial;
    
	cout<<"The accept ratio is "<<acceptratio<<endl;
	cout<<"The block ratio is "<<block<<endl;
	cout<<"The node reject ratio is "<<nrr<<endl;
	cout<<"The link reject ratio is "<<lrr<<endl;
    cout<<"backup used is "<<bb<<endl;
    cout<<"compute used is"<<bb1<<endl;
    cout<<"storage backup used is"<<bb2<<endl;
    cout<<"memory backup used is"<<bb3<<endl;
    cout<<"link backup used is"<<bl<<endl;
}



//the following functions are used for finding K-alternate routes
route::route(){
	head=0;
	tail=0;
	inter=0;
    
}
void route::initialize(){
	head=0;
	tail=0;
	inter=0;
}
void route::destroy(void){
	path *temp_del;
	while(head!=NULL){
		temp_del = head;
		head = head->next;
		temp_del->next=0;
		delete temp_del;
	}
	tail = NULL;
	inter = NULL;
}
void route::copy (const route& rtemp){
	path  *newnode;
	path *current;
	if(head!=NULL){
		destroy();
	}
	if(rtemp.head == NULL){
		head = NULL;
		tail = NULL;
		inter = NULL;
	}
	else{
		current = rtemp.head;
		head = new path;
		assert(head != NULL);
		head->node = current->node;
		head->next = NULL;
		tail = head;
		current = current->next;
		while (current!=NULL){
			newnode = new path;
			assert(newnode != NULL);
			newnode->node = current->node;
			newnode->next = NULL;
			tail->next = newnode;
			tail = newnode;
			current = current->next;
		}
	}
	
}
void route::insert(int x){
	path *newnode;
	newnode = new path;
	assert(newnode!=NULL);
	newnode->node = x;
	newnode->next = head;
	head = newnode;
}
void route::display(void){
	path *newnode;
	newnode = head;
	while(newnode!=NULL){
		cout<<newnode->node<<" ";
		newnode = newnode->next; 
	}
	cout<<endl;
}
int route::extract(){
	path *newnode;
	newnode = head;
	assert(newnode!=NULL);
	return newnode->node;
}
void vector_route::destroy(void){
	path *del;
	copies *temp_del;
	while(first!=NULL){
		temp_del = first;
		while(temp_del->dtemp.head!=NULL){
			del=temp_del->dtemp.head;
			temp_del->dtemp.head = temp_del->dtemp.head->next;
			del->next=0;
			delete del;
		}
        
        
		first = first->next;
		temp_del->next=0;
		delete temp_del;
	}
	last = NULL;
	middle = NULL;
}
route vector_route::extract(){
	copies *tempnode;
	tempnode = first;
	assert(first!=NULL);
	first = first->next;
	tempnode->next = NULL;
	route obj = tempnode->dtemp;
	delete tempnode;
	count--;
	return obj;
}
vector_route::vector_route(){
	first=0;
	last=0;
	middle=0;
	count=0;
}
void vector_route::initialize(){
	first=0;
	last=0;
	middle=0;
	count=0;
}
void vector_route::insert(route r){
	copies *tempnode;
	tempnode = new copies;
	tempnode->dtemp = r;
	tempnode->next=NULL;
	if(first==NULL){
		first=tempnode;
		last=tempnode;
	}
	else{
		last->next = tempnode;
		last = tempnode;
	}
	count++;
}
void vector_route::display(){
	copies *tempnode;
	tempnode = first;
	while(tempnode!=NULL){
		tempnode->dtemp.display();
		tempnode = tempnode->next; 
	}
	cout<<"----------------------------"<<endl;
}
bool vector_route::isempty(){
	if(count==0){
		return true;
	}
	else{
		return false;
	}
}

