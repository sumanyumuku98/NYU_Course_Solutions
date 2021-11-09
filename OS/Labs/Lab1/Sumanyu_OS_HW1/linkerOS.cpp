#include <stdio.h>
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

using namespace std;


// (line, offset) datatype
typedef pair<int,int> loff;
typedef pair<string,string> errType;

string rule5Warning(int module, char* token, int len, int modulLen){
    // printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n", module, token, len, modulLen-1);
    string s = "Warning: Module " + to_string(module) + ": "+ string(token) + " too big "+ to_string(len) + " (max=" + to_string(modulLen-1) + ") assume zero relative\n";
    return s;
}
//parse error types
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
                    // cout<<line<<endl;
                    while(toks!=NULL){
                        string toks_str(toks);
                        // cout<<toks_str<<endl;
                        int found=line.find(toks_str, offsetCounter);
                        // cout<<found<<" "<<toks_str<<" "<<toks_str.length()<<endl;
                        container.push_back(make_pair(make_pair(lineCounter, found+1), toks_str));
                        toks=strtok(NULL, " \t\r\f\v");
                        offsetCounter=found+toks_str.length();
                        // cout<<offsetCounter<<endl;
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
            // if(itr==itrEnd){
            //     _parseError(l.first, l.second+s.length(), 0);
            // }
            int x;
            bool truth=true;
            for (char const &c : s){
                if (isdigit(c) == 0){
                    truth=false;
                    break;
                    }
                }

            if(truth){
                // cout<<s<<endl;
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
                // Symbol sym(s);
                // cout<<sym.s<<endl;
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
            // pair<int,int> fileEnd=container[container.size()-1].first;
            // string lastElem=container[container.size()-1].second;
            // int finalOffset=fileEnd.second+lastElem.length();

            for(int i=0; i<container.size(); i++){
                cout<<"Token: "<<container[i].first.first<<":"<<container[i].first.second<<" : "<<container[i].second<<"\n";
            }
            cout<<"Final Spot in File : "<<"line="<<finalLine<<" offset="<<offset<<"\n";
        };

};


/*
pass1 for the linker
-> Takes the file name
-> reads the input as tokens with line, offset
-> Does parse error checking
-> return symbol table
*/
vector<pair<string,int> > pass1( string filename){
    Tokenizer tokens = Tokenizer(filename);
    int finalOff=tokens.offset;
    int finalLine=tokens.finalLine;

    vector<pair<string, int> > symTable;
    // vector< vector<string> > rule5Warns;
    vector<pair<string,loff > > moduleSymTracker;
    int globalBaseAddr=0;
    int moduleCounter=0;
    vector<pair<loff, string> >::iterator ftr;
    vector<pair<loff, string> >::iterator btr;
    ftr = tokens.container.begin();
    btr = tokens.container.end();
    int instrCounter=0;
    map<string, string> rule2Errors;
    vector<int> instrCount_;

    loff pairVal(0,0);
    string oldToken("");

    while(ftr!=btr){
        moduleCounter+=1;
        // Read definition list first
        int defCount=tokens.readInt(ftr, btr, finalLine, finalOff);
        pairVal=ftr->first;
        oldToken=ftr->second;

        vector<string> defSymbols;
        vector<string> rule5Warns;

        if(defCount>16)
            _parseError((ftr->first).first, (ftr->first).second, 4);
        ftr=next(ftr);

        for(int i=0; i<defCount; i++){
            string sym= tokens.readSymbol(ftr, btr, finalLine, finalOff);

            pairVal=ftr->first;
            oldToken=ftr->second;

            ftr=next(ftr);
            int addr = tokens.readInt(ftr, btr, finalLine, finalOff);

            defSymbols.push_back(sym);

            pairVal=ftr->first;
            oldToken=ftr->second;

            ftr=next(ftr);
            vector<pair<string, int> >::iterator it;
            for(it=symTable.begin(); it!=symTable.end(); it++){
                if(it->first==sym)
                    break;
            }
            // if(it==symTable.end())
            //     symTable.insert(pair<string, int> (sym, globaladdr));
            // else
            //     it->second+=globalBaseAddr;

            // rule 2 check
            if(it==symTable.end()){
                symTable.push_back(make_pair(sym, globalBaseAddr+addr));
                moduleSymTracker.push_back(make_pair(sym, make_pair(moduleCounter, globalBaseAddr)));
            }
            else{
                rule2Errors.insert(make_pair(sym, "Error: This variable is multiple times defined; first value used"));
            }

        }

        // Read use list next

        int useCount=tokens.readInt(ftr, btr, finalLine, finalOff);
        pairVal=ftr->first;
        oldToken=ftr->second;
        
        if(useCount>16)
            _parseError((ftr->first).first, (ftr->first).second, 5);
        ftr=next(ftr);
        for(int j=0; j<useCount; j++){
            string sym2 = tokens.readSymbol(ftr, btr, finalLine, finalOff);
            pairVal=ftr->first;
            oldToken=ftr->second;
            ftr=next(ftr);
        }

        // read text data next
        int codeCount = tokens.readInt(ftr, btr, finalLine, finalOff);
        instrCounter+=codeCount;

        if(instrCounter>512){
            _parseError((ftr->first).first, (ftr->first).second, 6);
        }
        instrCount_.push_back(codeCount);

        pairVal=ftr->first;
        oldToken=ftr->second;
        ftr=next(ftr);



        //rule 5 warning check
        for(int i=0; i<defSymbols.size(); i++){
            string s = defSymbols[i];
            vector<pair<string, int> >:: iterator its;
            for(its=symTable.begin(); its!=symTable.end(); its++){
                if(its->first==s)
                    break;
            }
            int totalAddr = its->second;
            vector<pair<string, loff> >::iterator its2;
            for(its2=moduleSymTracker.begin(); its2!=moduleSymTracker.end(); its2++){
                if(its2->first==s)
                    break;
            }
            int moduleG = (its2->second).second;
            int moduleNo = (its2->second).first;

            if(moduleCounter==moduleNo && totalAddr-moduleG>=codeCount){
                char* arr = new char[s.length()+1];
                strcpy(arr, s.c_str());
                rule5Warns.push_back(rule5Warning(moduleCounter, arr, totalAddr-moduleG, codeCount));
                its->second = moduleG;
            }else if(totalAddr==globalBaseAddr+totalAddr-moduleG && totalAddr-moduleG>=codeCount){
                char* arr = new char[s.length()+1];
                strcpy(arr, s.c_str());
                rule5Warns.push_back(rule5Warning(moduleCounter, arr, totalAddr-moduleG, instrCount_[moduleNo-1]));
                its->second=moduleG;
                
            }
        }

        // else if(totalAddr-moduleG>=instrCount_[moduleNo-1]){
        //         if(totalAddr==globalBaseAddr+totalAddr-moduleG){
        //         char* arr = new char[s.length()+1];
        //         strcpy(arr, s.c_str());
        //         rule5Warning(moduleCounter, arr, totalAddr-moduleG, instrCount_[moduleNo-1]);
        //         its->second=moduleG;
        //         }
        //     }

        // vector<pair<string, int> >::iterator it;
        // for(it=symTable.begin(); it!=symTable.end(); it++){
        //     if(it->second-globalBaseAddr>=codeCount){
                // int addr_ = it->second-globalBaseAddr;
                // string s = it->first;
                // char* arr = new char[s.length()+1];
                // strcpy(arr, s.c_str());
                // rule5Warning(moduleCounter, arr, addr_, codeCount);
        //         it->second=globalBaseAddr;
        //     }
        // }

        for(int k=0; k<codeCount; k++){
            // if(instrCounter>=511)
            //     _parseError((ftr->first).first, (ftr->first).second, 6);
            char instr = tokens.readIAER(ftr, btr, finalLine, finalOff);
            pairVal=ftr->first;
            oldToken=ftr->second;

            ftr=next(ftr);
            int instrCode = tokens.readInt(ftr, btr, finalLine, finalOff);
            pairVal=ftr->first;
            oldToken=ftr->second;

            ftr=next(ftr);
            // instrCounter++;

        }
        globalBaseAddr+=codeCount;

        // Print rule 5 warns
        for(int i=0; i<rule5Warns.size(); i++)
            cout<<rule5Warns[i];
    }

    // print symbol table
    cout<<"Symbol Table"<<endl;
    vector<pair<string, int> >::iterator it;
    map<string, string>::iterator errItr;
    for(it=symTable.begin(); it!=symTable.end(); it++){
        // cout<<it->first<<"="<<it->second<<endl;
        errItr=rule2Errors.find(it->first);
        string s;
        if(errItr!=rule2Errors.end())
            s=errItr->second;
        else
            s="";

        cout<<it->first<<"="<<it->second<<" "<<s<<endl;   

}

    cout<<"\n";

    return symTable;
};



/*
pass 2 for linker
-> takes symbol table and file name
-> Reads input as tokens with (line, offset)
-> resolves addresses
-> checks for errors and warnings
-> prints memory map
*/
void pass2(vector<pair<string, int> > & table, string filename){
    Tokenizer tokens = Tokenizer(filename);
    // vector<Module> objModules;
    int finalOff = tokens.offset;
    int finalLine = tokens.finalLine;

    int globalBaseAddr=0;

    vector< vector<pair<int, string> > > globalAddressResolver;


    vector<pair<string, int> > symWithMod;
    set<string> globalUseSymbols;


    // Rule 4 warnings
    vector<pair<int, string> > extraWarnings;

    // Rule 7 warnings
    vector<vector<string> > eInstrWarn;
    vector<int> baseAddrTracker;
    
    int moduleCounter=0;

    
    vector<pair<loff, string> >::iterator ftr;
    vector<pair<loff, string> >::iterator btr;
    ftr = tokens.container.begin();
    btr = tokens.container.end();

    loff pairVal(0, 0);
    string oldToken("");


    while(ftr!=btr){
        
        vector<string> useSymbols;
        vector<pair<int, string> > addressResolver;

        // vector<string> eRefSym;
        // Read definition list first
        moduleCounter+=1;
        // cout<<"========== "<<moduleCounter<<" ==========="<<endl;

        int defCount=tokens.readInt(ftr, btr, finalLine, finalOff);
        ftr=next(ftr);
        for(int i=0; i<defCount; i++){
            string sym= tokens.readSymbol(ftr, btr, finalLine, finalOff);
            ftr=next(ftr);

            // symWithMod.insert(make_pair(sym, moduleCounter));
            vector<pair<string, int> >::iterator itrs;
            for(itrs=symWithMod.begin(); itrs!=symWithMod.end(); itrs++){
                if(itrs->first==sym)
                    break;
            }
            if(itrs==symWithMod.end())
                symWithMod.push_back(make_pair(sym, moduleCounter));

            int addr = tokens.readInt(ftr, btr, finalLine, finalOff);
            ftr=next(ftr);
        }

        // if(moduleCounter==3){
        //     for(int i=0; i<symWithMod.size(); i++){
        //         cout<<symWithMod[i].first<<" "<<symWithMod[i].second<<endl;
        //     }
        //     cout<"==================\n";
        // }


        // Read use list next

        int useCount=tokens.readInt(ftr, btr, finalLine, finalOff);
        ftr=next(ftr);
        for(int j=0; j<useCount; j++){
            string sym2 = tokens.readSymbol(ftr, btr, finalLine, finalOff);
            ftr=next(ftr);
            
            // vector<string>::iterator itStr = find(useSymbols.begin(), useSymbols.end(), sym2);

            useSymbols.push_back(sym2);            
            // globalUseSymbols.insert(sym2);
        }

        // if(moduleCounter==3){
        //     for(int i=0; i<useSymbols.size(); i++){
        //         cout<<useSymbols[i]<<endl;
        //     }
        //     cout<"==================\n";

        // }

        bool *eRefSym;

        if(useSymbols.empty()==false)
            eRefSym = new bool[useSymbols.size()]{false};

        // read text data next
        int codeCount = tokens.readInt(ftr, btr, finalLine, finalOff);
        ftr=next(ftr);

        for(int k=0; k<codeCount; k++){
            char instr = tokens.readIAER(ftr, btr, finalLine, finalOff);
            ftr=next(ftr);
            int instrCode = tokens.readInt(ftr, btr, finalLine, finalOff);
            ftr=next(ftr);

            // Change made here for pass 2

            // rule 11 check
            int opC=(int)instrCode/1000;
            if(opC>=10){
                if(instr=='I')
                    addressResolver.push_back(make_pair(9999, "Error: Illegal immediate value; treated as 9999"));
                else
                    addressResolver.push_back(make_pair(9999, "Error: Illegal opcode; treated as 9999"));
                continue;
                }

            if (instr=='R'){
                string war;
                int opCode= (int)instrCode/1000;
                int operand = instrCode%1000;
                // rule 9 check
                if(operand>=codeCount){
                    war="Error: Relative address exceeds module size; zero used";
                    instrCode=opCode*1000;
                }else
                    war="";

                int newAddr=instrCode+globalBaseAddr;
                addressResolver.push_back(make_pair(newAddr, war));
            }else if(instr=='E'){
                int index = instrCode%1000;

                // rule 6 check
                if(useSymbols.empty() || index>=useSymbols.size()){
                    string war="Error: External address exceeds length of uselist; treated as immediate";
                    addressResolver.push_back(make_pair(instrCode, war));
                    continue;
                }
                string s = useSymbols[index];

                // if(index=0){
                //     cout<<instr<<" "<<instrCode<<endl;

                // }

                eRefSym[index]=true;
                globalUseSymbols.insert(s);
                
                vector<pair<string, int> >::iterator it2;
                for(it2=table.begin(); it2!=table.end(); it2++){
                    if(it2->first==s)
                        break;
                }

                int offset;
                string war;
                // rule 3 check
                if(it2==table.end()){
                    offset=0;
                    war="Error: "+s+" is not defined; zero used";
                }else{
                offset = it2->second;
                war="";
                }
                int opCode=(int)instrCode/1000;

                int newAddr = opCode*1000 + offset;
                addressResolver.push_back(make_pair(newAddr, war));
        }else if(instr=='A'){
            string war;
            int operand=instrCode%1000;
            int op=(int)instrCode/1000;
            // rule 8 check
            if(operand>=512){
                war="Error: Absolute address exceeds machine size; zero used";
                addressResolver.push_back(make_pair(op*1000, war));
            }else                
                addressResolver.push_back(make_pair(instrCode, war));

        }else{
            string war;
            war="";
            addressResolver.push_back(make_pair(instrCode, war));
            

        }
        }

        globalBaseAddr+=codeCount;
        baseAddrTracker.push_back(globalBaseAddr-1);

        // cout<<"=========================\n";
        // cout<<"Module: "<<moduleCounter<<endl;
        // cout<<"Use Symbols: ";
        // for(int i=0; i<useSymbols.size(); i++){
        //     cout<<useSymbols[i]<<" ";
        // }
        // cout<<endl;
        // cout<<"E Ref Symbols: ";
        // int n = (int)(sizeof(eRefSym)/sizeof(eRefSym[0]));
        // for(int i=0; i<n; i++){
        //     if(eRefSym[i])
        //         cout<<useSymbols[i]<<" ";
        // }
        // cout<<endl;
        // cout<<"=========================\n";


        // Rule 7 check
        vector<string> temp;
        for(int i=0; i<useSymbols.size(); i++){
            string s = useSymbols[i];
            // vector<string>:: iterator its = find(eRefSym.begin(), eRefSym.end(), s);
            if(eRefSym[i]==false)
                temp.push_back("Warning: Module "+to_string(moduleCounter)+": "+ s+ " appeared in the uselist but was not actually used\n");
            // else
            //     cout<<"====== "<<s<<" ========"<<endl;
        }

        // cout<<"======= Module: "<<moduleCounter<<" =======\n"<<endl;
        // for(int i=0; i<temp.size(); i++){
        //     cout<<temp[i]<<" ";
        // }
        // cout<<endl;
        // cout<<"==============================================\n";
        eInstrWarn.push_back(temp);
        globalAddressResolver.push_back(addressResolver);
    }

    // Rule 4 check
    // map<string, int>::iterator it3;
    for(int i=0; i<symWithMod.size(); i++){
        string sym_ = symWithMod[i].first;
        int m_no = symWithMod[i].second;
        if(globalUseSymbols.find(sym_)==globalUseSymbols.end()){
            string war="Warning: Module "+to_string(m_no)+": "+sym_+" was defined but never used\n";
            extraWarnings.push_back(make_pair(m_no, war));
        }
    }
    // if(extraWarnings.size())
    //     sort(extraWarnings.begin(), extraWarnings.end());

    cout<<"Memory Map"<<endl;
    int baseCounter=0;
    
    for(int i=0; i<globalAddressResolver.size(); i++){
        
        vector<string> moduleWarnings = eInstrWarn[i];
        vector<pair< int, string> > moduleInstr = globalAddressResolver[i];
        
        for(int j=0; j<moduleInstr.size(); j++){
            stringstream ss;
            ss<<setw(3)<<setfill('0')<<baseCounter;
            string count= ss.str();

            stringstream ss2;
            ss2<<setw(4)<<setfill('0')<<moduleInstr[j].first;


            cout<<count<<": "<<ss2.str()<<" "<<moduleInstr[j].second<<endl;
            baseCounter++;

        }

        for(int j=0; j<moduleWarnings.size(); j++){
            cout<<moduleWarnings[j];
        }


    }

    cout<<"\n";

    // for(int i=0; i<baseAddrTracker.size(); i++){
    //     cout<<baseAddrTracker[i]<<" ";
    // }
    // for(int i=0; i<eInstrWarn.size(); i++){
    //     cout<<"\n"<<endl;
    //     vector<string> vecs = eInstrWarn[i];
    //     for(int j=0; j<vecs.size(); j++){
    //         cout<<vecs[j]<<endl;
    //     }
    // }

    //print Extra warnings here
    for(auto v: extraWarnings)
        cout<<v.second;

    cout<<endl;
};



int main(int argc, char* argv[]){

    vector<pair<string, int> > symTable=pass1(argv[argc-1]);


    pass2(symTable, argv[argc-1]);

    // Tokenizer tok = Tokenizer(argv[argc-1]);
    // tok.printToken();
    
    return 0;
}