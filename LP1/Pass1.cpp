#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <iomanip>
using namespace std;
struct Symbol
{
    string sym;
    int addr = -1; // add not yet assign
};

struct Literal
{
    string lit;
    int addr = -1;
};

class Pass1
{
    unordered_map<std::string, int> OPTAB, REGTAB, CONDTAB, ADTAB;
    vector<Symbol> SYMTAB;
    vector<Literal> LITTAB;
    vector<int> POOLTAB;

    int lc = 0, litcnt = 0, proc_lit = 0; // index till which literal have been assign memeory

public:
    Pass1()
    {
        initialize_OPTAB(); // machine opcode store
        initialize_REGTAB();
        initialize_CONDTAB();
        initialize_ADTAB();
    }

    void initialize_OPTAB()
    {
        OPTAB["STOP"] = 0;
        OPTAB["ADD"] = 1;
        OPTAB["SUB"] = 2;
        OPTAB["MULT"] = 3;
        OPTAB["MOVER"] = 4;
        OPTAB["MOVEM"] = 5;
        OPTAB["COMP"] = 6;
        OPTAB["BC"] = 7;
        OPTAB["DIV"] = 8;
        OPTAB["READ"] = 9;
        OPTAB["PRINT"] = 10;
    }

    void initialize_REGTAB()
    {
        REGTAB["AREG"] = 1;
        REGTAB["BREG"] = 2;
        REGTAB["CREG"] = 3;
        REGTAB["DREG"] = 4;
    }

    void initialize_CONDTAB()
    {
        CONDTAB["LT"] = 1;
        CONDTAB["LE"] = 2;
        CONDTAB["EQ"] = 3;
        CONDTAB["GT"] = 4;
        CONDTAB["GE"] = 5;
        CONDTAB["ANY"] = 6;
    }

    void initialize_ADTAB()
    {
        ADTAB["START"] = 1;
        ADTAB["END"] = 2;
        ADTAB["ORIGIN"] = 3;
        ADTAB["EQU"] = 4;
        ADTAB["LTORG"] = 5;
    }

    int search_symbol(const string &s) // Find index of s in SYMTAB,
    {
        for (size_t i = 0; i < SYMTAB.size(); i++)
        {
            if (SYMTAB[i].sym == s)
                return (int)i;
        }
        return -1; // if not found
    }

    int add_symbol(const string &s)
    {
        int idx = search_symbol(s);
        if (idx == -1)
        {
            Symbol sym;
            sym.sym = s;
            sym.addr = -1;
            SYMTAB.push_back(sym);
            return (int)SYMTAB.size() - 1;
        }
        return idx;
    }

    int search_literal(const string &lit)
    {
        for (size_t i = 0; i < LITTAB.size(); i++)
        {
            if (LITTAB[i].lit == lit)
                return (int)i; // if found
        }
        return -1; // not found
    }

    void add_literal(const string &lit) // add literal if not present
    {
        if (search_literal(lit) == -1)
        {
            Literal l;
            l.lit = lit;
            l.addr = -1;
            LITTAB.push_back(l);
            litcnt++;
        }
    }

    void allocate_literals(ofstream &ft)
    {
        if (proc_lit < litcnt)
        {
            POOLTAB.push_back(proc_lit + 1);
        }
        for (size_t j = proc_lit; j < litcnt; j++)
        {
            LITTAB[j].addr = lc;
            lc++;
        }
        proc_lit = litcnt;
    }
    vector<string> tokenize(const string &line) // Splits a line into tokens.
    {
        vector<string> tokens;   // result
        string temp;             // hold the temp tokens
        istringstream iss(line); // spliting by space
        while (iss >> temp)      // ex: ADD AREG='5' then first iteration temp=ADD, 2nd temp=AREG
        {
            // split tokens by comma if comma present
            size_t start = 0, pos;

            while ((pos = temp.find(',', start)) != string::npos) // checks if temp has , in it if it does then pos has index where comma occured in AGEG, here pos=4 where comma came
            {
                if (pos > start)                                       // true
                    tokens.push_back(temp.substr(start, pos - start)); // add operand before , means AGER therefore 4-0 = 4(End index/no.character) in substr(0,4) start from 0 take 4 character from 0
                start = pos + 1;                                       // move to next operand after comma therefore 4+1
            }
            if (start < temp.size())                  // if ADD no comma its size=3 that is 0<3
                tokens.push_back(temp.substr(start)); // temp.substr(0) means 0-temp->end
        }
        return tokens;
    }

    void pass_one(const string &filename) // main file process everthing and generate intermediate code
    {
        ifstream source(filename);
        if (!source.is_open()) // open file
        {
            cout << "Source file not found\n";
            return;
        }
        ofstream icfile("ic.txt"); // creating a file to write IC
        if (!icfile.is_open())
        {
            cout << "Cannot create intermediate code file\n";
            return;
        }

        string line;
        while (getline(source, line)) // read file line by line
        {
            if (line.empty()) // if empty skip
                continue;
            vector<string> tokens = tokenize(line); // tokenizing each line
            int n = (int)tokens.size();             // token added like ['ADD','AREG','=5']

            if (n == 0) // if no token skip
                continue;

            if (n == 1) // stop ltorg end
            {
                // Case 1: Single token instructions like STOP, END, LTORG
                if (OPTAB.count(tokens[0]) && OPTAB[tokens[0]] == 0) // STOP fisrt check it is 0 or not if yes then check ==
                {
                    icfile << "(IS,00)\n";
                    lc++;
                    continue;
                }
                if (ADTAB.count(tokens[0]) && (ADTAB[tokens[0]] == 2 || ADTAB[tokens[0]] == 5))
                { // END or LTORG
                    allocate_literals(icfile);
                    icfile << "(AD," << setw(2) << setfill('0') << ADTAB[tokens[0]] << ")\n";
                    continue;
                }
            }
            else if (n == 2) // START 100
            {
                // Two token instructions
                if (ADTAB.count(tokens[0]) && (ADTAB[tokens[0]] == 1 || ADTAB[tokens[0]] == 3))
                { // START or ORIGIN
                    lc = stoi(tokens[1]);
                    icfile << "(AD," << setw(2) << setfill('0') << ADTAB[tokens[0]] << ") (C," << tokens[1] << ")\n";
                    continue;
                }
                if (OPTAB.count(tokens[0]) && (OPTAB[tokens[0]] == 9 || OPTAB[tokens[0]] == 10))
                { // READ or PRINT
                    int p = search_symbol(tokens[1]);
                    if (p == -1)
                    {
                        p = add_symbol(tokens[1]); // check for symbol table
                    }
                    icfile << "(IS," << setw(2) << setfill('0') << OPTAB[tokens[0]] << ") (S," << setw(2) << setfill('0') << (p + 1) << ")\n";
                    lc++;
                    continue;
                }
                if (OPTAB.count(tokens[1]) && OPTAB[tokens[1]] == 0)
                { // If second token is STOP
                    int p = search_symbol(tokens[0]);
                    if (p == -1)
                        p = add_symbol(tokens[0]);
                    SYMTAB[p].addr = lc;
                    icfile << "(IS,00)\n";
                    lc++;
                    continue;
                }
            }
            else if (n == 3)
            {
                // Handle Instructions with Label or Declarative statements
                // see always 0 index would have operand value like add,sub,mover
                if (OPTAB.count(tokens[0]) && OPTAB[tokens[0]] >= 1 && OPTAB[tokens[0]] <= 8) //.count() checks if it exists in OPTAB return 1 if true, also check value between 1 -8
                {
                    lc++;
                    // always 1 index woud have either condition branch or register
                    int k = (tokens[0] == "BC") ? CONDTAB[tokens[1]] : REGTAB[tokens[1]]; // so here if BC return BC opcode if register like AREg,BREg then returns its opcode
                    if (tokens[2].substr(0, 2) == "='")                                   // if index og token[2] 0->= and 1->' then literal
                    {                                                                     // Literal
                        std::string lit = tokens[2].substr(2, tokens[2].length() - 3);    //='52' substr(2-> skip =,' start from 5, token.length=5-3(since skip =,',')) therefore substr(2,2) from 5 take 2 character including 5
                        add_literal(lit);
                        icfile << "(IS," << setw(2) << setfill('0') << OPTAB[tokens[0]] << ") (" << k << ")(L," << setw(2) << setfill('0') << litcnt << ")\n";
                    }
                    else
                    { // Symbol
                        int p = search_symbol(tokens[2]);
                        if (p == -1)
                            p = add_symbol(tokens[2]);
                        icfile << "(IS," << setw(2) << setfill('0') << OPTAB[tokens[0]] << ") (" << k << ")(S," << setw(2) << setfill('0') << (p + 1) << ")\n";
                    }
                    continue;
                }
                if (tokens[1] == "DS")
                {
                    int p = search_symbol(tokens[0]);
                    if (p == -1)
                        p = add_symbol(tokens[0]);
                    SYMTAB[p].addr = lc;
                    icfile << "(DL,02) (C," << tokens[2] << ")\n";
                    lc += stoi(tokens[2]);
                    continue;
                }
                if (tokens[1] == "DC")
                {
                    int p = search_symbol(tokens[0]);
                    if (p == -1)
                        p = add_symbol(tokens[0]);
                    SYMTAB[p].addr = lc;
                    icfile << "(DL,01) (C," << tokens[2] << ")\n";
                    lc++;
                    continue;
                }
                if (ADTAB.count(tokens[1]) && ADTAB[tokens[1]] == 4)
                { // EQU
                    int p = search_symbol(tokens[0]);
                    int q = search_symbol(tokens[2]);
                    if (p == -1)
                        p = add_symbol(tokens[0]);
                    if (q != -1 && SYMTAB[q].addr != -1)
                        SYMTAB[p].addr = SYMTAB[q].addr;
                    continue;
                }
            }
            else if (n == 4)
            {
                // Label + instruction + reg + operand
                if (OPTAB.count(tokens[1]) && OPTAB[tokens[1]] >= 1 && OPTAB[tokens[1]] <= 8)
                {
                    int p = search_symbol(tokens[0]);
                    if (p == -1)
                        p = add_symbol(tokens[0]);
                    SYMTAB[p].addr = lc;
                    lc++;

                    int k = (tokens[1] == "BC") ? CONDTAB[tokens[2]] : REGTAB[tokens[2]];
                    if (tokens[3].substr(0, 2) == "='")
                    {
                        std::string lit = tokens[3].substr(2, tokens[3].length() - 3);
                        add_literal(lit);
                        icfile << "(IS," << setw(2) << setfill('0') << OPTAB[tokens[1]] << ") (" << k << ")(L," << setw(2) << setfill('0') << litcnt << ")\n";
                    }
                    else
                    {
                        int q = search_symbol(tokens[3]);
                        if (q == -1)
                            q = add_symbol(tokens[3]);
                        icfile << "(IS," << setw(2) << setfill('0') << OPTAB[tokens[1]] << ") (" << k << ")(S," << setw(2) << setfill('0') << (q + 1) << ")\n";
                    }
                }
            }
        }

        // Assign address to any literals remaining at the end
        allocate_literals(icfile);

        source.close();
        icfile.close();

        print_source(filename);
        print_symbol_table();
        print_literal_table();
        print_intermediate_code();
        print_pool_table();
    }

    void print_source(const string &filename)
    {
        ifstream src(filename);
        cout << "Source code:\n";
        string line;
        while (getline(src, line))
        {
            cout << line << "\n";
        }
        cout << "\n";
        src.close();
    }

    void print_symbol_table()
    {
        cout << "Symbol Table:\n";
        cout << setw(10) << "Symbol" << setw(10) << "Address\n";
        for (const auto &sym : SYMTAB)
        {
            cout << setw(10) << sym.sym << setw(10) << sym.addr << "\n";
        }
        std::cout << "\n";
    }

    void print_literal_table()
    {
        cout << "Literal Table:\n";
        cout << setw(10) << "Literal" << setw(10) << "Address\n";
        for (const auto &lit : LITTAB)
        {
            cout << setw(10) << lit.lit << setw(10) << lit.addr << "\n";
        }
        cout << "\n";
    }

    void print_pool_table()
    {
        cout << "Pool Table:\n";
        cout << (10) << "Index\n";
        for (auto idx : POOLTAB)
        {
            cout << setw(10) << idx << "\n";
        }
        cout << "\n";
    }

    void print_intermediate_code()
    {
        ifstream icfile("ic.txt");
        cout << "Intermediate Code:\n";
        string line;
        while (getline(icfile, line))
        {
            cout << line << "\n";
        }
        cout << "\n";
        icfile.close();
    }
};

int main()
{
    Pass1 assembler;
    assembler.pass_one("source.txt");
    return 0;
}
