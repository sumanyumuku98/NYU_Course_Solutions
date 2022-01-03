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


using namespace std;

int* globalCurrentTrack=NULL;
bool* globalSeekDirection=NULL;
int* globalSimTime=NULL;

class ArgsCollector{
    public:
        string algo;
        string inputFile;
        bool v;
        bool q;
        bool f;
        ArgsCollector(){
            algo="i";
            inputFile="";
            v=q=f=false;
        }
        void printArgs(){
            cout<<"Algo: "<<algo<<endl;
            cout<<"Input File: "<<inputFile<<endl;
            cout<<"v: "<<v<<endl;
            cout<<"q: "<<q<<endl;
            cout<<"f: "<<f<<endl;
        }
};

class ioOp{
    public:
        int at;
        int trackNum;
        int start;
        int end;
        ioOp(int arrival, int t){
            at=arrival;
            trackNum=t;
            start=-1;
            end=-1;
        }

};

deque<ioOp*> completeIoOperations;
deque<ioOp*> ioQueue;
deque<ioOp*> flookQueue;
deque<ioOp*> *activeQueue = &flookQueue;
deque<ioOp*> *addQueue = &ioQueue;



class IoScheduler{
    public:
        virtual ioOp* strategy()=0;
};

class FIFO : public IoScheduler{
    public:
        FIFO(){};
        ioOp* strategy(){
            ioOp* op = NULL;
            if(!ioQueue.empty()){
                op=ioQueue.front();
                ioQueue.pop_front();
            }

            return op;
        }
};

class SSTF : public IoScheduler{
    public:
        SSTF(){};
        ioOp* strategy(){
            ioOp* op=NULL;
            int index=-1;
            // cout<<ioQueue.size()<<endl;
            int minMovement = INT_MAX;
            for(int i=0; i<ioQueue.size(); i++){
                int seekTime = abs(*globalCurrentTrack - ioQueue[i]->trackNum);
                if(seekTime < minMovement){
                    minMovement=seekTime;
                    index=i;
                }
            }

            op=ioQueue[index];
            ioQueue.erase(ioQueue.begin()+index);
            return op;
        }
};

class LOOK : public IoScheduler{
    public:
        deque<pair <int,int> > left;
        deque<pair <int,int> > right;

        LOOK(){};
        ioOp* strategy(){
            left.clear();
            right.clear();
            ioOp* op=NULL;
            for(int i=0; i<ioQueue.size(); i++){
                if(ioQueue[i]->trackNum>*globalCurrentTrack)
                    right.push_back(make_pair(ioQueue[i]->trackNum, i));
                else if(ioQueue[i]->trackNum < *globalCurrentTrack)
                    left.push_back(make_pair(ioQueue[i]->trackNum, i));
                else{
                    if(*globalSeekDirection)
                        right.push_back(make_pair(ioQueue[i]->trackNum, i));
                    else
                        left.push_back(make_pair(ioQueue[i]->trackNum, i));
                }
            }
            sort(left.begin(), left.end());
            sort(right.begin(), right.end());

            pair<int, int> item;
            bool ejectfromRight;

            if(*globalSeekDirection){
                if(!right.empty()){
                    item=right.front();
                    ejectfromRight=true;
                }
                else{
                    item=left.back();
                    ejectfromRight=false;
                }
            }else{
                if(!left.empty()){
                    item=left.back();
                    ejectfromRight=false;
                }
                else{
                    item=right.front();
                    ejectfromRight=true;
                }
            }
            
            int ejectIndex=item.second;
            if(ejectfromRight){
                for(int j=0 ; j<right.size(); j++){
                    if(right[j].first==item.first && right[j].second<ejectIndex)
                        ejectIndex=right[j].second;
                }
            }else{
                for(int j=left.size()-1 ; j>=0; j--){
                    if(left[j].first==item.first && left[j].second < ejectIndex)
                        ejectIndex = left[j].second;
                }
            }

            op=ioQueue[ejectIndex];
            ioQueue.erase(ioQueue.begin() + ejectIndex);

            return op;

        }
};

class CLOOK : public IoScheduler{
    public:
        deque<pair <int,int> > left;
        deque<pair <int,int> > right;

        CLOOK(){};
        ioOp* strategy(){
            left.clear();
            right.clear();
            ioOp* op=NULL;
            for(int i=0; i<ioQueue.size(); i++){
                if(ioQueue[i]->trackNum>*globalCurrentTrack)
                    right.push_back(make_pair(ioQueue[i]->trackNum, i));
                else if(ioQueue[i]->trackNum < *globalCurrentTrack)
                    left.push_back(make_pair(ioQueue[i]->trackNum, i));
                else{
                    right.push_back(make_pair(ioQueue[i]->trackNum, i));
                }
            }
            sort(left.begin(), left.end());
            sort(right.begin(), right.end());

            pair<int, int> item;
            int ejectIndex;
            if(!right.empty()){
                item=right.front();
                ejectIndex=item.second;
                for(int j=0 ; j<right.size(); j++){
                    if(right[j].first==item.first && right[j].second<ejectIndex)
                        ejectIndex=right[j].second;
                }

            }else{
                item=left.front();
                ejectIndex=item.second;
                for(int j=0 ; j<left.size(); j++){
                    if(left[j].first==item.first && left[j].second<ejectIndex)
                        ejectIndex=left[j].second;
                }
            }

            op=ioQueue[ejectIndex];
            ioQueue.erase(ioQueue.begin() + ejectIndex);
            return op;

        }
};

class FLOOK : public IoScheduler{
    public:
        deque<pair <int,int> > left;
        deque<pair <int,int> > right;

        FLOOK(){};
        ioOp* strategy(){
            if(activeQueue->empty()){
                deque<ioOp*> *temp = activeQueue;
                activeQueue=addQueue;
                addQueue=temp;
            }
            // cout<<activeQueue->size()<<" "<<addQueue->size()<<endl;
            left.clear();
            right.clear();
            ioOp* op=NULL;
            for(int i=0; i<activeQueue->size(); i++){
                if(activeQueue->at(i)->trackNum>*globalCurrentTrack)
                    right.push_back(make_pair(activeQueue->at(i)->trackNum, i));
                else if(activeQueue->at(i)->trackNum < *globalCurrentTrack)
                    left.push_back(make_pair(activeQueue->at(i)->trackNum, i));
                else{
                    if(*globalSeekDirection)
                        right.push_back(make_pair(activeQueue->at(i)->trackNum, i));
                    else
                        left.push_back(make_pair(activeQueue->at(i)->trackNum, i));
                }
            }
            sort(left.begin(), left.end());
            sort(right.begin(), right.end());

            pair<int, int> item;
            bool ejectfromRight;

            if(*globalSeekDirection){
                if(!right.empty()){
                    item=right.front();
                    ejectfromRight=true;
                }
                else{
                    item=left.back();
                    ejectfromRight=false;
                }
            }else{
                if(!left.empty()){
                    item=left.back();
                    ejectfromRight=false;
                }
                else{
                    item=right.front();
                    ejectfromRight=true;
                }
            }
            
            int ejectIndex=item.second;
            if(ejectfromRight){
                for(int j=0 ; j<right.size(); j++){
                    if(right[j].first==item.first && right[j].second<ejectIndex)
                        ejectIndex=right[j].second;
                }
            }else{
                for(int j=left.size()-1 ; j>=0; j--){
                    if(left[j].first==item.first && left[j].second < ejectIndex)
                        ejectIndex = left[j].second;
                }
            }

            op=activeQueue->at(ejectIndex);
            activeQueue->erase(activeQueue->begin() + ejectIndex);

            return op;

            
        }
};

IoScheduler* IOSCHED = NULL;

class Simulation{
    public:
        int total_time;
        int tot_movement;
        double avg_turnaround;
        double avg_waittime;
        int max_waittime;
        ifstream inp;
        string line;
        int trackNum;
        int arrivalTime;
        ArgsCollector* args;
        int currentTrack;
        bool lowToHigh;
        int simulationTime;
        int instIterator;
        ioOp* activeIo;
        
        Simulation(ArgsCollector* a){
            total_time=tot_movement=max_waittime=trackNum=arrivalTime=currentTrack=0;
            avg_turnaround=avg_waittime=0.0;
            line="";
            args=a;
            char arr[(a->inputFile).length()+1];
            strcpy(arr, (a->inputFile).c_str());
            inp.open(arr);
            lowToHigh=true;
            simulationTime=instIterator=0;
            activeIo=NULL;
            globalCurrentTrack=&currentTrack;
            globalSeekDirection=&lowToHigh;
            globalSimTime=&simulationTime;
        }

        bool get_next_instruction(int& at, int& tr){
            stringstream ss(line);
            if(ss>>at>>tr){
                getline(inp, line);
                return true;
            }
            else
                return false;            
        }

        
        void readIoOperations(){
            getline(inp, line);
            while(line.at(0)=='#')
                getline(inp, line);

            while(this->get_next_instruction(arrivalTime, trackNum)){
                ioOp* newOp = new ioOp(arrivalTime, trackNum);
                completeIoOperations.push_back(newOp);
            }
            inp.close();
        }
        void simulate(){
            while(true){
                if(instIterator<completeIoOperations.size() && completeIoOperations[instIterator]->at==simulationTime){
                    ioOp* currOp=completeIoOperations[instIterator];
                    if(args->algo=="f" || args->algo=="F")
                        addQueue->push_back(currOp);
                    else
                        ioQueue.push_back(currOp);
                    instIterator++;
                }

                if(activeIo && activeIo->end==simulationTime){
                    avg_turnaround+=(activeIo->end - activeIo->at);
                    avg_waittime+=(activeIo->start - activeIo->at);
                    max_waittime=max(max_waittime, activeIo->start - activeIo->at);
                    activeIo=NULL;
                }

                if(!activeIo){
                    if(args->algo=="f" || args->algo=="F"){
                        if(!activeQueue->empty() || !addQueue->empty()){
                            activeIo=IOSCHED->strategy();
                            activeIo->start=simulationTime;
                            activeIo->end=simulationTime+ abs(activeIo->trackNum-currentTrack);
                            if(activeIo->end==simulationTime)
                                continue;
                            if(lowToHigh && activeIo->trackNum<currentTrack)
                                lowToHigh=false;
                            else if(!lowToHigh && activeIo->trackNum>currentTrack)
                                lowToHigh=true;
                        }else if(instIterator<completeIoOperations.size()){
                            simulationTime++;
                            continue;
                        }else{
                            total_time=simulationTime;
                            avg_turnaround/=completeIoOperations.size();
                            avg_waittime/=completeIoOperations.size();
                            break;
                    }

                    }else{
                    if(!ioQueue.empty()){
                        activeIo=IOSCHED->strategy();
                        activeIo->start=simulationTime;
                        activeIo->end=simulationTime+ abs(activeIo->trackNum-currentTrack);
                        if(activeIo->end==simulationTime)
                            continue;
                        if(lowToHigh && activeIo->trackNum<currentTrack)
                            lowToHigh=false;
                        else if(!lowToHigh && activeIo->trackNum>currentTrack)
                            lowToHigh=true;
                    }else if(instIterator<completeIoOperations.size()){
                        simulationTime++;
                        continue;
                    }else{
                        total_time=simulationTime;
                        avg_turnaround/=completeIoOperations.size();
                        avg_waittime/=completeIoOperations.size();
                        break;
                    }
                    }
                }

                if(activeIo){
                    if(lowToHigh)
                        currentTrack++;
                    else
                        currentTrack--;
                    tot_movement++;
                    simulationTime++;
                }
            }
        }


        void printSummary();
        void printOpLevel(int i, ioOp* op); 
        void finalPrintStats();           
};

void Simulation::printSummary(){
    printf("SUM: %d %d %.2lf %.2lf %d\n", this->total_time, this->tot_movement, this->avg_turnaround, this->avg_waittime, this->max_waittime);
}

void Simulation::printOpLevel(int i, ioOp* op){
    printf("%5d: %5d %5d %5d\n",i, op->at, op->start, op->end);
}

void Simulation::finalPrintStats(){
    for(int i=0; i<completeIoOperations.size(); i++){
        this->printOpLevel(i, completeIoOperations[i]);
    }
    this->printSummary();
}



ArgsCollector* read(int argc, char* argv[]){
    ArgsCollector* args = new ArgsCollector();
    opterr = 0;
    int index=0;
    int c=0;

    while((c=getopt(argc, argv, "vqfs:"))!=-1){
        switch(c){
            case 's':
                args->algo=optarg;
                break;
            case 'v':
                args->v = true;
                break;
            case 'q':
                args->q=true;
                break;
            case 'f':
                args->f=true;
                break;
            default:
                break;
        }

        for(index=optind ; index<argc; index++){
            if(index==argc-1)
                args->inputFile=argv[index];
        }


}
    return args;
}

void run(ArgsCollector* args){
    if(args->algo=="i" || args->algo=="I")
        IOSCHED= new FIFO();
    else if(args->algo=="j" || args->algo=="J")
        IOSCHED= new SSTF();
    else if(args->algo=="s" || args->algo=="S")
        IOSCHED= new LOOK();
    else if(args->algo=="c" || args->algo=="C")
        IOSCHED= new CLOOK();
    else if(args->algo=="f" || args->algo=="F")
        IOSCHED= new FLOOK();
    else{
        cout<<"Not implemented"<<endl;;
        exit(EXIT_FAILURE);
    }

    Simulation* simulator = new Simulation(args);
    simulator->readIoOperations();
    simulator->simulate();
    simulator->finalPrintStats();

};



int main(int argc, char* argv[]){
    ArgsCollector* args = read(argc, argv);
    run(args);
    return 0;
}
