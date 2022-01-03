// Author: Sumanyu Muku (smuku@nyu.edu)
#include <stdio.h>
#include <ctype.h>
#include<stdlib.h>
#include<iostream>
#include<cstring>
#include <utility>
#include<vector>
#include <fstream>
#include<regex>
#include <map>
#include <sstream>
#include<iomanip>
#include<cmath>
#include <algorithm>
#include<set>
#include <unistd.h>
#include<queue>
#include<deque>
#include <limits.h>
#define MAX_VPAGES 64

using namespace std;

// Pointers for keeping track of some variables in the simulation class.
int* actual_size=NULL;
unsigned long int* instPointer= new unsigned long int(0);

// Few Global Variables
int* randvals=NULL;
int ofs=0;
int randSize=0;

void initRandom(char* fname){
    ifstream inp (fname);
    string line;
    int arr_size;
    if(inp.is_open()){
        getline(inp, line);
        arr_size=stoi(line);
        randSize=arr_size;
    }

    randvals= new int[arr_size];
    int i=0;
    while(getline(inp, line)){
        int num=stoi(line);
        randvals[i]=num;
        i++;
    }
    inp.close();

};

// Random function using the rfile which will be used in the Random Pager.
int myrandom(int burst){ 
    int randomVal=randvals[ofs] % burst;
    ofs=(ofs+1) % randSize;
    return randomVal;
}

// Argument Class for storing the arguments which is used to initialize
// different objects and is passed to the simulation class.
class ArgsCollector{
    public:
        bool oohnoo;
        bool pagetable;
        bool frametable;
        bool summary;
        bool x;
        bool y;
        bool f;
        bool ageing;
        string inputFile;
        string randFile;
        string algo;
        string num_frames;
        string options;

        ArgsCollector(){
            oohnoo=false;
            pagetable=false;
            frametable=false;
            summary=false;
            x=false;
            y=false;
            f=false;
            ageing=false;
            num_frames="";
            inputFile="";
            randFile="";
            algo="f";
            options="";
        };
        void printArgs(){
            cout<<"Ooh Nooh: "<<oohnoo<<endl;
            cout<<"Page Table: "<<pagetable<<endl;
            cout<<"Frame Table: "<<frametable<<endl;
            cout<<"Summary: "<<summary<<endl;
            cout<<"Algo: "<<algo<<endl;
            cout<<"Input file path: "<<inputFile<<endl;
            cout<<"Rand file path: "<<randFile<<endl;
        }
};

// Struct for each frame table entry
typedef struct 
{
    int frameID;
    int pID;
    int pageNo;
    unsigned long int duration;
    unsigned long int t;
    int victimized;

} frame_t;

// Struct for each page table entry
typedef struct
{
    unsigned int present:1;
    unsigned int referenced:1;
    unsigned int modified:1;
    unsigned int write_protect:1;
    unsigned int pagedout:1;
    unsigned int framenum:7;
    unsigned int fmapped:1;

} pte_t;

enum pte_entry{PRESENT, REFERENCED, MODIFIED, WRITE_PROTECT, PAGEDOUT, FRAMENUM, FILE_MAPPED};
enum frame_entry{FRAMEID, PROCESSID, PAGE, AGE, TAU, VICTIM};

// Function as mentioned in Lab PDF for updating the PTE.
void updatePTE(pte_t* p, pte_entry entry, int val){
    if(entry==PRESENT)
        p->present=val;
    else if(entry==REFERENCED)
        p->referenced=val;
    else if(entry==MODIFIED)
        p->modified=val;
    else if(entry==WRITE_PROTECT)
        p->write_protect=val;
    else if(entry==PAGEDOUT)
        p->pagedout=val;
    else if(entry==FRAMENUM)
        p->framenum=val;
    else if(entry==FILE_MAPPED)
        p->fmapped=val;
    else{
        printf("Invalid Entry\n");
        exit(EXIT_FAILURE);
    }
};

// Function for updating frame
void updateFTE(frame_t* f, frame_entry entry, unsigned long int val){
    if(entry==FRAMEID)
        f->frameID=val;
    else if(entry==PROCESSID)
        f->pID=val;
    else if(entry==PAGE)
        f->pageNo=val;
    else if(entry==AGE)
        f->duration=val;
    else if(entry==TAU)
        f->t=val;
    else if(entry==VICTIM)
        f->victimized=val;
    else{
        printf("Invalid Entry\n");
        exit(EXIT_FAILURE);
    }
};

// Global frame table pointer, will be initialized
// at runtime.
frame_t* global_frame_table=NULL;
int MAX_FRAMES=128; // default value ; would override it

// VMA class for storing the entries for each process.
class VMA{
    public:
        int start_page;
        int end_page;
        int write_protected;
        int file_mapped;

        VMA(int s, int e, int wp, int fm){
            start_page=s;
            end_page=e;
            write_protected=wp;
            file_mapped=fm;
        };
};

// Process class for storing page table, vma array
// and other variable for reporting process level
// stats
class Process{
    public:
        int pID;
        vector<VMA> vma_array;
        pte_t page_table[MAX_VPAGES];
        unsigned long int maps;
        unsigned long int unmaps;
        unsigned long int pageins;
        unsigned long int pageouts;
        unsigned long int fins;
        unsigned long int fouts;
        unsigned long int zeros;
        unsigned long int segv;
        unsigned long int segprot;

        Process(int i){
            pID=i;
            for(int j=0; j<MAX_VPAGES; j++){
                page_table[j].present=0;
                page_table[j].referenced=0;
                page_table[j].modified=0;
                page_table[j].write_protect=0;
                page_table[j].pagedout=0;
                page_table[j].framenum=0;
                page_table[j].fmapped=0;
            }

            maps=unmaps=pageins=pageouts=fins=fouts=zeros=segv=segprot=0;

        };

        void printProcess(){
            cout<<"Created Process "<<pID<<endl;
            cout<<"VMAs: "<<endl;
            for(int i=0; i<vma_array.size(); i++){
                cout<<vma_array[i].start_page<<" "<<vma_array[i].end_page<<" "<<vma_array[i].write_protected<<" "<<vma_array[i].file_mapped<<endl;
            }
            cout<<"##### Process Level Stats ########"<<endl;
            cout<<"Maps: "<<maps<<endl;
            cout<<"Unmaps: "<<unmaps<<endl;
            cout<<"Ins: "<<pageins<<endl;
            cout<<"Outs: "<<pageouts<<endl;
            cout<<"FIns: "<<fins<<endl;
            cout<<"FOuts: "<<fouts<<endl;
            cout<<"Zeros: "<<zeros<<endl;
            cout<<"Segv: "<<segv<<endl;
            cout<<"Segprot"<<segprot<<endl;
        }
};

// Global Process List for keeping for stroing all processes
vector<Process*> processList;
// Process Pointer for keeping track of the current process.
Process* currentProcess=NULL;

// Virtual Pager class which will be inherited by all the Paging Algorithms.
class Pager{
    public:
        int hand;
        virtual frame_t* select_victim_frame() = 0;
        virtual void setAge(){};
};

// First In First Out Page Replacement Algorithm
class FIFO : public Pager{
    public:
        FIFO(){
            hand=0;
        }

        frame_t* select_victim_frame(){
            frame_t* v = &global_frame_table[hand];
            hand=(hand+1)%MAX_FRAMES;
            updateFTE(v, VICTIM, 1);
            return v;
        }

};

// Random Page Replacement algorithm
class Random : public Pager{
    public:
        Random(){};
        frame_t* select_victim_frame(){
            int c = *actual_size;
            int index = myrandom(c);
            frame_t* v = &global_frame_table[index];
            updateFTE(v, VICTIM, 1);
            return v;
        }
};

// Clock Page replacement Algorithm
class Clock : public Pager{
    public:
        Clock(){
            hand=0;
        };
        frame_t* select_victim_frame(){
            frame_t* v=NULL;
            frame_t* oldFrame = &global_frame_table[hand];

            pte_t* oldPage = &processList[oldFrame->pID]->page_table[oldFrame->pageNo];
            while(oldPage->referenced){
                updatePTE(oldPage, REFERENCED, 0);
                hand=(hand+1)%MAX_FRAMES;
                oldFrame=&global_frame_table[hand];
                oldPage=&processList[oldFrame->pID]->page_table[oldFrame->pageNo];
            }
            
            v=&global_frame_table[oldPage->framenum];
            updateFTE(v, VICTIM, 1);
            hand=(hand+1)%MAX_FRAMES;

            return v;
        };
};

// Not Recently Used Page Replacement Algorithm
class NRU : public Pager{
    public:
        unsigned long int lastaccess;
        bool* foundClass;
        int*  framesOfClassID;
        NRU(){
            hand=0;
            lastaccess=*instPointer;
            foundClass= new bool[4];
            fill_n(foundClass, 4, false);
            framesOfClassID = new int[4];
            fill_n(framesOfClassID, 4, -1);
        };
        frame_t* select_victim_frame(){
            frame_t* v=NULL;
            for(int i=0; i<4; i++){
                foundClass[i]=false;
                framesOfClassID[i]=-1;
            }

            for(int i=0; i<MAX_FRAMES; i++){
                int fid = (i+hand)%MAX_FRAMES;
                frame_t* oldFrame = &global_frame_table[fid];
                pte_t* oldPage = &processList[oldFrame->pID]->page_table[oldFrame->pageNo];
                int classID =  2*oldPage->referenced + oldPage->modified;

                if(*instPointer-lastaccess>=50)
                    updatePTE(oldPage, REFERENCED, 0);

                if(!foundClass[classID]){
                    foundClass[classID]=true;
                    framesOfClassID[classID]=fid;
                }

            }

            if(*instPointer-lastaccess>=50)
                lastaccess=*instPointer;

            for(int i=0; i<4; i++){
                if(foundClass[i]){
                    hand=(framesOfClassID[i]+1)%MAX_FRAMES;
                    v=&global_frame_table[framesOfClassID[i]];
                    updateFTE(v, VICTIM, 1);
                    break;
                }
            }

            return v;
            
        };
};

// Aging Page Replacement Algorithm
class Aging : public Pager{
    public:
        Aging(){
            hand=0;
        };
        frame_t* select_victim_frame(){
            frame_t* v=NULL;
            v=&global_frame_table[hand];
            frame_t* oldFrame=NULL;
            for(int i=0; i<MAX_FRAMES; i++){
                int fid = (i+hand)%MAX_FRAMES;
                oldFrame = &global_frame_table[fid];
                updateFTE(oldFrame, AGE, oldFrame->duration>>1);

                pte_t* oldPage = &processList[oldFrame->pID]->page_table[oldFrame->pageNo];
                if(oldPage->referenced){
                    updatePTE(oldPage, REFERENCED, 0);
                    updateFTE(oldFrame, AGE, oldFrame->duration|0x80000000);
                }

                if(oldFrame->duration<v->duration)
                    v=oldFrame;
            }

            updateFTE(v, VICTIM, 1);
            hand=(v->frameID + 1) % MAX_FRAMES;
            updateFTE(v, AGE, 0);
            return v;
        };
};

// Working Set Page Replacement Algorithm
class WorkingSet : public Pager{
    public:
        WorkingSet(){
            hand=0;
        };
        frame_t* select_victim_frame(){
            frame_t* v=NULL;
            unsigned long int smallestTime = ULONG_MAX;
            
            for(int i=0; i<MAX_FRAMES; i++){
                int fid=(i+hand)%MAX_FRAMES;
                frame_t* oldFrame = &global_frame_table[fid];
                pte_t* oldPage = &processList[oldFrame->pID]->page_table[oldFrame->pageNo];
                int timeElapsed = *instPointer - oldFrame->t;

                if(oldPage->referenced){
                    updateFTE(oldFrame, TAU, *instPointer);
                    updatePTE(oldPage, REFERENCED, 0);
                }else{
                    if(timeElapsed>=50){
                        v=oldFrame;
                        break;
                    }else if(oldFrame->t < smallestTime){
                        smallestTime=oldFrame->t;
                        v=oldFrame;
                    }
                }
            }
            if(!v)
                v=&global_frame_table[hand];
            
            updateFTE(v, VICTIM, 1);
            hand=(v->frameID+1)%MAX_FRAMES;
            return v;

        };
};


// Pager Class Pointer, will be
// initialized at run time to particular page replacement algorithm
// based on the option given.
Pager* THE_PAGER=NULL;

// Deque for maintaining the free frames
deque<frame_t*> freeFrames;

// Function as given in Lab PDF for returning the frame pointer from the free frame list
frame_t* allocate_frame_from_free_list(){
    frame_t* f=NULL;
    if(!freeFrames.empty()){
        f=freeFrames.front();
        freeFrames.pop_front();
    }
    return f;
};

// Function as described in the lab PDF for getting the frame.
frame_t *get_frame(){
    frame_t *frame = allocate_frame_from_free_list();
    if (frame == NULL) frame = THE_PAGER->select_victim_frame();
        return frame;
}

// Simulation class which contains the variables that will be used
// for summary and will run the simulation block.
class Simulation{
    public:
        char instr;
        int vpage;
        int numProcesses;
        int numVMAs;
        int sp;
        int ep;
        int wp;
        int fm;
        ArgsCollector* args;
        unsigned long long int cost;
        unsigned long int inst_count;
        unsigned long int ctx_switches;
        unsigned long int process_exits;
        ifstream fp;
        string line;
        
        // Constructor for initializing the arguments.
        Simulation(ArgsCollector* a){
            instr='0';
            line="";
            vpage=0;
            inst_count=ctx_switches=process_exits=0;
            numProcesses=numVMAs=sp=ep=wp=fm=0;
            cost=0;
            args=a;
            char arr[(a->inputFile).length()+1];
            strcpy(arr, (a->inputFile).c_str());
            fp.open(arr);
        }

        // Function as given in lab PDF for getting the next instruction from the input file.
        bool get_next_instruction(char& op, int& vpage){
            stringstream ss(line);
            if(ss>>op>>vpage){
                getline(fp, line);
                return true;
            }
            else
                return false;            
        };

        // Function for modifying referenced/modify bits post oage fault check.
        void modifyBits(){
            pte_t* pte = &currentProcess->page_table[vpage];
            if(!pte->referenced)
                updatePTE(pte, REFERENCED, 1);
            
            if(instr=='w'){
                if(pte->write_protect){
                    if(args->oohnoo)
                        printf(" SEGPROT\n");
                    currentProcess->segprot++;
                    cost+=420;
                }else
                    updatePTE(pte, MODIFIED, 1);
            }
        }
        
        void contextSwitch(){
            currentProcess=processList[vpage];
            ctx_switches++;
            cost+=130;
        };

        void processExit(pte_t* pte){
            printf("EXIT current process %d\n", vpage);
            for(int i=0; i<MAX_VPAGES; i++){
                pte=&currentProcess->page_table[i];
            if(pte->present){
                frame_t* oldFrame = &global_frame_table[pte->framenum];
                printf(" UNMAP %d:%d\n", oldFrame->pID, oldFrame->pageNo);

                updateFTE(oldFrame, TAU, 0);
                updateFTE(oldFrame, AGE, 0);
                updateFTE(oldFrame, VICTIM, 0);
                updateFTE(oldFrame, PAGE, -1);
                updateFTE(oldFrame, PROCESSID, -1);
                freeFrames.push_back(oldFrame);
                cost+=400;
                currentProcess->unmaps++;
                
                if(pte->modified && pte->fmapped){
                    cost+=2400;
                    printf(" FOUT\n");
                    currentProcess->fouts++;
                }
            }
            
            updatePTE(pte, PRESENT, 0);
            updatePTE(pte, PAGEDOUT, 0);
            updatePTE(pte, REFERENCED, 0);
        }
        process_exits++;
        cost+=1250;
        currentProcess=NULL;

        };

        int searchVMAs(){
            int index=-1;
            for(int i=0; i<currentProcess->vma_array.size(); i++){
                if(vpage>=currentProcess->vma_array[i].start_page && vpage<=currentProcess->vma_array[i].end_page){
                    index=i;
                    break;
                }
            }
            return index;
        };


        int pageFaultHandler(pte_t* pte){
            int valid=searchVMAs();
            
            if(valid!=-1){
                updatePTE(pte, FILE_MAPPED, currentProcess->vma_array[valid].file_mapped);
                updatePTE(pte, WRITE_PROTECT, currentProcess->vma_array[valid].write_protected);
            }else
            {
                if(args->oohnoo)
                    printf(" SEGV\n");
                cost+=340;
                currentProcess->segv++;
                return -1;
            }

            frame_t* allocatedFrame = get_frame();

            if(allocatedFrame->victimized==1){
                int oldProcessID = allocatedFrame->pID;
                int oldPageNo = allocatedFrame->pageNo;

                pte_t* oldPTE = &processList[oldProcessID]->page_table[oldPageNo];
                updatePTE(oldPTE, PRESENT, 0);
                processList[oldProcessID]->unmaps++;
                cost+=400;

                if(args->oohnoo)
                    printf(" UNMAP %d:%d\n", oldProcessID, oldPageNo);
                
                if(oldPTE->modified){
                    if(oldPTE->fmapped){
                        processList[oldProcessID]->fouts++;
                        if(args->oohnoo)
                            printf(" FOUT\n");

                        updatePTE(oldPTE, MODIFIED, 0);
                        cost+=2400;

                    }else{
                        processList[oldProcessID]->pageouts++;
                        if(args->oohnoo)
                            printf(" OUT\n");
                        
                        updatePTE(oldPTE, PAGEDOUT, 1);
                        updatePTE(oldPTE, MODIFIED, 0);
                        cost+=2700;
                    }
                }
                updatePTE(oldPTE, FRAMENUM, 0);
            }

            if(pte->fmapped){
                if(args->oohnoo)
                    printf(" FIN\n");

                currentProcess->fins++;
                cost+=2800;
            }else{
                if(pte->pagedout){
                    if(args->oohnoo)
                        printf(" IN\n");
                    cost+=3100;
                    currentProcess->pageins++;
                }else{
                    if(args->oohnoo)
                        printf(" ZERO\n");
                    cost+=140;
                    currentProcess->zeros++;
                }
            }

            updatePTE(pte, PRESENT, 1);
            updatePTE(pte, FRAMENUM, allocatedFrame->frameID);

            updateFTE(allocatedFrame, PROCESSID, currentProcess->pID);
            updateFTE(allocatedFrame, PAGE, vpage);
            updateFTE(allocatedFrame, TAU, inst_count);

            if(args->oohnoo)
                printf(" MAP %d\n", allocatedFrame->frameID);
            
            currentProcess->maps++;
            cost+=300;
            return 1;
        };


        void simulate(){
            getline(fp, line);
            while(line.at(0)=='#')
                getline(fp, line);
            
            // Reading Num of processes
            numProcesses=stoi(line);
            getline(fp, line);
            while(line.at(0)=='#')
                getline(fp, line);
            
            // Reading VMAs of processes 
            for(int i=0; i<numProcesses; i++){
                Process* p = new Process(i);
                numVMAs=stoi(line);
                getline(fp, line);
                for(int j=0; j<numVMAs; j++){
                    stringstream ss(line);
                    ss>>sp>>ep>>wp>>fm;
                    VMA* vmaobj = new VMA(sp, ep, wp, fm);
                    p->vma_array.push_back(*vmaobj);
                    getline(fp, line);
                    while(line.at(0)=='#')
                        getline(fp, line);
                    
                }
                // Adding Process to global process list.
                processList.push_back(p);
            }

            if(line.at(0)=='#')
                getline(fp, line);
            else{
                while(this->get_next_instruction(instr, vpage)){
                    if(args->oohnoo)
                        printf("%lu: ==> %c %d\n", inst_count, instr, vpage);

                    inst_count++;
                    *(instPointer)= *(instPointer)+1;
                    pte_t* pte=NULL;

                    switch(instr){
                        case 'c':
                            this->contextSwitch();
                            continue;
                        case 'e':
                            this->processExit(pte);
                            continue;
                        default:
                            pte = &currentProcess->page_table[vpage];
                            cost++;
                            break;

                    }

                    // Page Fault Check
                    if(!pte->present){
                        int x = this->pageFaultHandler(pte);
                        if(x==-1)
                            continue;
                    }

                    this->modifyBits();

                }
            }

            fp.close();
        };

        void printPT();
        void printFT();
        void printSummary();
        void describe();       

};

// For printing the page table
void Simulation::printPT(){
            for(int i=0; i<processList.size(); i++){
                printf("PT[%d]: ", i);
                Process* p = processList[i];
                for(int j=0; j<MAX_VPAGES; j++){
                    if(p->page_table[j].present){
                        printf("%d:", j);
                        if(p->page_table[j].referenced)
                            printf("R");
                        else
                            printf("-");

                        if(p->page_table[j].modified)
                            printf("M");
                        else
                            printf("-");

                        if(p->page_table[j].pagedout)
                            printf("S");
                        else
                            printf("-");
                    }else{
                        if(p->page_table[j].pagedout)
                            printf("#");
                        else
                            printf("*");
                    }

                    if(j==MAX_VPAGES-1)
                        printf("");
                    else
                        printf(" ");
                }
                cout<<"\n";
            }

};

// For printing the frame table
void Simulation::printFT(){
            int c = *actual_size;
            printf("FT: ");
            for(int i=0; i<*actual_size ; i++){
                if(global_frame_table[i].pID!=-1)
                    printf("%d:%d", global_frame_table[i].pID, global_frame_table[i].pageNo);
                else
                    printf("*");

                if(i==c-1)
                    printf("");
                else
                    printf(" ");
            }
            printf("\n");

};

// For print summary stats
void Simulation::printSummary(){
    for(int i=0; i<processList.size(); i++){
        Process* p = processList[i];

        printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n", p->pID, p->unmaps, p->maps, p->pageins, p->pageouts,
                p->fins, p->fouts, p->zeros, p->segv, p->segprot);
    }

    printf("TOTALCOST %lu %lu %lu %llu %lu\n", this->inst_count, this->ctx_switches, this->process_exits, this->cost, sizeof(pte_t));

};

// Culmination of all options
void Simulation::describe(){
    if(this->args->pagetable)
        this->printPT();

    if(this->args->frametable)
        this->printFT();

    if(this->args->summary)
        this->printSummary();

};

ArgsCollector* read(int argc, char* argv[]){
    ArgsCollector* args = new ArgsCollector();
    opterr = 0;
    int index=0;
    int c=0;

    while((c=getopt(argc, argv, "a:o:f:"))!=-1){
        switch(c){
            case 'f':
                args->num_frames=optarg;
                break;
            case 'a':
                args->algo = optarg;
                break;
            case 'o':
                args->options=optarg;
            default:
                break;
        }

        for(index=optind ; index<argc; index++){
            if(index==argc-2)
                args->inputFile=argv[index];
            else if(index==argc-1)
                args->randFile=argv[index];
        }

    }

    return args;

}

// Reinitializing certain pointers and running the simulation.
void run(ArgsCollector* args){
    // Initialized random array
    char randName[(args->randFile).length()+1];
    strcpy(randName, (args->randFile).c_str());
    initRandom(randName);
    string frame_string = args->num_frames;

    // overrid the MAX_FRAMES argument
    if(frame_string.length()>0)
        MAX_FRAMES=stoi(frame_string);

    actual_size=&MAX_FRAMES;
    // defined global frames table
    global_frame_table= new frame_t[MAX_FRAMES];


    // init frame_table and freeframes
    for(int i=0 ; i<MAX_FRAMES; i++){
        // frame_t ft = global_frame_table[i];
        global_frame_table[i].frameID=i;
        global_frame_table[i].pID=-1;
        global_frame_table[i].pageNo=-1;
        global_frame_table[i].victimized=0;
        global_frame_table[i].duration=0;
        global_frame_table[i].t=0;
        frame_t* fp = &global_frame_table[i];
        freeFrames.push_back(fp);
    }

    string opt = args->options;
    if(opt.find("O")!=string::npos)
        args->oohnoo=true;

    if(opt.find("P")!=string::npos)
        args->pagetable=true;

    if(opt.find("F")!=string::npos)
        args->frametable=true;
    
    if(opt.find("S")!=string::npos)
        args->summary=true;
    
    if(opt.find("x")!=string::npos)
        args->x=true;

    if(opt.find("y")!=string::npos)
        args->y=true;

    if(opt.find("f")!=string::npos)
        args->f=true;

    if(opt.find("a")!=string::npos)
        args->ageing=true;

    if(args->algo=="F" || args->algo=="f")
        THE_PAGER= new FIFO();
    else if(args->algo=="R" || args->algo=="r")
        THE_PAGER= new Random();
    else if(args->algo=="c" || args->algo=="C")
        THE_PAGER = new Clock();
    else if(args->algo=="e" || args->algo=="E")
        THE_PAGER = new NRU();
    else if(args->algo=="A" || args->algo=="a")
        THE_PAGER = new Aging();
    else if(args->algo=="w" || args->algo=="W")
        THE_PAGER = new WorkingSet();
    else{
        cout<<"Not implemented";
        exit(EXIT_FAILURE);
        }

    Simulation* simulator = new Simulation(args);
    simulator->simulate();
    simulator->describe();

};


// Main function
int main(int argc, char* argv[]){

    ArgsCollector* args = read(argc, argv);
    run(args);

    return 0;

}