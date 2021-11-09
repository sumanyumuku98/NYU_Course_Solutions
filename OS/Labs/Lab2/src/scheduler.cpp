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

using namespace std;

typedef pair<int,int> loff;

/*
This is Lab1 code for Parse Error
*/
void _parseError(int line, int offset, int errorCode){
    const char* errors[] = {
        "NUM_EXPECTED",
        "SYM_EXPECTED",
        "ADDR_EXPECTED",
        "SYM_TOO_LONG",
        "TOO_MANY_DEF_IN_MODULE",
        "TOO_MANY_USE_IN_MODULE",
        "TOO_MANY_INSTR",
    };
    printf("Parse Error line %d offset %d: %s\n", line, offset, errors[errorCode]);
    exit(-1);
}


//Tokenizer class
class Tokenizer{
    public:
    int offset;
    int finalLine;
    vector< pair<loff, string> > container;
        Tokenizer(){};
        Tokenizer(string filename){
            offset=1;
            finalLine=0;
            getToken(filename);
        };
        
        // Function for getting tokens from file stream
        void getToken(string filename){
            ifstream infile(filename);
            int lineCounter=0;
            string line;
            while(getline(infile, line)){
                lineCounter+=1;

                if(line.length()!=0){
                    offset=line.length()+1;
                    if(line[line.length()-1]==' ')
                        offset-=1;
                    int offsetCounter=0;
                    char offsetArr[line.length()+1];
                    strcpy(offsetArr, line.c_str());
                    char* toks=strtok(offsetArr, " \t\r\f\v");
                    while(toks!=NULL){
                        string toks_str(toks);
                        int found=line.find(toks_str, offsetCounter);
                        container.push_back(make_pair(make_pair(lineCounter, found+1), toks_str));
                        toks=strtok(NULL, " \t\r\f\v");
                        offsetCounter=found+toks_str.length();
                    }
                }else
                    offset=1;
            }
            finalLine=lineCounter;
            infile.close();
        };
        //Function for reading integer token.
        int readInt(vector<pair<loff, string> >::iterator &ftr, vector<pair<loff, string> >::iterator &btr, int finalLine, int offset){
            string s;
            loff l;
            if(ftr==btr){
                _parseError(finalLine, offset, 0);

            }else{
                l=ftr->first;
                s=ftr->second;
            }
            int x;
            bool truth=true;
            for (char const &c : s){
                if (isdigit(c) == 0){
                    truth=false;
                    break;
                    }
                }

            if(truth){
                x=stoi(s);
                
                if(x>pow(2,30)) _parseError(l.first, l.second, 0);

                return x;
            }else{
                _parseError(l.first, l.second, 0);
            }
        };
        
        //Function for reading symbol token.
        string readSymbol(vector<pair<loff, string> >::iterator &ftr, vector<pair<loff, string> >::iterator &btr, int finalLine, int offset){
            string s;
            loff l;
            if(ftr==btr){
                _parseError(finalLine, offset, 1);

            }else{
                l=ftr->first;
                s=ftr->second;
            }


            string s1 = s.substr(0,1);
            string s2 = s.substr(1);
            if(isalpha(s1) && isalnum(s2) && s.length()<=16){
                return s;
            }else{
                if(s.length()>16) 
                _parseError(l.first, l.second, 3);
                else
                _parseError(l.first, l.second, 1);

            }
        };

        // Function for reading instruction type
        char readIAER(vector<pair<loff, string> >::iterator &ftr, vector<pair<loff, string> >::iterator &btr, int finalLine, int offset){
            string s;
            loff l;
            if(ftr==btr){
                _parseError(finalLine, offset, 2);

            }else{
                l=ftr->first;
                s=ftr->second;
            }

            if(s.length()==1){
                char* arr = (char*) s.c_str();
                char x =*arr;
                if(x=='E' || x=='I' || x=='A' || x=='R')
                    return x;
                else
                    _parseError(l.first, l.second, 2);

            }else{
                cout<<s<<" "<<s.length()<<endl;
                cout<<l.first<<" "<<l.second<<endl;
                _parseError(l.first, l.second, 2);
            }
        };

        // Helper function for alpha chars
        bool isalpha(string s){
            return regex_match(s, regex("[a-zA-Z]"));
        };
        
        // Helper function for alpha numeric characters
        bool isalnum(string &s){
            return regex_match(s, regex("[a-zA-Z0-9]*"));
        };

        // Helper function for printing tokens with line and offset
        void printToken(){

            for(int i=0; i<container.size(); i++){
                cout<<"Token: "<<container[i].first.first<<":"<<container[i].first.second<<" : "<<container[i].second<<"\n";
            }
            cout<<"Final Spot in File : "<<"line="<<finalLine<<" offset="<<offset<<"\n";
        };

};

// Some global variables that I will be using for summarization.
int total_final_time=0;
int currTimeStamp=0;
double avg_wait_time=0.0;
double avg_trt=0.0;
double total_io_time=0.0;
double total_cpu_time=0.0;
double throughput=0.0;

int* randvals=NULL;
int ofs=0;
int randSize=0;

// Function for rfile initializaton.
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

int myrandom(int burst){ 
    int randomVal=1 + (randvals[ofs] % burst);
    ofs=(ofs+1) % randSize;
    return randomVal;
}

// States that will be used
enum processState {CREATED, READY, RUNNG, BLOCK, FINISHED, PREEMPTION};
string printStates[6] = {"CREATED", "READY", "RUNNG", "BLOCK", "DONE", "PREEMPT"};

// Function that I will be using to merge concurrent I/O Bursts
vector<vector<int>> ioTimeQueue;
vector<vector<int>> merge(vector<vector<int>>& intervals){
        sort(intervals.begin(), intervals.end());

        vector<vector<int>> merged;
        for (auto interval : intervals) {
            if (merged.empty() || merged.back()[1] < interval[0]) {
                merged.push_back(interval);
            }
            else {
                merged.back()[1] = max(merged.back()[1], interval[1]);
            }
        }
        return merged;

};

// Process Class. Defines different attributes for a process.
class Process{
    public:
        int pID;
        int arrivalTime;
        int totalCPUtime;
        int cpuBurst;
        int ioBurst;
        int staticPriority;
        int dynamicPriority;
        processState state;
        int stateTs;
        int ft;
        int trt;
        int io_time;
        int wait_time;
        int remCpuBurst;
        int randCpuBurst;

        Process(){};
        Process(int pid, int arrival, int total, int cpuB, int ioB, int staticP, int dynamicP, 
                processState currState){
            pID=pid;
            arrivalTime=arrival;
            totalCPUtime=total;
            cpuBurst=cpuB;
            ioBurst=ioB;
            staticPriority=staticP;
            dynamicPriority=dynamicP;
            state=currState;
            remCpuBurst=total;
            ft=0;
            trt=0;
            io_time=0;
            wait_time=0;
            stateTs=arrival;
            randCpuBurst=0;
        }
};

// Global Process List
vector<Process*> processList;

// Args Class that will be used to store the read cmd line arguments.
class ArgsCollector{
    public:
        bool verbose;
        bool eventTrace;
        bool runQueueTrace;
        bool preemptTrace;
        string inputFile;
        string randFile;
        string algo;

        ArgsCollector(){
            verbose=false;
            eventTrace=false;
            runQueueTrace=false;
            preemptTrace=false;
            inputFile="";
            randFile="";
            algo="F";
        };
        void printArgs(){
            cout<<"Verbose: "<<verbose<<endl;
            cout<<"Event Trace: "<<eventTrace<<endl;
            cout<<"runQueueTrace: "<<runQueueTrace<<endl;
            cout<<"PreemptTrace: "<<preemptTrace<<endl;
            cout<<"Algo: "<<algo<<endl;
            cout<<"Input file path: "<<inputFile<<endl;
            cout<<"Rand file path: "<<randFile<<endl;
        }
};

// Event class
class Event{
    public:
        int pID;
        int timeStamp;
        processState oldState;
        processState nextState;
        int endTime;
        Event(){};
        Event(int pid, int ts, processState old, processState newS, int end){
            pID=pid;
            timeStamp=ts;
            oldState=old;
            nextState=newS;
            endTime=end;
        }
};

// Abstract Scheduler class that will be inherited by each scheduler algorithm.
class Scheduler{
    public:
        deque<Process*> run_queue;
        int quantum;
        int maxPriority;
        virtual void add_process(Process* p){};
        virtual Process* get_next_process(){};
        virtual void insertExpiredProcess(Process* p){};
        virtual bool test_preempt(deque<Event*> event_queue, Process* run, int curr){};
        virtual string getSchedulerName(){};
        virtual Process* peek(){};
        virtual void printSize(){};

};

// FCFS Algorithm
class FCFS : public Scheduler{
    public:
        FCFS(){};
        FCFS(int q, int max){
            quantum=q;
            maxPriority=max;
        }
        Process* get_next_process(){
            Process* p=NULL;
            if(!run_queue.empty()){
                p = run_queue.front();
                run_queue.pop_front();
            }
            return p;
        }

        void add_process(Process* p){
            run_queue.push_back(p);
        }

        void insertExpiredProcess(Process* p){
            this->add_process(p);
        }


        string getSchedulerName(){ return "FCFS";}

        bool test_preempt(deque<Event*> event_queue, Process* run, int curr){ return false; }


};

// LCFS Algorithm
class LCFS: public Scheduler{
    public:
        LCFS(){};
        LCFS(int q, int max){
            quantum=q;
            maxPriority=max;
        }
        Process* get_next_process(){
            Process* p=NULL;
            if(!run_queue.empty()){
                p=run_queue.back();
                run_queue.pop_back();
            }

            return p;
        }
        void add_process(Process* p){
            run_queue.push_back(p);
        }

        void insertExpiredProcess(Process* p){
            this->add_process(p);
        }


        string getSchedulerName(){return "LCFS";}
        bool test_preempt(deque<Event*> event_queue, Process* run, int curr){ return false; }

};

// SRTF Algorithm
class SRTF : public Scheduler{
    public:
        SRTF(){};
        SRTF(int q, int max){
            quantum=q;
            maxPriority=max;
        }
        Process* get_next_process(){
            Process* p=NULL;
            if(!run_queue.empty()){
                p = run_queue.front();
                run_queue.pop_front();
            }
            return p;
        }
        void add_process(Process* p){
            int ind = run_queue.size();
            for(int i=0; i<run_queue.size(); i++){
                if(p->remCpuBurst<run_queue[i]->remCpuBurst){
                    ind=i;
                    break;
                }
            }
            run_queue.insert(run_queue.begin()+ind, p);
        }

        void insertExpiredProcess(Process* p){
            this->add_process(p);
        }


        string getSchedulerName(){return "SRTF";}
        bool test_preempt(deque<Event*> event_queue, Process* run, int curr){ return false; }

};

// Round Robin Algorithm
class RR : public Scheduler{
    public:
        RR(){};
        RR(int q, int max){
            quantum=q;
            maxPriority=max;
        }

        Process* get_next_process(){
            Process* p=NULL;
            if(!run_queue.empty()){
                p=run_queue.front();
                run_queue.pop_front();
            }
            return p;
        }
        void add_process(Process* p){
            run_queue.push_back(p);
        }

        void insertExpiredProcess(Process* p){
            this->add_process(p);
        }
        
        string getSchedulerName(){return "RR";}
        bool test_preempt(deque<Event*> event_queue, Process* run, int curr){ return false; }

};

// Priority Scheduling Algorithm
class PRIO : public Scheduler{
    public:
        map<int, deque<Process*>> active_queue;
        map<int, deque<Process*>> expired_queue;
        PRIO(){};
        PRIO(int q, int max){
            quantum=q;
            maxPriority=max;
            for(int i=0; i<maxPriority; i++){
                active_queue[i]=deque<Process*>();
                expired_queue[i]=deque<Process*>();
            }
        }
        Process* get_next_process(){
            Process* p=NULL;
            bool all_Empty=true;
            for(int i=0; i<maxPriority; i++){
                if(!active_queue[i].empty()){
                    all_Empty=false;
                    break;
                }
            }

            if(all_Empty){
                for(int i=0; i<maxPriority; i++){
                    active_queue[i].swap(expired_queue[i]);
                }
            }

            int index;
            for(index=maxPriority-1; index>=0; index--){

                if(!active_queue[index].empty())
                    break;
            }

            if(index!=-1){
            p=active_queue[index].front();
            active_queue[index].pop_front();
            }

            return p;

        }
        void add_process(Process* p){
            active_queue[p->dynamicPriority].push_back(p);
        }
        void insertExpiredProcess(Process* p){
            expired_queue[p->dynamicPriority].push_back(p);
        }

        string getSchedulerName(){return "PRIO";}
        bool test_preempt(deque<Event*> event_queue, Process* run, int curr){ return false; }

};

// Preemptive Priority Scheduling Algorithm
class PREPRIO: public Scheduler{
    public:
        map<int, deque<Process*>> active_queue;
        map<int, deque<Process*>> expired_queue;
        PREPRIO(){};
        PREPRIO(int q, int max){
            quantum=q;
            maxPriority=max;
            for(int i=0; i<maxPriority; i++){
                active_queue[i]=deque<Process*>();
                expired_queue[i]=deque<Process*>();
            }
        }
        Process* get_next_process(){
            Process* p=NULL;
            bool all_Empty=true;
            for(int i=0; i<maxPriority; i++){
                if(!active_queue[i].empty()){
                    all_Empty=false;
                    break;
                }
            }
            if(all_Empty){
                for(int i=0; i<maxPriority; i++){
                    active_queue[i].swap(expired_queue[i]);
                }
            }

            int index;
            for(index=maxPriority-1; index>=0; index--){
                if(!active_queue[index].empty())
                    break;
            }


            if(index!=-1){
            p=active_queue[index].front();
            active_queue[index].pop_front();
            }

            return p;

        }
        void add_process(Process* p){
            active_queue[p->dynamicPriority].push_back(p);
        }
        void insertExpiredProcess(Process* p){
            expired_queue[p->dynamicPriority].push_back(p);
        }
        void printSize(){
            for(int i=0; i<maxPriority; i++){
                cout<<"AQ "<<i<<" "<<active_queue[i].size()<<endl;
            }
        }

        Process* peek(){
            Process* p=NULL;
            bool all_Empty=true;
            for(int i=0; i<maxPriority; i++){
                if(!active_queue[i].empty()){
                    all_Empty=false;
                    break;
                }
            }

            if(all_Empty){
                for(int i=0; i<maxPriority; i++){
                    active_queue[i].swap(expired_queue[i]);
                }
            }

            int index;
            for(index=maxPriority-1; index>=0; index--){
                if(!active_queue[index].empty())
                    break;
            }

            if(index!=-1){
            p=active_queue[index].front();
            }
            return p;
        }

        string getSchedulerName(){return "PREPRIO";}

        bool test_preempt(deque<Event*> event_queue, Process* runP, int curr){
            int i;
            for(i=0; i<event_queue.size(); i++){
                if(event_queue[i]->pID==runP->pID)
                    break;
            }
            if(i==event_queue.size())
                return false;

            if(curr==event_queue[i]->endTime)
                return false;
            else{
                Process* highest = this->peek();
                if(highest==NULL)
                    return false;
                else if(highest->dynamicPriority > runP->dynamicPriority)
                    return true;
                else
                    return false;
            }
        }

};


// Simulator class for DES.
class Simulator{
    public:
        deque<Event*> event_queue;
        Scheduler* myScheduler;
        ArgsCollector* myargs;
        Process* runningProcess;

        Simulator(){};
        Simulator(ArgsCollector* args, Scheduler* s){
            myargs=args;
            myScheduler=s;
            runningProcess=NULL;
        }

        void printEventQueue(){
        for(int i=0; i<event_queue.size(); i++){
            cout<<"("<<event_queue[i]->pID<<", "<<event_queue[i]->endTime<<", "<<printStates[event_queue[i]->nextState]<<")"<<" ";
        }
        cout<<endl;    
        }

        Event* getEvent(){
        Event* eve=NULL;
        if(!event_queue.empty()){
            eve = event_queue.front();
            event_queue.pop_front();
        }
        return eve;
        }

        int get_next_event_time(){
        Event* eve=NULL;
        if(!event_queue.empty())
            eve = event_queue.front();

        if(eve)
            return eve->endTime;
        else
            return -1;
    }

        void putEvent(Event* eve){
        int n = event_queue.size();
        int i;
        for(i=0; i<n; i++){
            if(eve->endTime<event_queue[i]->endTime)
                break;
        }
        event_queue.insert(event_queue.begin()+i, eve);
        }

        void initializeEventQueue(){
        for(int i=0; i<processList.size(); i++){
            Process* p = processList[i];
            Event* eve;
            eve = new Event(p->pID, p->arrivalTime, p->state, READY, p->arrivalTime);
            this->putEvent(eve);
        } 
        }
        // Function for running the simulation.
        void simulate(){
            bool callScheduler=false;
            Event* eve;
            while((eve=this->getEvent())){
                int randomIo;
                int randomCpuBurst;
                vector<int> interval;
                int j;
                Event* newEvent;
                Event* oldEvent;
                int rems, util;
                Process* p = processList[eve->pID];
                currTimeStamp=eve->endTime;
                int timeInPrevState=currTimeStamp-(p->stateTs);
                switch(eve->nextState){
                    case READY:
                    if(p->state==RUNNG){
                        p->remCpuBurst=(p->remCpuBurst) - (currTimeStamp-eve->timeStamp);
                    }
                    if(myargs->verbose && eve->oldState!=RUNNG)
                        cout<<currTimeStamp<<" "<<p->pID<<" "<<timeInPrevState<<": "<<printStates[eve->oldState]<<" -> "<<printStates[eve->nextState]<<endl;
                    else if(myargs->verbose && eve->oldState==RUNNG)
                        cout<<currTimeStamp<<" "<<p->pID<<" "<<timeInPrevState<<": "<<printStates[eve->oldState]<<" -> "<<printStates[eve->nextState]<<" cb="<<p->randCpuBurst<<" rem="<<p->remCpuBurst<<" prio="<<p->dynamicPriority<<endl;

                    if(myargs->eventTrace){
                        printEventQueue();
                    }

                        p->state=READY;
                        p->stateTs=currTimeStamp;

                        if(eve->oldState==BLOCK){
                            p->dynamicPriority=(p->staticPriority-1);
                            myScheduler->add_process(p);
                            }else if(eve->oldState==PREEMPTION || eve->oldState==RUNNG){
                                if(eve->oldState==RUNNG)
                                    runningProcess=NULL;
                                p->dynamicPriority=(p->dynamicPriority)-1;
                                if(p->dynamicPriority==-1){
                                    p->dynamicPriority=(p->staticPriority)-1;
                                    myScheduler->insertExpiredProcess(p);
                                }else{
                                    myScheduler->add_process(p);
                                }
                            }else{
                                myScheduler->add_process(p);
                            }
                            callScheduler=true;  
                            break;
                    case RUNNG:

                        p->state=RUNNG;
                        p->wait_time+=currTimeStamp-(p->stateTs);
                        p->stateTs=currTimeStamp;
                        runningProcess=p;
                        if(p->randCpuBurst==0){
                            randomCpuBurst=myrandom(p->cpuBurst);
                            p->randCpuBurst=randomCpuBurst;
                        }else{
                            randomCpuBurst=p->randCpuBurst;
                        }
                        if(myargs->eventTrace){
                            cout<<"Before: ";
                            printEventQueue();
                        }

                        if(myargs->verbose)
                            cout<<currTimeStamp<<" "<<p->pID<<" "<<timeInPrevState<<": "<<printStates[eve->oldState]<<" -> "<<printStates[eve->nextState]<<" cb="<<p->randCpuBurst<<" rem="<<p->remCpuBurst<<" prio="<<p->dynamicPriority<<endl;

                        if(myScheduler->quantum>p->randCpuBurst){
                            if(p->randCpuBurst<p->remCpuBurst){
                                newEvent = new Event(p->pID, currTimeStamp, p->state, BLOCK, currTimeStamp+randomCpuBurst);
                                p->randCpuBurst=0;
                                this->putEvent(newEvent);

                            }else{
                                randomCpuBurst=p->remCpuBurst;
                                newEvent = new Event(p->pID, currTimeStamp, p->state, FINISHED, currTimeStamp+randomCpuBurst);

                                p->randCpuBurst=0;
                                this->putEvent(newEvent);
                            }
                        }else{
                            if(myScheduler->quantum<p->remCpuBurst){
                                if(myScheduler->quantum==p->randCpuBurst){
                                    randomCpuBurst=myScheduler->quantum;
                                    newEvent = new Event(p->pID, currTimeStamp, p->state, BLOCK, currTimeStamp+randomCpuBurst);
                                    p->randCpuBurst=0;
                                    this->putEvent(newEvent);
                                }else{
                                    randomCpuBurst=myScheduler->quantum;
                                    newEvent = new Event(p->pID, currTimeStamp, p->state, READY, currTimeStamp+randomCpuBurst);

                                    p->randCpuBurst=(p->randCpuBurst)-randomCpuBurst;
                                    this->putEvent(newEvent);
                                }
                            }else{
                                randomCpuBurst=p->remCpuBurst;
                                newEvent = new Event(p->pID, currTimeStamp, p->state, FINISHED, currTimeStamp+randomCpuBurst);
                                p->randCpuBurst=0;
                                this->putEvent(newEvent);
                            }
                        }
                        if(myargs->eventTrace){
                        cout<<"After: ";
                        printEventQueue();
                        }
                        break;
                    case BLOCK:
                        p->state=BLOCK;
                        p->remCpuBurst = (p->remCpuBurst)-(currTimeStamp-(eve->timeStamp));
                        p->stateTs=currTimeStamp;
                        runningProcess=NULL;
                        randomIo =  myrandom(p->ioBurst);
                        p->io_time+=randomIo;
                        if(myargs->eventTrace){
                            cout<<"Before: ";
                            printEventQueue();
                        }
                        interval.push_back(currTimeStamp);
                        interval.push_back(currTimeStamp+randomIo);
                        ioTimeQueue.push_back(interval);
                        newEvent = new Event(p->pID, currTimeStamp, p->state, READY, currTimeStamp+randomIo);
                        this->putEvent(newEvent);
                        callScheduler=true;
                        if(myargs->verbose){
                        cout<<currTimeStamp<<" "<<p->pID<<" "<<timeInPrevState<<": "<<printStates[eve->oldState]<<" -> "<<printStates[eve->nextState]<<" ib="<<randomIo<<" rem="<<p->remCpuBurst<<endl;
                    }
                        if(myargs->eventTrace){
                            cout<<"After: ";
                            printEventQueue();
                        }

                        break;
                    case PREEMPTION:
                        p->state=PREEMPTION;
                        p->stateTs=currTimeStamp;
                        runningProcess=NULL;
                        if(myargs->eventTrace){
                            cout<<"Before: ";
                            printEventQueue();
                        }

                        for(j=0; j<event_queue.size(); j++){
                            if(event_queue[j]->pID==p->pID)
                             break;
                            }
                        oldEvent = event_queue[j];
                        event_queue.erase(event_queue.begin()+j);
                        util = currTimeStamp-(oldEvent->timeStamp);
                        p->remCpuBurst=(p->remCpuBurst)-util;
                        p->randCpuBurst=(p->randCpuBurst)+(oldEvent->endTime)-currTimeStamp;
                        rems = eve->endTime - currTimeStamp;
                        p->randCpuBurst+=rems;
                        newEvent= new Event(p->pID, currTimeStamp, p->state, READY, currTimeStamp);
                        
                        if(myargs->eventTrace){
                            cout<<"After: ";
                            printEventQueue();
                        }
                        this->putEvent(newEvent);
                        callScheduler=false;
                        break;
                    case FINISHED:
                        p->ft=currTimeStamp;
                        p->trt=(p->ft)-(p->arrivalTime);
                        p->state=FINISHED;
                        runningProcess=NULL;
                        if(myargs->verbose){
                        cout<<currTimeStamp<<" "<<p->pID<<" "<<timeInPrevState<<": "<<printStates[eve->nextState]<<endl;
                         }

                        if(myargs->eventTrace){
                            cout<<"After: ";
                            printEventQueue();
                        }
                        callScheduler=true;
                        break;
                    }

                    if(callScheduler){
                        if(this->get_next_event_time()==currTimeStamp)
                            continue;
                        callScheduler=false;
                        if(runningProcess==NULL){
                            runningProcess=myScheduler->get_next_process();
                            if(runningProcess==NULL)
                                continue;
                            if(myargs->eventTrace){
                            cout<<"Before: ";
                            printEventQueue();
                            }

                            Event* newEvent= new Event(runningProcess->pID, currTimeStamp, runningProcess->state, RUNNG, currTimeStamp);
                            this->putEvent(newEvent);
                            if(myargs->eventTrace){
                            cout<<"After: ";
                            printEventQueue();
                            }

                        }else{
                            if(myScheduler->test_preempt(event_queue, runningProcess, currTimeStamp)){
                                if(myargs->eventTrace){
                                cout<<"Before: ";
                                printEventQueue();
                                }
                                Event* preemptEvent = new Event(runningProcess->pID, currTimeStamp, runningProcess->state, PREEMPTION, currTimeStamp);
                                this->putEvent(preemptEvent);
                                if(myargs->eventTrace){
                                cout<<"After: ";
                                printEventQueue();
                                }

                            }
                        }
                    }
            }
        }
        // Function for printing the results to STDOUT
        void describe(){
            string algo = myargs->algo;
            if(algo=="F")
                cout<<"FCFS"<<endl;
            else if(algo=="L")
                cout<<"LCFS"<<endl;
            else if(algo=="S")
                cout<<"SRTF"<<endl;
            else if(algo[0]=='R'){
                cout<<"RR "<<myScheduler->quantum<<endl;
            }else if(algo[0]=='P'){
                cout<<"PRIO "<<myScheduler->quantum<<endl;
            }else if(algo[0]=='E'){
                cout<<"PREPRIO "<<myScheduler->quantum<<endl;
            }

            vector<vector<int>> mergedIoTime = merge(ioTimeQueue);
            for(int i=0; i<mergedIoTime.size(); i++){
                total_io_time+=(mergedIoTime[i][1]-mergedIoTime[i][0]);
            }


            for(int i=0; i<processList.size(); i++){

                printf("%04d: %4d %4d %4d %4d %d |  %4d  %4d  %4d  %4d\n", processList[i]->pID, processList[i]->arrivalTime, processList[i]->totalCPUtime, processList[i]->cpuBurst,
                       processList[i]->ioBurst, processList[i]->staticPriority, processList[i]->ft, processList[i]->trt, processList[i]->io_time, processList[i]->wait_time);

                if(processList[i]->ft>total_final_time)
                    total_final_time=processList[i]->ft;

                total_cpu_time+=processList[i]->totalCPUtime;
                avg_wait_time+=processList[i]->wait_time;
                avg_trt+=processList[i]->trt;
            }
            avg_wait_time/=processList.size();
            avg_trt/=processList.size();
            total_cpu_time=(total_cpu_time*100.0)/total_final_time;
            total_io_time=(total_io_time*100.0)/total_final_time;
            throughput=(processList.size()*100.0)/total_final_time;

            printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", total_final_time, total_cpu_time, total_io_time, avg_trt, avg_wait_time, throughput);
            
        }
};

// Function for reading the arguments.
ArgsCollector* read(int argc, char* argv[]){
    ArgsCollector* args = new ArgsCollector();
    opterr = 0;
    int index=0;
    int c=0;

    while((c=getopt(argc, argv, "vteps:"))!=-1){
        switch(c){
            case 'v':
                args->verbose=true;
                break;
            case 's':
                args->algo = optarg;
                break;
            case 'e':
                args->eventTrace=true;
                break;
            case 't':
                args->runQueueTrace=true;
                break;
            case 'p':
                args->preemptTrace=true;
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

void run(ArgsCollector* args){
    // Initialize random seed array
    char randName[(args->randFile).length()+1];
    strcpy(randName, (args->randFile).c_str());
    initRandom(randName);
    int quantum;
    int maxPriority;
    // Read algo name and initialize scheduler
    Scheduler* schedule;
    string algo = args->algo;
    if(algo=="F"){
        quantum=10000;
        maxPriority=4;
        schedule= new FCFS(quantum, maxPriority);

    }else if(algo=="L"){
        quantum=10000;
        maxPriority=4;
        schedule= new LCFS(quantum, maxPriority);

    }else if(algo=="S"){
        quantum=10000;
        maxPriority=4;
        schedule= new SRTF(quantum, maxPriority);

    }else if(algo[0]=='R'){
        maxPriority=4;
        string q = algo.substr(1);
        quantum = stoi(q);
        schedule = new RR(quantum, maxPriority);
    }else if(algo[0]=='P'){
        string qP = algo.substr(1);
        size_t found;
        found=algo.find(':');
        if(found==string::npos){
            quantum=stoi(qP);
            maxPriority=4;
        }else{
            int index = found;
            quantum=stoi(qP.substr(0, index));
            maxPriority=stoi(qP.substr(index));
        }
        schedule = new PRIO(quantum, maxPriority);
    }else if(algo[0]=='E'){
        string qP = algo.substr(1);
        size_t found;
        found=algo.find(':');
        if(found==string::npos){
            quantum=stoi(qP);
            maxPriority=4;
        }else{
            int index = found;
            quantum=stoi(qP.substr(0, index));
            maxPriority=stoi(qP.substr(index));
        }
        schedule = new PREPRIO(quantum, maxPriority);

    }
    else
        schedule=NULL;

    // Read Input and initialize process objects
    Tokenizer tokens = Tokenizer(args->inputFile);
    int finalOff=tokens.offset;
    int finalLine=tokens.finalLine;
    int processCounter=-1;

    vector<pair<loff, string> >::iterator ftr;
    vector<pair<loff, string> >::iterator btr;
    ftr = tokens.container.begin();
    btr = tokens.container.end();

    while(ftr!=btr){
        processCounter+=1;
        int at = tokens.readInt(ftr, btr, finalLine, finalOff);
        ftr = next(ftr);

        int totalCPU = tokens.readInt(ftr, btr, finalLine, finalOff);
        ftr=next(ftr);

        int cpuB = tokens.readInt(ftr, btr, finalLine, finalOff);
        ftr=next(ftr);

        int ioB = tokens.readInt(ftr, btr, finalLine, finalOff);
        ftr=next(ftr);
        int staticPriority=myrandom(maxPriority);
        int dynamicPriority=staticPriority-1;
        Process* p = new Process(processCounter, at, totalCPU, cpuB, ioB, staticPriority, dynamicPriority, CREATED);
        processList.push_back(p);
    
    }
    // Do simulation
    Simulator simulator(args, schedule);
    simulator.initializeEventQueue();
    simulator.simulate();

    // Print Results
    simulator.describe();

}

int main(int argc, char* argv[]){

    ArgsCollector* args = read(argc, argv);
    run(args);
    return 0;
}

