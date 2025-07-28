#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <iomanip>

struct Symbol
{
    std::string sym;
    int addr = -1;
};

struct Literal
{
    std::string lit;
    int addr = -1;
};

class Pass1
{
    std::unordered_map<std::string, int> OPTAB, REGTAB, CONDTAB, ADTAB;
    std::vector<Symbol> SYMTAB;
    std::vector<Literal> LITTAB;

    int lc = 0, litcnt = 0, proc_lit = 0;

public:
    Pass1()
    {
        initialize_OPTAB();
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

    int search_symbol(const std::string &s)
    {
        for (size_t i = 0; i < SYMTAB.size(); i++)
        {
            if (SYMTAB[i].sym == s)
                return (int)i;
        }
        return -1;
    }

    int add_symbol(const std::string &s)
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

    int search_literal(const std::string &lit)
    {
        for (size_t i = 0; i < LITTAB.size(); i++)
        {
            if (LITTAB[i].lit == lit)
                return (int)i;
        }
        return -1;
    }

    void add_literal(const std::string &lit)
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

    std::vector<std::string> tokenize(const std::string &line)
    {
        std::vector<std::string> tokens;
        std::string temp;
        std::istringstream iss(line);
        while (iss >> temp) // ex: ADD AREG='5' then first iteration temp=ADD, 2nd temp=AGER
        {
            // split tokens by comma if comma present
            size_t start = 0, pos;

            while ((pos = temp.find(',', start)) != std::string::npos) // checks if temp has , in it if it does then pos has index where comma occured in AGEG, here pos=4 where comma came
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

    void allocate_literals(std::ofstream &ft)
    {
        // Assign addresses to literals not yet assigned
        for (size_t j = proc_lit; j < litcnt; j++)
        {
            LITTAB[j].addr = lc;
            lc++;
        }
        proc_lit = litcnt;
    }

    void pass_one(const std::string &filename)
    {
        std::ifstream source(filename);
        if (!source.is_open())
        {
            std::cerr << "Source file not found\n";
            return;
        }
        std::ofstream icfile("ic.txt");
        if (!icfile.is_open())
        {
            std::cerr << "Cannot create intermediate code file\n";
            return;
        }

        std::string line;
        while (getline(source, line))
        {
            if (line.empty())
                continue;
            std::vector<std::string> tokens = tokenize(line);
            int n = (int)tokens.size(); // token added like ['ADD','AREG','=5']

            if (n == 0)
                continue;

            if (n == 1)
            {
                // Case 1: Single token instructions like STOP, END, LTORG
                if (OPTAB.count(tokens[0]) && OPTAB[tokens[0]] == 0)
                { // STOP
                    icfile << "(IS,00)\n";
                    lc++;
                    continue;
                }
                if (ADTAB.count(tokens[0]) && (ADTAB[tokens[0]] == 2 || ADTAB[tokens[0]] == 5))
                { // END or LTORG
                    allocate_literals(icfile);
                    icfile << "(AD," << std::setw(2) << std::setfill('0') << ADTAB[tokens[0]] << ")\n";
                    continue;
                }
            }
            else if (n == 2)
            {
                // Two token instructions
                if (ADTAB.count(tokens[0]) && (ADTAB[tokens[0]] == 1 || ADTAB[tokens[0]] == 3))
                { // START or ORIGIN
                    lc = stoi(tokens[1]);
                    icfile << "(AD," << std::setw(2) << std::setfill('0') << ADTAB[tokens[0]] << ") (C," << tokens[1] << ")\n";
                    continue;
                }
                if (OPTAB.count(tokens[0]) && (OPTAB[tokens[0]] == 9 || OPTAB[tokens[0]] == 10))
                { // READ or PRINT
                    int p = search_symbol(tokens[1]);
                    if (p == -1)
                    {
                        p = add_symbol(tokens[1]);
                    }
                    icfile << "(IS," << std::setw(2) << std::setfill('0') << OPTAB[tokens[0]] << ") (S," << std::setw(2) << std::setfill('0') << (p + 1) << ")\n";
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
                        icfile << "(IS," << std::setw(2) << std::setfill('0') << OPTAB[tokens[0]] << ") (" << k << ")(L," << std::setw(2) << std::setfill('0') << litcnt << ")\n";
                    }
                    else
                    { // Symbol
                        int p = search_symbol(tokens[2]);
                        if (p == -1)
                            p = add_symbol(tokens[2]);
                        icfile << "(IS," << std::setw(2) << std::setfill('0') << OPTAB[tokens[0]] << ") (" << k << ")(S," << std::setw(2) << std::setfill('0') << (p + 1) << ")\n";
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
                        icfile << "(IS," << std::setw(2) << std::setfill('0') << OPTAB[tokens[1]] << ") (" << k << ")(L," << std::setw(2) << std::setfill('0') << litcnt << ")\n";
                    }
                    else
                    {
                        int q = search_symbol(tokens[3]);
                        if (q == -1)
                            q = add_symbol(tokens[3]);
                        icfile << "(IS," << std::setw(2) << std::setfill('0') << OPTAB[tokens[1]] << ") (" << k << ")(S," << std::setw(2) << std::setfill('0') << (q + 1) << ")\n";
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
    }

    void print_source(const std::string &filename)
    {
        std::ifstream src(filename);
        std::cout << "Source code:\n";
        std::string line;
        while (getline(src, line))
        {
            std::cout << line << "\n";
        }
        std::cout << "\n";
        src.close();
    }

    void print_symbol_table()
    {
        std::cout << "Symbol Table:\n";
        std::cout << std::setw(10) << "Symbol" << std::setw(10) << "Address\n";
        for (const auto &sym : SYMTAB)
        {
            std::cout << std::setw(10) << sym.sym << std::setw(10) << sym.addr << "\n";
        }
        std::cout << "\n";
    }

    void print_literal_table()
    {
        std::cout << "Literal Table:\n";
        std::cout << std::setw(10) << "Literal" << std::setw(10) << "Address\n";
        for (const auto &lit : LITTAB)
        {
            std::cout << std::setw(10) << lit.lit << std::setw(10) << lit.addr << "\n";
        }
        std::cout << "\n";
    }

    void print_intermediate_code()
    {
        std::ifstream icfile("ic.txt");
        std::cout << "Intermediate Code:\n";
        std::string line;
        while (getline(icfile, line))
        {
            std::cout << line << "\n";
        }
        std::cout << "\n";
        icfile.close();
    }
};

int main()
{
    Pass1 assembler;
    assembler.pass_one("source.txt");
    return 0;
}
