#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <set>
#include <map>
#include <windows.h>

using namespace std;

const int ALPHABET_SIZE = 5;

int getCharIndex(char c) {
    switch (c) {
        case 'A': return 0;
        case 'C': return 1;
        case 'G': return 2;
        case 'T': return 3;
        case 'N': return 4;
        default: return -1;
    }
}

char getCharFromIndex(int idx) {
    switch (idx) {
        case 0: return 'A';
        case 1: return 'C';
        case 2: return 'G';
        case 3: return 'T';
        case 4: return 'N';
        default: return '?';
    }
}

struct Node {
    int next[ALPHABET_SIZE];
    int link;
    int go[ALPHABET_SIZE];
    vector<int> part_indices;

    Node() {
        fill(begin(next), end(next), -1);
        fill(begin(go), end(go), -1);
        link = 0;
    }
};

vector<Node> trie;
vector<int> part_start_pos;
vector<int> part_length;
vector<string> part_strings;

void addPart(const string& s, int start_pos) {
    int v = 0;
    for (char ch : s) {
        int c = getCharIndex(ch);
        if (c == -1) return;
        if (trie[v].next[c] == -1) {
            trie[v].next[c] = trie.size();
            trie.emplace_back();
        }
        v = trie[v].next[c];
    }
    trie[v].part_indices.push_back(part_start_pos.size());
    part_start_pos.push_back(start_pos);
    part_length.push_back(s.length());
    part_strings.push_back(s);
}

void buildLinks() {
    queue<int> q;
    trie[0].link = 0;
    for (int i = 0; i < ALPHABET_SIZE; ++i) {
        if (trie[0].next[i] != -1) {
            int u = trie[0].next[i];
            trie[u].link = 0;
            trie[0].go[i] = u;
            q.push(u);
        } else {
            trie[0].go[i] = 0;
        }
    }

    while (!q.empty()) {
        int v = q.front();
        q.pop();

        for (int i = 0; i < ALPHABET_SIZE; ++i) {
            if (trie[v].next[i] != -1) {
                int u = trie[v].next[i];
                trie[u].link = trie[trie[v].link].go[i];
                q.push(u);
            } else {
                trie[v].go[i] = trie[trie[v].link].go[i];
            }
        }
    }
}

void printTrie() {
    cout << "\n=== СТРУКТУРА БОРА ===\n";
    for (int i = 0; i < (int)trie.size(); ++i) {
        cout << "Вершина " << i << ": ";
        bool hasEdge = false;
        for (int c = 0; c < ALPHABET_SIZE; ++c) {
            if (trie[i].next[c] != -1) {
                cout << getCharFromIndex(c) << "->" << trie[i].next[c] << " ";
                hasEdge = true;
            }
        }
        if (!hasEdge) cout << "нет исходящих ребер";

        if (!trie[i].part_indices.empty()) {
            cout << " [ТЕРМИНАЛ: ";
            for (int idx : trie[i].part_indices) {
                cout << "\"" << part_strings[idx] << "\" ";
            }
            cout << "]";
        }
        cout << "\n";
    }
}

void printAutomaton() {
    cout << "\n=== АВТОМАТ (СУФФИКСНЫЕ ССЫЛКИ И ПЕРЕХОДЫ) ===\n";
    for (int i = 0; i < (int)trie.size(); ++i) {
        cout << "Вершина " << i << ": link=" << trie[i].link << " | Переходы: ";
        for (int c = 0; c < ALPHABET_SIZE; ++c) {
            cout << getCharFromIndex(c) << "->" << trie[i].go[c] << " ";
        }
        cout << "\n";
    }
}

// Функция проверки одного кандидата с подробным выводом
bool checkCandidate(const string& T, const string& P, char wildcard,
                    char forbidden_char, int pos, bool verbose) {
    if (pos < 0 || pos + (int)P.length() > (int)T.length()) {
        if (verbose) cout << "  Кандидат pos=" << pos + 1 << " отброшен: выход за границы текста\n";
        return false;
    }

    if (verbose) cout << "  Проверка кандидата pos=" << pos + 1 << ":\n";

    for (int j = 0; j < (int)P.length(); ++j) {
        if (P[j] == wildcard) {
            // Джокер не может совпадать с forbidden_char
            if (T[pos + j] == forbidden_char) {
                if (verbose) cout << "    Позиция " << j << ": джокер совпал с запрещенным символом '"
                                  << forbidden_char << "' -> не подходит\n";
                return false;
            }
            if (verbose) cout << "    Позиция " << j << ": джокер на '" << T[pos + j]
                              << "' (не запрещен) -> OK\n";
        } else {
            if (P[j] != T[pos + j]) {
                if (verbose) cout << "    Позиция " << j << ": '" << P[j]
                                  << "' != '" << T[pos + j] << "' -> не подходит\n";
                return false;
            }
            if (verbose) cout << "    Позиция " << j << ": '" << P[j]
                              << "' == '" << T[pos + j] << "' -> OK\n";
        }
    }
    if (verbose) cout << "    >>> Совпадение найдено!\n";
    return true;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    string T, P;
    char wildcard, forbidden_char;

    cout << "Введите текст T: ";
    if (!getline(cin, T)) return 0;

    cout << "Введите шаблон P: ";
    if (!getline(cin, P)) return 0;

    cout << "Введите символ джокера: ";
    cin >> wildcard;

    cout << "Введите запрещенный символ для джокера: ";
    cin >> forbidden_char;

    trie.clear();
    trie.emplace_back();
    part_start_pos.clear();
    part_length.clear();
    part_strings.clear();

    string current_part;
    int start_idx = 0;
    for (int i = 0; i < (int)P.length(); ++i) {
        if (P[i] == wildcard) {
            if (!current_part.empty()) {
                addPart(current_part, start_idx);
                current_part.clear();
            }
            start_idx = i + 1;
        } else {
            current_part += P[i];
        }
    }
    if (!current_part.empty()) {
        addPart(current_part, start_idx);
    }

    cout << "\n=== РАЗБИЕНИЕ ШАБЛОНА ===\n";
    for (int i = 0; i < (int)part_strings.size(); ++i) {
        cout << "Часть " << i << ": \"" << part_strings[i]
             << "\" (начало в шаблоне: " << part_start_pos[i]
             << ", длина: " << part_length[i] << ")\n";
    }

    // Особый случай: в шаблоне нет обычных символов (только джокеры)
    if (part_start_pos.empty()) {
        cout << "\n=== В ШАБЛОНЕ ТОЛЬКО ДЖОКЕРЫ ===\n";
        vector<int> results;
        for (int i = 0; i <= (int)T.length() - (int)P.length(); ++i) {
            if (checkCandidate(T, P, wildcard, forbidden_char, i, true)) {
                results.push_back(i + 1);
            }
        }
        cout << "\n=== РЕЗУЛЬТАТЫ ПОИСКА ===\n";
        if (results.empty()) {
            cout << "Вхождений не найдено.\n";
        } else {
            for (int pos : results) cout << pos << "\n";
        }
        return 0;
    }

    buildLinks();
    printTrie();
    printAutomaton();

    cout << "\n=== ПРОЦЕСС ПОИСКА В ТЕКСТЕ ===\n";
    set<int> candidates;
    int v = 0;

    for (int i = 0; i < (int)T.length(); ++i) {
        int c = getCharIndex(T[i]);
        if (c == -1) {
            cout << "Шаг " << i + 1 << ": символ '" << T[i]
                 << "' не из алфавита -> сброс в корень\n";
            v = 0;
            continue;
        }

        int new_v = trie[v].go[c];
        cout << "Шаг " << i + 1 << ": символ '" << T[i]
             << "', переход " << v << " -> " << new_v << "\n";
        v = new_v;

        int temp = v;
        while (temp != 0) {
            for (int pid : trie[temp].part_indices) {
                int start_pos = i - (part_start_pos[pid] + part_length[pid]) + 1;
                if (start_pos >= 0) {
                    candidates.insert(start_pos);
                    cout << "  Найдена часть \"" << part_strings[pid]
                         << "\" (конец на позиции " << i + 1
                         << "), возможное начало шаблона: " << start_pos + 1 << "\n";
                } else {
                    cout << "  Найдена часть \"" << part_strings[pid]
                         << "\" (конец на позиции " << i + 1
                         << "), но начало шаблона выходит за границы ("
                         << start_pos + 1 << ") -> отброшено\n";
                }
            }
            temp = trie[temp].link;
        }
    }

    cout << "\n=== ПРОВЕРКА КАНДИДАТОВ ===\n";
    if (candidates.empty()) {
        cout << "Кандидатов не найдено.\n";
    }

    vector<int> results;
    for (int pos : candidates) {
        if (checkCandidate(T, P, wildcard, forbidden_char, pos, true)) {
            results.push_back(pos + 1);
        }
    }

    cout << "\n=== РЕЗУЛЬТАТЫ ПОИСКА ===\n";
    if (results.empty()) {
        cout << "Вхождений не найдено.\n";
    } else {
        sort(results.begin(), results.end());
        for (int pos : results) {
            cout << pos << "\n";
        }
    }

    return 0;
}