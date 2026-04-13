#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>


using Symbol     = std::string;
using Production = std::vector<Symbol>;   
using Gramatica    = std::map<Symbol, std::vector<Production>>;

const Symbol EPSILON = "ε";
const Symbol END_SYM = "$";


bool esTerminal(const Symbol& s, const Gramatica& g)
{
    return g.find(s) == g.end() && s != EPSILON;
}


//  FIRST(X)
std::map<Symbol, std::set<Symbol>> computeFirst(const Gramatica& g)
{
    std::map<Symbol, std::set<Symbol>> first;

    // Inicializar con terminales: FIRST(a) = {a}
    for (auto& [nt, prods] : g)
        for (auto& prod : prods)
            for (auto& sym : prod)
                if (esTerminal(sym, g))
                    first[sym].insert(sym);

    // Inicializar no-terminales vacíos
    for (auto& [nt, _] : g)
        if (first.find(nt) == first.end())
            first[nt] = {};

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& [A, prods] : g) {
            for (auto& prod : prods) {
                // Producción A -> ε
                if (prod.size() == 1 && prod[0] == EPSILON) {
                    if (!first[A].count(EPSILON)) {
                        first[A].insert(EPSILON);
                        changed = true;
                    }
                    continue;
                }

                // Producción A -> Y1 Y2 ... Yk
                bool allDeriveEps = true;
                for (auto& Y : prod) {
                    // Agregar FIRST(Y) - {ε} a FIRST(A)
                    for (auto& t : first[Y]) {
                        if (t != EPSILON && !first[A].count(t)) {
                            first[A].insert(t);
                            changed = true;
                        }
                    }
                    // Si Y no puede derivar ε, detenerse
                    if (!first[Y].count(EPSILON)) {
                        allDeriveEps = false;
                        break;
                    }
                }
                
                // Si todos los Yi pueden derivar ε
                if (allDeriveEps) {
                    if (!first[A].count(EPSILON)) {
                        first[A].insert(EPSILON);
                        changed = true;
                    }
                }
            }
        }
    }
    return first;
}

// FIRST de una cadena de símbolos α  (usado en la tabla)
std::set<Symbol> firstOfString(
    const Production& alpha,
    const std::map<Symbol, std::set<Symbol>>& first)
{
    std::set<Symbol> result;
    bool allEps = true;
    for (auto& Y : alpha) {
        auto it = first.find(Y);
        if (it == first.end()) {
            // Terminal no listado (no debería ocurrir si first está completo)
            result.insert(Y);
            allEps = false;
            break;
        }
        for (auto& t : it->second)
            if (t != EPSILON) result.insert(t);
        if (!it->second.count(EPSILON)) {
            allEps = false;
            break;
        }
    }
    if (allEps) result.insert(EPSILON);
    return result;
}


//  FOLLOW(A)
std::map<Symbol, std::set<Symbol>> computeFollow(
    const Gramatica& g,
    const Symbol& startSymbol,
    const std::map<Symbol, std::set<Symbol>>& first)
{
    std::map<Symbol, std::set<Symbol>> follow;

    // Inicializar todos los no-terminales
    for (auto& [nt, _] : g)
        follow[nt] = {};

    // Regla 1: $ ∈ FOLLOW(S)
    follow[startSymbol].insert(END_SYM);

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& [A, prods] : g) {
            for (auto& prod : prods) {
                for (int i = 0; i < (int)prod.size(); ++i) {
                    Symbol B = prod[i];
                    if (esTerminal(B, g) || B == EPSILON) continue;

                    // β = prod[i+1 .. end]
                    Production beta(prod.begin() + i + 1, prod.end());

                    if (!beta.empty()) {
                        // Regla 2: agregar FIRST(β) - {ε} a FOLLOW(B)
                        auto fb = firstOfString(beta, first);
                        for (auto& t : fb) {
                            if (t != EPSILON && !follow[B].count(t)) {
                                follow[B].insert(t);
                                changed = true;
                            }
                        }
                        // Si ε ∈ FIRST(β), agregar FOLLOW(A) a FOLLOW(B)
                        if (fb.count(EPSILON)) {
                            for (auto& t : follow[A]) {
                                if (!follow[B].count(t)) {
                                    follow[B].insert(t);
                                    changed = true;
                                }
                            }
                        }
                    } else {
                        // β vacío: regla 3 — agregar FOLLOW(A) a FOLLOW(B)
                        for (auto& t : follow[A]) {
                            if (!follow[B].count(t)) {
                                follow[B].insert(t);
                                changed = true;
                            }
                        }
                    }
                }
            }
        }
    }
    return follow;
}

// ──────────────────────────────────────────────────────────────
//  Tabla de Análisis Sintáctico Predictivo  
//  M[A, x]  ->  producción a aplicar
// ──────────────────────────────────────────────────────────────
struct TableEntry {
    std::vector<std::string> productions;  // puede haber conflicto (>1)
};

using ParseTable = std::map<Symbol, std::map<Symbol, TableEntry>>;

// Convierte una producción a string legible: "A -> α"
std::string prodToStr(const Symbol& lhs, const Production& rhs)
{
    std::string s = lhs + " -> ";
    for (auto& sym : rhs) s += sym;
    return s;
}

ParseTable buildParseTable(
    const Gramatica& g,
    const Symbol& startSymbol,
    const std::map<Symbol, std::set<Symbol>>& first,
    const std::map<Symbol, std::set<Symbol>>& follow)
{
    ParseTable M;

    for (auto& [A, prods] : g) {
        for (auto& prod : prods) {
            // Calcular FIRST(α)
            std::set<Symbol> firstAlpha;
            if (prod.size() == 1 && prod[0] == EPSILON) {
                firstAlpha.insert(EPSILON);
            } else {
                firstAlpha = firstOfString(prod, first);
            }

            // Para cada terminal a ∈ FIRST(α) - {ε}: M[A,a] = A -> α
            for (auto& a : firstAlpha) {
                if (a != EPSILON) {
                    M[A][a].productions.push_back(prodToStr(A, prod));
                }
            }

            // Si ε ∈ FIRST(α): para cada b ∈ FOLLOW(A): M[A,b] = A -> α
            if (firstAlpha.count(EPSILON)) {
                auto it = follow.find(A);
                if (it != follow.end()) {
                    for (auto& b : it->second) {
                        M[A][b].productions.push_back(prodToStr(A, prod));
                    }
                }
            }
        }
    }
    return M;
}


//  Impresión de resultados
void printSet(const std::string& name, const std::set<Symbol>& s)
{
    std::cout << name << " = { ";
    bool first = true;
    for (auto& x : s) { if (!first) std::cout << ", "; std::cout << x; first=false; }
    std::cout << " }\n";
}

void printFirst(const Gramatica& g,
                const std::map<Symbol, std::set<Symbol>>& first)
{

    std::cout << "Conjuntos FIRST: \n";
    for (auto& [nt, _] : g)
        printSet("  FIRST(" + nt + ")", first.at(nt));
}

void printFollow(const Gramatica& g,
                 const std::map<Symbol, std::set<Symbol>>& follow)
{
    std::cout << "Conjuntos FOLLOW: \n";
    for (auto& [nt, _] : g)
        printSet("  FOLLOW(" + nt + ")", follow.at(nt));
}

void printTable(const Gramatica& g,
                const ParseTable& M,
                const std::vector<Symbol>& terminals)
{
    std::cout << "Tabla de Análisis Sintáctico Predictivo\n";

    // Orden de no-terminales
    std::vector<Symbol> nts;
    for (auto& [nt, _] : g) nts.push_back(nt);

    int col = 18;
    // Encabezado
    std::cout << std::setw(6) << " ";
    for (auto& t : terminals) std::cout << std::setw(col) << t;
    std::cout << "\n" << std::string(6 + col * terminals.size(), '-') << "\n";

    for (auto& A : nts) {
        std::cout << std::setw(5) << A << " |";
        for (auto& t : terminals) {
            auto itA = M.find(A);
            std::string cell = "";
            if (itA != M.end()) {
                auto itT = itA->second.find(t);
                if (itT != itA->second.end() && !itT->second.productions.empty())
                    cell = itT->second.productions[0];
            }
            std::cout << std::setw(col) << cell;
        }
        std::cout << "\n";
    }
}

bool checkLL1(const ParseTable& M)
{
    bool isLL1 = true;
    std::cout << "Verificación LL(1): \n";
    for (auto& [A, row] : M) {
        for (auto& [t, entry] : row) {
            if (entry.productions.size() > 1) {
                isLL1 = false;
                std::cout << "  CONFLICTO en M[" << A << ", " << t << "]:\n";
                for (auto& p : entry.productions)
                    std::cout << "    " << p << "\n";
            }
        }
    }
    if (isLL1) std::cout << "La gramática ES LL(1).\n";
    else        std::cout << "La gramática NO es LL(1).\n";
    return isLL1;
}


int main()
{
    std::cout << "============================================================\n";
    std::cout << "  Tabla de Análisis Sintáctico Predictivo \n";
    std::cout << "  Gramática: Expresiones Aritméticas\n";
    std::cout << "============================================================\n";

    //  Definición de la gramática 1
    //   E  -> T E'
    //   E' -> + T E' | ε
    //   T  -> F T'
    //   T' -> * F T' | ε
    //   F  -> ( E ) | id
    Gramatica g;
    g["E"]  = { {"T", "E'"} };
    g["E'"] = { {"+", "T", "E'"}, {EPSILON} };
    g["T"]  = { {"F", "T'"} };
    g["T'"] = { {"*", "F", "T'"}, {EPSILON} };
    g["F"]  = { {"(", "E", ")"}, {"id"} };

    Symbol startSymbol = "E";

    // Terminales para la tabla (en el orden deseado)
    std::vector<Symbol> terminals = {"id", "+", "*", "(", ")", END_SYM};

    //  Algoritmos 
    auto first  = computeFirst(g);
    auto follow = computeFollow(g, startSymbol, first);
    auto table  = buildParseTable(g, startSymbol, first, follow);

    //  Salida 
    printFirst(g, first);
    printFollow(g, follow);
    printTable(g, table, terminals);
    checkLL1(table);

    std::cout << "\n";

    // Definición de la gramática 2
    // S -> D S'
    // S' -> . D S' | ε
    // D -> 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9

    Gramatica g2;
    g2["S"]  = { {"D", "S'"} };
    g2["S'"] = { {".", "D", "S'"}, {EPSILON} };
    g2["D"]  = { {"1"}, {"2"}, {"3"}, {"4"}, {"5"}, {"6"}, {"7"}, {"8"}, {"9"} };

    Symbol startSymbol2 = "S";

    // Terminales para la tabla (en el orden deseado)
    std::vector<Symbol> terminals2 = {"1", "2", "3", "4", "5", "6", "7", "8", "9", ".", END_SYM};

    //  Algoritmos
    auto first2  = computeFirst(g2);
    auto follow2 = computeFollow(g2, startSymbol2, first2);
    auto table2  = buildParseTable(g2, startSymbol2, first2, follow2);

    //  Salida
    printFirst(g2, first2);
    printFollow(g2, follow2);
    printTable(g2, table2, terminals2);
    checkLL1(table2);

    // Definición de la gramática 3
    // S -> NP VP
    // NP -> the N | a N
    // N -> cat | dog
    // VP -> V NP
    // V -> chases | sees

    Gramatica g3;
    g3["S"]  = { {"NP", "VP"} };
    g3["NP"] = { {"the", "N"}, {"a", "N"} };
    g3["N"]  = { {"cat"}, {"dog"} };
    g3["VP"] = { {"V", "NP"} };
    g3["V"]  = { {"chases"}, {"sees"} };

    Symbol startSymbol3 = "S";
    // Terminales para la tabla (en el orden deseado)
    std::vector<Symbol> terminals3 = {"the", "a", "cat", "dog", "chases", "sees", END_SYM};
    //  Algoritmos
    auto first3  = computeFirst(g3);
    auto follow3 = computeFollow(g3, startSymbol3, first3);
    auto table3  = buildParseTable(g3, startSymbol3, first3, follow3);

    //  Salida
    printFirst(g3, first3);
    printFollow(g3, follow3);
    printTable(g3, table3, terminals3);
    checkLL1(table3);

    // Gramática 4 (con conflicto)
    // S -> A a | b A c | d c | b d
    // A -> d
    Gramatica g4;
    g4["S"] = { {"A", "a"}, {"b", "A", "c"}, {"d", "c"}, {"b", "d"} };
    g4["A"] = { {"d"} };

    Symbol startSymbol4 = "S";
    std::vector<Symbol> terminals4 = {"a", "b", "c", "d", END_SYM};
    auto first4  = computeFirst(g4);
    auto follow4 = computeFollow(g4, startSymbol4, first4);
    auto table4  = buildParseTable(g4, startSymbol4, first4, follow4);

    printFirst(g4, first4);
    printFollow(g4, follow4);
    printTable(g4, table4, terminals4);
    checkLL1(table4);


    return 0;
}
