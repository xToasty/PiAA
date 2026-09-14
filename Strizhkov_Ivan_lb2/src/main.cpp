#include <iostream>
#include <vector>
#include <queue>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <string>
#include <random>
#include <chrono>
#include <windows.h>

using namespace std;

const int INF = 1e9;

void printMatrix(const vector<vector<int>>& m) {
    int n = (int)m.size();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (m[i][j] >= INF) cout << setw(6) << "inf";
            else cout << setw(6) << m[i][j];
        }
        cout << "\n";
    }
}

bool hasTour(const vector<vector<int>>& cost) {
    int n = (int)cost.size();
    for (int i = 0; i < n; ++i) {
        bool out = false, in = false;
        for (int j = 0; j < n; ++j) {
            if (i != j && cost[i][j] < INF) out = true;
            if (i != j && cost[j][i] < INF) in = true;
        }
        if (!out || !in) return false;
    }
    return true;
}

void generateRandomMatrix(vector<vector<int>>& cost, int n, int maxW, unsigned seed) {
    mt19937 rng(seed);
    uniform_int_distribution<int> dist(1, maxW);
    cost.assign(n, vector<int>(n, INF));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i == j) cost[i][j] = INF;
            else cost[i][j] = dist(rng);
        }
    }
}

void generateSymmetricMatrix(vector<vector<int>>& cost, int n, int maxW, unsigned seed) {
    mt19937 rng(seed);
    uniform_int_distribution<int> dist(1, maxW);
    cost.assign(n, vector<int>(n, INF));
    for (int i = 0; i < n; ++i) {
        cost[i][i] = INF;
        for (int j = i + 1; j < n; ++j) {
            int w = dist(rng);
            cost[i][j] = w;
            cost[j][i] = w;
        }
    }
}

bool saveMatrixToFile(const vector<vector<int>>& cost, int startVertex,
                      const string& filename) {
    ofstream fout(filename);
    if (!fout) return false;
    int n = (int)cost.size();
    fout << n << "\n";
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (cost[i][j] >= INF) fout << -1;
            else fout << cost[i][j];
            if (j + 1 < n) fout << " ";
        }
        fout << "\n";
    }
    fout << startVertex << "\n";
    return true;
}

bool loadMatrixFromFile(vector<vector<int>>& cost, int& n, int& startVertex,
                        const string& filename) {
    ifstream fin(filename);
    if (!fin) return false;
    fin >> n;
    if (n < 2) return false;
    cost.assign(n, vector<int>(n, INF));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            int x;
            if (!(fin >> x)) return false;
            if (x < 0 || x >= INF) cost[i][j] = INF;
            else cost[i][j] = x;
        }
    }
    if (!(fin >> startVertex)) return false;
    if (startVertex < 0 || startVertex >= n) return false;
    return true;
}

bool saveResultToFile(const vector<int>& path, long long total,
                      const string& method, const string& filename) {
    ofstream fout(filename, ios::app);
    if (!fout) return false;
    fout << "=== " << method << " ===\n";
    if (path.empty() || total >= INF) {
        fout << "Решение не найдено.\n\n";
    } else {
        fout << "Путь: ";
        for (size_t i = 0; i < path.size(); ++i) {
            fout << path[i];
            if (i + 1 < path.size()) fout << " -> ";
        }
        fout << "\nСтоимость: " << total << "\n\n";
    }
    return true;
}

long long mstWeight(const vector<vector<int>>& cost,
                    const vector<int>& verts) {
    int k = (int)verts.size();
    if (k <= 1) return 0;

    vector<bool> used(k, false);
    vector<int> minEdge(k, INF);
    minEdge[0] = 0;
    long long total = 0;

    for (int iter = 0; iter < k; ++iter) {
        int v = -1;
        for (int i = 0; i < k; ++i)
            if (!used[i] && (v == -1 || minEdge[i] < minEdge[v]))
                v = i;
        used[v] = true;
        total += minEdge[v];
        for (int to = 0; to < k; ++to) {
            if (!used[to]) {
                int w = cost[verts[v]][verts[to]];
                if (w < minEdge[to]) minEdge[to] = w;
            }
        }
    }
    return total;
}

long long mstEstimate(const vector<vector<int>>& cost,
                      const vector<pair<int,int>>& path,
                      int n, int startVertex) {
    vector<bool> inPath(n, false);
    for (auto& e : path) inPath[e.first] = true;

    vector<int> verts;
    for (int i = 0; i < n; ++i) {
        if (!inPath[i]) verts.push_back(i);
    }

    if (verts.size() <= 1) return 0;
    return mstWeight(cost, verts);
}

struct Node {
    long long lowerBound;
    long long reducedCost;
    long long mstEst;
    int level;
    vector<vector<int>> matrix;
    vector<pair<int,int>> path;
    int startVertex;

    bool operator>(const Node& other) const {
        return lowerBound > other.lowerBound;
    }
};

long long reduceMatrix(vector<vector<int>>& m, int n) {
    long long cost = 0;

    for (int i = 0; i < n; ++i) {
        int mn = INF;
        for (int j = 0; j < n; ++j)
            if (m[i][j] < mn) mn = m[i][j];
        if (mn != INF && mn > 0) {
            cost += mn;
            for (int j = 0; j < n; ++j)
                if (m[i][j] < INF) m[i][j] -= mn;
        }
    }

    for (int j = 0; j < n; ++j) {
        int mn = INF;
        for (int i = 0; i < n; ++i)
            if (m[i][j] < mn) mn = m[i][j];
        if (mn != INF && mn > 0) {
            cost += mn;
            for (int i = 0; i < n; ++i)
                if (m[i][j] < INF) m[i][j] -= mn;
        }
    }
    return cost;
}

vector<int> buildOrder(const vector<pair<int,int>>& path,
                       int n, int startVertex) {
    if ((int)path.size() != n - 1) return {};

    vector<vector<int>> next(n);
    vector<int> degIn(n, 0), degOut(n, 0);
    for (auto& e : path) {
        next[e.first].push_back(e.second);
        degOut[e.first]++;
        degIn[e.second]++;
    }

    int begin = -1;
    for (int i = 0; i < n; ++i) {
        if (degIn[i] == 0) { begin = i; break; }
    }
    if (begin == -1) return {};

    vector<int> order;
    vector<bool> used(n, false);
    int v = begin;
    for (int i = 0; i < n; ++i) {
        if (used[v]) return {};
        if (degOut[v] > 1) return {};
        order.push_back(v);
        used[v] = true;
        if (i == n - 1) break;
        if (next[v].empty()) return {};
        v = next[v][0];
    }

    for (int i = 0; i < n; ++i) if (!used[i]) return {};

    int pos = -1;
    for (int i = 0; i < n; ++i) if (order[i] == startVertex) { pos = i; break; }
    if (pos == -1) return {};

    vector<int> rotated;
    rotated.reserve(n);
    for (int i = 0; i < n; ++i)
        rotated.push_back(order[(pos + i) % n]);

    return rotated;
}

pair<vector<int>, long long> littleAlgorithm(const vector<vector<int>>& costForLittle,
                                             const vector<vector<int>>& costReal,
                                             int startVertex) {
    int n = (int)costForLittle.size();
    long long bestCost = INF;
    vector<int> bestPath;

    Node root;
    root.matrix = costForLittle;
    root.level = 0;
    root.startVertex = startVertex;
    root.reducedCost = reduceMatrix(root.matrix, n);
    root.mstEst = mstEstimate(root.matrix, root.path, n, startVertex);
    root.lowerBound = root.reducedCost;

    priority_queue<Node, vector<Node>, greater<Node>> pq;
    pq.push(root);

    while (!pq.empty()) {
        Node cur = pq.top(); pq.pop();

        if (cur.lowerBound >= bestCost) continue;

        if (cur.level == n - 1) {
            vector<int> degIn(n, 0), degOut(n, 0);
            for (auto& e : cur.path) {
                degOut[e.first]++;
                degIn[e.second]++;
            }

            int beginVertex = -1, endVertex = -1;
            for (int i = 0; i < n; ++i) {
                if (degIn[i] == 0)  beginVertex = i;
                if (degOut[i] == 0) endVertex = i;
            }
            if (beginVertex == -1 || endVertex == -1) continue;

            int w = costReal[endVertex][beginVertex];
            if (w >= INF) continue;

            long long total = 0;
            for (auto& e : cur.path) total += costReal[e.first][e.second];
            total += w;

            if (total < bestCost) {
                vector<int> ord = buildOrder(cur.path, n, startVertex);
                if (!ord.empty()) {
                    bestCost = total;
                    bestPath = ord;
                    bestPath.push_back(startVertex);
                }
            }
            continue;
        }

        int bi = -1, bj = -1;
        int bestPenalty = -1;
        long long bestChildMst = -1;
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == j) continue;
                if (cur.matrix[i][j] != 0) continue;

                int rowMin = INF;
                for (int k = 0; k < n; ++k)
                    if (k != j && cur.matrix[i][k] < rowMin)
                        rowMin = cur.matrix[i][k];
                int colMin = INF;
                for (int k = 0; k < n; ++k)
                    if (k != i && cur.matrix[k][j] < colMin)
                        colMin = cur.matrix[k][j];

                int penalty = (rowMin >= INF ? 0 : rowMin) +
                              (colMin >= INF ? 0 : colMin);
                long long childMst = cur.mstEst;

                if (penalty > bestPenalty ||
                    (penalty == bestPenalty && childMst > bestChildMst)) {
                    bestPenalty = penalty;
                    bestChildMst = childMst;
                    bi = i; bj = j;
                }
            }
        }

        if (bi == -1) continue;

        {
            Node child = cur;
            child.level++;
            child.path.push_back({bi, bj});

            child.matrix[bj][bi] = INF;
            for (int k = 0; k < n; ++k) {
                child.matrix[bi][k] = INF;
                child.matrix[k][bj] = INF;
            }
            child.matrix[bi][bj] = INF;

            long long r = reduceMatrix(child.matrix, n);
            child.reducedCost += r;

            child.mstEst = mstEstimate(child.matrix, child.path, n, startVertex);
            child.lowerBound = child.reducedCost;

            if (child.lowerBound < bestCost)
                pq.push(child);
        }

        {
            Node child = cur;
            child.matrix[bi][bj] = INF;
            long long r = reduceMatrix(child.matrix, n);
            child.reducedCost += r;

            child.mstEst = mstEstimate(child.matrix, child.path, n, startVertex);
            child.lowerBound = child.reducedCost;

            if (child.lowerBound < bestCost)
                pq.push(child);
        }
    }

    return {bestPath, bestCost};
}

pair<vector<int>, long long> nearestNeighbor(const vector<vector<int>>& cost,
                                              int startVertex) {
    int n = (int)cost.size();
    vector<bool> visited(n, false);
    vector<int> path;
    long long total = 0;

    int cur = startVertex;
    visited[cur] = true;
    path.push_back(cur);

    for (int step = 1; step < n; ++step) {
        int next = -1;
        int bestW = INF;
        for (int j = 0; j < n; ++j) {
            if (!visited[j] && cost[cur][j] < bestW) {
                bestW = cost[cur][j];
                next = j;
            }
        }
        if (next == -1) return {{}, INF};
        total += bestW;
        visited[next] = true;
        path.push_back(next);
        cur = next;
    }

    if (cost[cur][startVertex] >= INF) return {{}, INF};
    total += cost[cur][startVertex];
    path.push_back(startVertex);

    return {path, total};
}

int askN() {
    int n;
    cout << "Введите количество вершин (>= 2): ";
    while (!(cin >> n) || n < 2) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Ошибка. Введите целое число >= 2: ";
    }
    return n;
}

int askStart(int n) {
    int s;
    cout << "Введите стартовую вершину (0.." << n - 1 << "): ";
    while (!(cin >> s) || s < 0 || s >= n) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Ошибка. Введите число от 0 до " << n - 1 << ": ";
    }
    return s;
}

string askFilename(const string& prompt, const string& def) {
    cout << prompt << " (по умолчанию " << def << "): ";
    string s;
    cin >> s;
    if (s.empty()) s = def;
    return s;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    vector<vector<int>> cost;
    int n = 0;
    int startVertex = 0;

    cout << "Выберите способ ввода:\n";
    cout << "  1 — ввод с клавиатуры\n";
    cout << "  2 — чтение из файла\n";
    cout << "  3 — генерация произвольной (несимметричной) матрицы\n";
    cout << "  4 — генерация симметричной матрицы\n";
    cout << "Ваш выбор: ";

    int mode = 0;
    while (!(cin >> mode) || mode < 1 || mode > 4) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Ошибка. Введите 1, 2, 3 или 4: ";
    }

    if (mode == 1) {
        n = askN();
        cost.assign(n, vector<int>(n));
        cout << "Введите матрицу весов (" << n << "x" << n << ").\n";
        cout << "Диагональ можно вводить как 0 или -1. "
             << "Для отсутствия ребра: -1 или большое число.\n";
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                int x;
                if (!(cin >> x)) {
                    cout << "Ошибка чтения элемента ["
                         << i << "][" << j << "].\n";
                    return 1;
                }
                if (x < 0 || x >= INF) cost[i][j] = INF;
                else cost[i][j] = x;
            }
        }
        for (int i = 0; i < n; ++i) cost[i][i] = INF;
        startVertex = askStart(n);
    }
    else if (mode == 2) {
        string filename = askFilename("Введите имя файла для чтения", "matrix.txt");
        if (!loadMatrixFromFile(cost, n, startVertex, filename)) {
            cout << "Ошибка: не удалось прочитать файл " << filename << "\n";
            return 1;
        }
        cout << "Файл успешно прочитан: n = " << n
             << ", старт = " << startVertex << "\n";
    }
    else if (mode == 3 || mode == 4) {
        n = askN();
        int maxW;
        cout << "Введите максимальный вес (например, 100): ";
        while (!(cin >> maxW) || maxW < 1) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Ошибка. Введите целое число >= 1: ";
        }
        unsigned seed = (unsigned)chrono::system_clock::now().time_since_epoch().count();
        if (mode == 3) {
            generateRandomMatrix(cost, n, maxW, seed);
            cout << "Сгенерирована произвольная матрица (seed = " << seed << ")\n";
        } else {
            generateSymmetricMatrix(cost, n, maxW, seed);
            cout << "Сгенерирована симметричная матрица (seed = " << seed << ")\n";
        }
        startVertex = askStart(n);
    }

    cout << "\n=== Исходная матрица ===\n";
    printMatrix(cost);

    cout << "\nСохранить матрицу в файл? (y/n): ";
    char ans;
    cin >> ans;
    if (ans == 'y' || ans == 'Y') {
        string filename = askFilename("Имя файла для сохранения", "matrix.txt");
        if (saveMatrixToFile(cost, startVertex, filename))
            cout << "Матрица сохранена в " << filename << "\n";
        else
            cout << "Ошибка сохранения.\n";
    }

    if (!hasTour(cost)) {
        cout << "\nТур коммивояжёра не существует "
             << "(у какой-то вершины нет входящего/исходящего ребра).\n";
        return 0;
    }

    vector<vector<int>> costForLittle = cost;

    cout << "\n=== МВиГ (Литтл + МОД-эвристика) ===\n";
    auto res1 = littleAlgorithm(costForLittle, cost, startVertex);
    const auto& path1 = res1.first;
    long long cost1 = res1.second;

    if (path1.empty() || cost1 >= INF) {
        cout << "Решение не найдено.\n";
    } else {
        cout << "Путь: ";
        for (size_t i = 0; i < path1.size(); ++i) {
            cout << path1[i];
            if (i + 1 < path1.size()) cout << " -> ";
        }
        cout << "\nСтоимость: " << cost1 << "\n";
    }

    cout << "\n=== Приближённый алгоритм АБС (ближайший сосед) ===\n";
    auto res2 = nearestNeighbor(cost, startVertex);
    const auto& path2 = res2.first;
    long long cost2 = res2.second;

    if (path2.empty() || cost2 >= INF) {
        cout << "Решение не найдено.\n";
    } else {
        cout << "Путь: ";
        for (size_t i = 0; i < path2.size(); ++i) {
            cout << path2[i];
            if (i + 1 < path2.size()) cout << " -> ";
        }
        cout << "\nСтоимость: " << cost2 << "\n";
    }

    cout << "\nСохранить результаты в файл? (y/n): ";
    cin >> ans;
    if (ans == 'y' || ans == 'Y') {
        string filename = askFilename("Имя файла для результатов", "results.txt");
        saveResultToFile(path1, cost1, "МВиГ (Литтл + МОД-эвристика)", filename);
        saveResultToFile(path2, cost2, "АБС (ближайший сосед)", filename);
        cout << "Результаты сохранены в " << filename << "\n";
    }

    return 0;
}