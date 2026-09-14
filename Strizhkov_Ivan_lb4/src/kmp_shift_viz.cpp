#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

enum Algorithm { ALG_KMP = 0, ALG_SHIFT = 1 };

static Algorithm gAlg = ALG_KMP;

static std::wstring gP, gT;          // для KMP
static std::wstring gA, gB;          // для Shift
static std::wstring gTop, gBottom;
static std::vector<int> gPi;         // префикс-функция
static std::vector<int> gFound;      // найденные позиции (KMP) или одна (Shift)

// Текущее состояние анимации
static int gI = -1;                  // указатель по верхней строке
static int gJ = -1;                  // указатель по нижней строке
static int gPiI = -1;                // текущий индекс при построении pi
static int gPiJ = -1;
static int gPiStep = -1;             // 0 — строим pi, 1 — ищем
static bool gRunning = false;
static bool gFinished = false;
static int gLastMatch = -1;          // индекс только что совпавшего символа (для подсветки)
static int gLastMismatch = -1;       // индекс несовпавшего
static bool gFallback = false;       // был ли откат по pi на последнем шаге
static int gFallbackTo = -1;         // к чему откатились
static std::wstring gAction = L"";
static int gResult = -2;             // -2 — не посчитано

// Контролы
static HWND hRadioKMP = nullptr;
static HWND hRadioShift = nullptr;
static HWND hLabel1 = nullptr;
static HWND hLabel2 = nullptr;
static HWND hEdit1 = nullptr;
static HWND hEdit2 = nullptr;
static HWND hButton = nullptr;
static HWND hResult = nullptr;
static HWND hAction = nullptr;

static HFONT hFontUI = nullptr;
static HFONT hFontCell = nullptr;
static HFONT hFontSmall = nullptr;

static HBRUSH hBrushResultBg = nullptr;
static HBRUSH hBrushActionBg = nullptr;

static const int CELL_W = 40;
static const int CELL_H = 34;
static const int TOP_PANEL_H = 200;

#define ID_RADIO_KMP   201
#define ID_RADIO_SHIFT 202
#define ID_EDIT_1      203
#define ID_EDIT_2      204
#define ID_BTN         205
#define ID_TIMER       1

// ---------------- Построение pi (общая логика) ----------------
// Полностью повторяет kmp.cpp: pi[0] = 0, идём i = 1..n-1
static std::vector<int> buildPi(const std::wstring& P) {
    int n = (int)P.size();
    std::vector<int> pi(n, 0);
    for (int i = 1, j = 0; i < n; i++) {
        while (j > 0 && P[i] != P[j]) {
            j = pi[j - 1];
        }
        if (P[i] == P[j]) {
            j++;
        }
        pi[i] = j;
    }
    return pi;
}

static void resetViz() {
    gI = gJ = -1;
    gPiI = gPiJ = -1;
    gPiStep = -1;
    gRunning = false;
    gFinished = false;
    gLastMatch = -1;
    gLastMismatch = -1;
    gFallback = false;
    gFallbackTo = -1;
    gAction = L"";
    gFound.clear();
    gResult = -2;
}

static void startKMP() {
    resetViz();
    gTop = gT;
    gBottom = gP;
    gPi = buildPi(gP);
    gPiStep = 0;
    gPiI = 1;
    gPiJ = 0;
    gRunning = true;
    gAction = L"Построение префикс-функции для P…";
}

static void startShift() {
    resetViz();
    gTop = gA + gA;
    gBottom = gB;
    gPi = buildPi(gB);
    gPiStep = 0;
    gPiI = 1;
    gPiJ = 0;
    gRunning = true;
    gAction = L"Построение префикс-функции для B…";
}

static void stepOnce(HWND hwnd) {
    if (!gRunning) return;

    int n = (int)gBottom.size();
    int m = (int)gTop.size();

    // -------- Постройка pi --------
    if (gPiStep == 0) {
        if (gPiI >= n) {
            gPiStep = 1;
            gI = 0;
            gJ = 0;
            gPiI = gPiJ = -1;
            gLastMatch = gLastMismatch = -1;
            gFallback = false;
            gFallbackTo = -1;
            gAction = L"pi построена. Начинаем поиск…";
            InvalidateRect(hwnd, nullptr, TRUE);
            return;
        }

        int i = gPiI;
        int j = gPiJ;

        if (gBottom[i] == gBottom[j]) {
            j++;
            gLastMatch = i;
            gLastMismatch = -1;
            gFallback = false;
            gAction = L"pi[" + std::to_wstring(i) + L"] = " + std::to_wstring(j);
        } else {
            if (j > 0) {
                int oldJ = j;
                j = gPi[j - 1];
                gLastMismatch = i;
                gFallback = true;
                gFallbackTo = j;
                gAction = L"Несовпадение, откат j: " + std::to_wstring(oldJ) +
                          L" → " + std::to_wstring(j);
            } else {
                gLastMismatch = i;
                gFallback = false;
                gAction = L"Несовпадение, j = 0, pi[" + std::to_wstring(i) + L"] = 0";
            }
        }

        // Записываем pi, если символы совпали или j = 0
        if (gBottom[i] == gBottom[gPiJ] || gPiJ == 0) {
            gPi[i] = j;
        }
        gPiJ = j;
        gPiI = i + 1;
        InvalidateRect(hwnd, nullptr, TRUE);
        return;
    }

    // -------- Фаза 2: поиск --------
    if (gPiStep == 1) {
        if (gI >= m) {
            gRunning = false;
            gFinished = true;

            // KMP: результат — список позиций
            if (gAlg == ALG_KMP) {
                if (gFound.empty()) {
                    gResult = -1;
                    SetWindowTextW(hResult, L"Результат: -1");
                } else {
                    std::wstring s = L"Результат: ";
                    for (size_t k = 0; k < gFound.size(); ++k) {
                        if (k) s += L",";
                        s += std::to_wstring(gFound[k]);
                    }
                    SetWindowTextW(hResult, s.c_str());
                }
            } else {
                // Shift: результат — startIdx
                if (gResult == -2) {
                    gResult = -1;
                    SetWindowTextW(hResult, L"Результат: -1");
                } else {
                    SetWindowTextW(hResult,
                        (L"Результат: " + std::to_wstring(gResult)).c_str());
                }
            }
            gAction = L"Готово.";
            InvalidateRect(hwnd, nullptr, TRUE);
            return;
        }

        wchar_t c = gTop[gI];

        if (c == gBottom[gJ]) {
            gJ++;
            gLastMatch = gI;
            gLastMismatch = -1;
            gFallback = false;
            gAction = L"Совпадение, j = " + std::to_wstring(gJ);
        } else {
            if (gJ > 0) {
                int oldJ = gJ;
                gJ = gPi[gJ - 1];
                gLastMismatch = gI;
                gFallback = true;
                gFallbackTo = gJ;
                gAction = L"Несовпадение, откат j: " + std::to_wstring(oldJ) +
                          L" → " + std::to_wstring(gJ);
                InvalidateRect(hwnd, nullptr, TRUE);
                return; // повторить сравнение на i
            } else {
                gLastMismatch = gI;
                gFallback = false;
                gAction = L"Несовпадение, j = 0";
            }
        }

        if (gJ == n) {
            int startIdx = gI - n + 1;

            if (gAlg == ALG_KMP) {
                gFound.push_back(startIdx);
                gAction = L"Найдено вхождение на позиции " + std::to_wstring(startIdx);
                gJ = gPi[gJ - 1]; //в kmp.cpp
            } else {
                // Shift: проверяем startIdx < n
                if (startIdx < (int)gA.size()) {
                    gResult = startIdx;
                    gRunning = false;
                    gFinished = true;
                    SetWindowTextW(hResult,
                        (L"Результат: " + std::to_wstring(startIdx)).c_str());
                    gAction = L"Найден сдвиг: " + std::to_wstring(startIdx);
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return;
                }
                gJ = gPi[gJ - 1];
            }
        }

        gI++;
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}

// ---------------- Цвета ячеек ----------------
static COLORREF topCellColor(int idx) {
    if (idx == gI && gPiStep == 1) return RGB(255, 235, 150);        // текущий i
    if (idx == gLastMatch) return RGB(180, 240, 180);                // совпало
    if (idx == gLastMismatch) return RGB(255, 180, 180);             // не совпало
    if (gPiStep == 1 && idx < gI) return RGB(235, 235, 235);         // уже пройдено
    return RGB(255, 255, 255);
}

static COLORREF bottomCellColor(int idx) {
    if (idx == gJ && gPiStep == 1) return RGB(255, 235, 150);        // текущий j
    if (idx == gPiJ && gPiStep == 0) return RGB(255, 235, 150);      // при построении pi
    if (idx == gLastMatch) return RGB(180, 240, 180);
    if (idx == gLastMismatch) return RGB(255, 180, 180);
    if (idx == gFallbackTo) return RGB(180, 200, 255);               // куда откатились
    return RGB(255, 255, 255);
}

// ---------------- Отрисовка ----------------
static void drawStripe(HDC hdc, const std::wstring& s, int x, int y,
                       COLORREF (*colorFn)(int), bool showPiValues,
                       int piOffset) {
    int n = (int)s.size();
    for (int k = 0; k < n; ++k) {
        int cx = x + k * CELL_W;
        RECT r{ cx, y, cx + CELL_W, y + CELL_H };

        COLORREF bg = colorFn(k);
        HBRUSH br = CreateSolidBrush(bg);
        FillRect(hdc, &r, br);
        DeleteObject(br);
        FrameRect(hdc, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));

        // Буква
        std::wstring lbl(1, s[k]);
        COLORREF old = SetTextColor(hdc, RGB(0, 0, 0));
        DrawTextW(hdc, lbl.c_str(), -1, &r,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SetTextColor(hdc, old);

        // Значение pi над ячейкой (для нижней строки)
        if (showPiValues && piOffset >= 0 && k < (int)gPi.size()) {
            RECT rp{ cx, y - 22, cx + CELL_W, y - 2 };
            HFONT hOld = (HFONT)SelectObject(hdc, hFontSmall);
            SetTextColor(hdc, RGB(60, 60, 60));
            std::wstring pv = std::to_wstring(gPi[k]);
            DrawTextW(hdc, pv.c_str(), -1, &rp,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SetTextColor(hdc, RGB(0, 0, 0));
            SelectObject(hdc, hOld);
        }
    }
}

static void drawViz(HDC hdc, const RECT& rc) {
    if (gTop.empty() && gBottom.empty()) return;

    HFONT hOldFont = (HFONT)SelectObject(hdc, hFontCell);
    SetBkMode(hdc, TRANSPARENT);

    int nTop = (int)gTop.size();
    int nBot = (int)gBottom.size();

    int totalW = std::max(nTop, nBot) * CELL_W;
    int baseX = (rc.right - totalW) / 2;
    if (baseX < 20) baseX = 20;

    int yTop = TOP_PANEL_H + 30;
    int yBot = yTop + CELL_H + 50;

    // Подпись «Текст» / «Образец»
    HFONT hOld = (HFONT)SelectObject(hdc, hFontUI);
    SetTextColor(hdc, RGB(80, 80, 80));
    {
        RECT r{ 20, yTop, 100, yTop + CELL_H };
        std::wstring lbl = (gAlg == ALG_KMP) ? L"T:" : L"A+A:";
        DrawTextW(hdc, lbl.c_str(), -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    {
        RECT r{ 20, yBot, 100, yBot + CELL_H };
        std::wstring lbl = (gAlg == ALG_KMP) ? L"P:" : L"B:";
        DrawTextW(hdc, lbl.c_str(), -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    SelectObject(hdc, hOld);

    // Верхняя полоса (T / A+A)
    SelectObject(hdc, hFontCell);
    drawStripe(hdc, gTop, baseX, yTop, topCellColor, false, -1);

    // Нижняя полоса (P / B) с pi сверху
    drawStripe(hdc, gBottom, baseX, yBot, bottomCellColor, true, 0);

    // Стрелки-указатели i и j
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(200, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    if (gPiStep == 1 && gI >= 0 && gI < nTop) {
        int cx = baseX + gI * CELL_W + CELL_W / 2;
        int y1 = yTop - 6;
        int y2 = yTop - 20;
        MoveToEx(hdc, cx, y1, nullptr);
        LineTo(hdc, cx, y2);
        LineTo(hdc, cx - 5, y2 + 6);
        MoveToEx(hdc, cx, y2, nullptr);
        LineTo(hdc, cx + 5, y2 + 6);
    }
    if (gPiStep == 1 && gJ >= 0 && gJ < nBot) {
        int cx = baseX + gJ * CELL_W + CELL_W / 2;
        int y1 = yBot + CELL_H + 6;
        int y2 = yBot + CELL_H + 20;
        MoveToEx(hdc, cx, y1, nullptr);
        LineTo(hdc, cx, y2);
        LineTo(hdc, cx - 5, y2 - 6);
        MoveToEx(hdc, cx, y2, nullptr);
        LineTo(hdc, cx + 5, y2 - 6);
    }
    if (gPiStep == 0 && gPiI >= 0 && gPiI < nBot) {
        int cx = baseX + gPiI * CELL_W + CELL_W / 2;
        int y1 = yBot + CELL_H + 6;
        int y2 = yBot + CELL_H + 20;
        MoveToEx(hdc, cx, y1, nullptr);
        LineTo(hdc, cx, y2);
        LineTo(hdc, cx - 5, y2 - 6);
        MoveToEx(hdc, cx, y2, nullptr);
        LineTo(hdc, cx + 5, y2 - 6);
    }
    if (gPiStep == 0 && gPiJ >= 0 && gPiJ < nBot) {
        int cx = baseX + gPiJ * CELL_W + CELL_W / 2;
        int y1 = yBot + CELL_H + 6;
        int y2 = yBot + CELL_H + 20;
        MoveToEx(hdc, cx, y1, nullptr);
        LineTo(hdc, cx, y2);
        LineTo(hdc, cx - 5, y2 - 6);
        MoveToEx(hdc, cx, y2, nullptr);
        LineTo(hdc, cx + 5, y2 - 6);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    if (gAlg == ALG_KMP && !gFound.empty()) {
        HPEN hPen2 = CreatePen(PS_SOLID, 3, RGB(0, 150, 0));
        HPEN hOldPen2 = (HPEN)SelectObject(hdc, hPen2);
        HBRUSH hNull = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hNull);

        for (int pos : gFound) {
            for (int k = 0; k < nBot; ++k) {
                int idx = pos + k;
                if (idx < 0 || idx >= nTop) continue;
                int cx = baseX + idx * CELL_W;
                Rectangle(hdc, cx + 2, yTop + 2, cx + CELL_W - 2, yTop + CELL_H - 2);
            }
        }
        SelectObject(hdc, hOldBr);
        SelectObject(hdc, hOldPen2);
        DeleteObject(hPen2);
    }

    if (gAlg == ALG_SHIFT && gResult >= 0) {
        HPEN hPen2 = CreatePen(PS_SOLID, 3, RGB(0, 150, 0));
        HPEN hOldPen2 = (HPEN)SelectObject(hdc, hPen2);
        HBRUSH hNull = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hNull);

        for (int k = 0; k < nBot; ++k) {
            int idx = gResult + k;
            if (idx < 0 || idx >= nTop) continue;
            int cx = baseX + idx * CELL_W;
            Rectangle(hdc, cx + 2, yTop + 2, cx + CELL_W - 2, yTop + CELL_H - 2);
        }
        SelectObject(hdc, hOldBr);
        SelectObject(hdc, hOldPen2);
        DeleteObject(hPen2);
    }

    SelectObject(hdc, hOldFont);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE: {
        hFontUI = CreateFontW(
            -16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        hFontCell = CreateFontW(
            -18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Consolas");
        hFontSmall = CreateFontW(
            -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        hBrushResultBg = CreateSolidBrush(RGB(245, 245, 245));
        hBrushActionBg = CreateSolidBrush(RGB(235, 240, 250));

        // Radio
        hRadioKMP = CreateWindowW(L"BUTTON", L"KMP (поиск всех вхождений P в T)",
                                  WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                                  20, 10, 280, 24, hwnd, (HMENU)ID_RADIO_KMP, nullptr, nullptr);
        hRadioShift = CreateWindowW(L"BUTTON", L"Shift (является ли B циклическим сдвигом A)",
                                    WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                    20, 36, 320, 24, hwnd, (HMENU)ID_RADIO_SHIFT, nullptr, nullptr);
        SendMessageW(hRadioKMP, BM_SETCHECK, BST_CHECKED, 0);

        // Метки и поля
        hLabel1 = CreateWindowW(L"STATIC", L"P (образец):",
                                WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                                20, 70, 110, 26, hwnd, nullptr, nullptr, nullptr);
        hLabel2 = CreateWindowW(L"STATIC", L"T (текст):",
                                WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                                20, 102, 110, 26, hwnd, nullptr, nullptr, nullptr);

        hEdit1 = CreateWindowW(L"EDIT", L"",
                               WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                               140, 70, 260, 26, hwnd, (HMENU)ID_EDIT_1, nullptr, nullptr);
        hEdit2 = CreateWindowW(L"EDIT", L"",
                               WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                               140, 102, 420, 26, hwnd, (HMENU)ID_EDIT_2, nullptr, nullptr);

        hButton = CreateWindowW(L"BUTTON", L"Вычислить",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                600, 70, 140, 58, hwnd, (HMENU)ID_BTN, nullptr, nullptr);

        hResult = CreateWindowW(L"STATIC", L"Результат: —",
                                WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_LEFT,
                                20, 140, 400, 24, hwnd, nullptr, nullptr, nullptr);

        hAction = CreateWindowW(L"STATIC", L"",
                                WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_LEFT,
                                20, 168, 700, 24, hwnd, nullptr, nullptr, nullptr);

        SendMessageW(hRadioKMP, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hRadioShift, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hLabel1, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hLabel2, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hEdit1, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hEdit2, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hButton, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hResult, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hAction, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HWND hCtl = (HWND)lParam;
        if (hCtl == hResult) {
            SetBkColor((HDC)wParam, RGB(245, 245, 245));
            return (LRESULT)hBrushResultBg;
        }
        if (hCtl == hAction) {
            SetBkColor((HDC)wParam, RGB(235, 240, 250));
            return (LRESULT)hBrushActionBg;
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == ID_RADIO_KMP && code == BN_CLICKED) {
            gAlg = ALG_KMP;
            SetWindowTextW(hLabel1, L"P (образец):");
            SetWindowTextW(hLabel2, L"T (текст):");
            break;
        }
        if (id == ID_RADIO_SHIFT && code == BN_CLICKED) {
            gAlg = ALG_SHIFT;
            SetWindowTextW(hLabel1, L"A:");
            SetWindowTextW(hLabel2, L"B:");
            break;
        }

        if (id == ID_BTN && code == BN_CLICKED) {
            KillTimer(hwnd, ID_TIMER);

            wchar_t buf1[512] = {0};
            wchar_t buf2[1024] = {0};
            GetWindowTextW(hEdit1, buf1, 511);
            GetWindowTextW(hEdit2, buf2, 1023);

            std::wstring s1 = buf1;
            std::wstring s2 = buf2;

            if (gAlg == ALG_KMP) {
                gP = s1;
                gT = s2;
                if (gP.empty()) {
                    SetWindowTextW(hResult, L"Результат: -1");
                    SetWindowTextW(hAction, L"Образец пуст.");
                    break;
                }
                startKMP();
            } else {
                gA = s1;
                gB = s2;
                if (gA.size() != gB.size() || gA.empty()) {
                    SetWindowTextW(hResult, L"Результат: -1");
                    SetWindowTextW(hAction, L"Длины не равны или пусто.");
                    break;
                }
                startShift();
            }

            SetWindowTextW(hResult, L"Результат: вычисляется…");
            SetWindowTextW(hAction, gAction.c_str());

            InvalidateRect(hwnd, nullptr, TRUE);
            SetTimer(hwnd, ID_TIMER, 220, nullptr);
        }
        break;
    }

    case WM_TIMER: {
        if (wParam == ID_TIMER) {
            stepOnce(hwnd);
            SetWindowTextW(hAction, gAction.c_str());
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

        HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(memDC, &rc, bg);
        DeleteObject(bg);

        drawViz(memDC, rc);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY: {
        KillTimer(hwnd, ID_TIMER);
        if (hFontUI)        DeleteObject(hFontUI);
        if (hFontCell)      DeleteObject(hFontCell);
        if (hFontSmall)     DeleteObject(hFontSmall);
        if (hBrushResultBg) DeleteObject(hBrushResultBg);
        if (hBrushActionBg) DeleteObject(hBrushActionBg);
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"KMPShiftVizClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME,
        L"Визуализация KMP и циклического сдвига",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1150, 720,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}