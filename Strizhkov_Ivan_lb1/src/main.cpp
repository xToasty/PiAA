#include <iostream>
#include <vector>
#include <algorithm>
#include <bit>
#include <iomanip>

using namespace std;

struct Tile {
    int r, c, w;
};

int N, M;
int min_k = 100;
int solutions_count = 0;
vector<Tile> best_solution;
vector<Tile> current_solution;
unsigned int board[22];

void toggle(int r, int c, int w) {
    unsigned int mask = ((1U << w) - 1) << c;
    for (int i = 0; i < w; ++i) {
        board[r + i] ^= mask;
    }
}

bool can_place(int r, int c, int w) {
    if (r + w > N || c + w > M) return false;
    unsigned int mask = ((1U << w) - 1) << c;
    for (int i = 0; i < w; ++i) {
        if (board[r + i] & mask) return false;
    }
    return true;
}

void solve(int count, int r) {
    if (count >= min_k) return;

    unsigned int full_mask = (1U << M) - 1;
    while (r < N && board[r] == full_mask) {
        r++;
    }

    if (r == N) {
        if (count < min_k) {
            min_k = count;
            solutions_count = 1;
            best_solution = current_solution;
        } else if (count == min_k) {
            solutions_count++;
        }
        return;
    }

    int c = std::countr_zero((~board[r]) & full_mask);

    int max_s = min(N - r, M - c);
    
    for (int s = max_s; s >= 1; --s) {
        if (can_place(r, c, s)) {
            toggle(r, c, s);
            current_solution.push_back({r + 1, c + 1, s});

            solve(count + 1, r);

            current_solution.pop_back();
            toggle(r, c, s);
        }
    }
}

void print_visualization() {
    // Создаем матрицу для хранения размеров квадратов
    vector<vector<int>> grid(N, vector<int>(M, 0));
    
    // Заполняем матрицу из best_solution
    for (const auto& tile : best_solution) {
        int r = tile.r - 1;
        int c = tile.c - 1;
        int w = tile.w;
        for (int i = 0; i < w; i++) {
            for (int j = 0; j < w; j++) {
                grid[r + i][c + j] = w;
            }
        }
    }
    
    // Выводим визуализацию
    cout << "\n==========================================\n";
    cout << "ВИЗУАЛИЗАЦИЯ ПОКРЫТИЯ (поле " << N << "x" << M << "):\n";
    cout << "==========================================\n\n";
    
    // Верхняя граница
    cout << "   ";
    for (int j = 0; j < M; j++) {
        cout << "+-------";
    }
    cout << "+\n";
    
    // Выводим строки
    for (int i = 0; i < N; i++) {
        cout << " " << i + 1 << " ";
        for (int j = 0; j < M; j++) {
            if (grid[i][j] > 0) {
                cout << "|  " << grid[i][j] << "x" << grid[i][j] << "  ";
            } else {
                cout << "|       ";
            }
        }
        cout << "|\n";
        
        // Разделитель между строками
        cout << "   ";
        for (int j = 0; j < M; j++) {
            cout << "+-------";
        }
        cout << "+\n";
    }
    
    cout << "\n";
}

void print_solution_detailed() {
    cout << "\n==========================================\n";
    cout << "РЕЗУЛЬТАТЫ РЕШЕНИЯ\n";
    cout << "==========================================\n";
    cout << "Размер поля: " << N << " × " << M << "\n";
    cout << "Минимальное количество квадратов: " << min_k << "\n";
    cout << "Количество различных минимальных покрытий: " << solutions_count << "\n";
    
    cout << "\n------------------------------------------\n";
    cout << "СПИСОК КВАДРАТОВ (одно из решений):\n";
    cout << "------------------------------------------\n";
    
    int num = 1;
    for (const auto& tile : best_solution) {
        cout << "Квадрат " << num++ << ": ";
        cout << "размер " << tile.w << "×" << tile.w << ", ";
        cout << "верхний левый угол в (строка " << tile.r << ", столбец " << tile.c << ")\n";
    }
    
    print_visualization();
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    system("chcp 65001 > nul");
    
    if (cin >> N >> M) {
        min_k = N * M;
        solutions_count = 0;
        for (int i = 0; i < 22; ++i) board[i] = 0;
        
        solve(0, 0);
        
        print_solution_detailed();
    } else {
        cout << "Ошибка ввода!\n";
    }
    
    return 0;
}