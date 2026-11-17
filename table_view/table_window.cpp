#define NOMINMAX
#include "table_window.h"
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

// ============================================================================
// Конструктор / деструктор
// ============================================================================
TableWindow::TableWindow() {
    //тестові дані
    m_model.SetCell(12, 24, L"УЛСЛЩ");
    HDC dc = GetDC(nullptr);
    int dpi = GetDeviceCaps(dc, LOGPIXELSY);

    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    m_font = CreateFontIndirectW(&ncm.lfMessageFont);

    HFONT old = (HFONT)SelectObject(dc, m_font);
    TEXTMETRICW tm;
    GetTextMetricsW(dc, &tm);
    SelectObject(dc, old);
    ReleaseDC(nullptr, dc);

    m_rowH    = tm.tmHeight + MulDiv(8, dpi, 96);
    m_defColW = MulDiv(150, dpi, 96);
    m_indexW  = MulDiv(50, dpi, 96);
}

TableWindow::~TableWindow() {
    if (m_font) DeleteObject(m_font);
}

PCWSTR TableWindow::GetClassName() const {
    return L"TableWindowClass";
}

bool TableWindow::Create(PCWSTR title, DWORD style, DWORD exStyle,
                         int x, int y, int width, int height,
                         HWND parent, HMENU menu)
{
    return Window::Create(title, style | WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN,
                          exStyle, x, y, width, height, parent, menu);
}

// ============================================================================
// Дані
// ============================================================================
// Припущення: GetSortIndices()[displayRow] = індекс запису в GetRecords().
int TableWindow::RecordIndex(int displayRow) const {
    const std::vector<int>& idx = const_cast<TableModel&>(m_model).GetSortIndices();
    if (displayRow >= 0 && (size_t)displayRow < idx.size())
        return idx[displayRow];
    return displayRow;
}

// Число у стовпці нумерації - це "рідний" номер запису (з одиниці),
// тому при сортуванні за стовпцем 0 за спаданням вони йдуть у зворотному порядку.
// Якщо потрібно показувати row_id замість позиції - поверніть тут records[ri].row_id.
int TableWindow::RowNumber(int displayRow) const {
    return RecordIndex(displayRow) + 1;
}

std::wstring TableWindow::GetCellText(int x, int y) {
    if (x <= 0) return L"";
    if (y == 0) {
        auto& headers = m_model.GetHeaders();
        return (size_t)(x - 1) < headers.size() ? headers[x - 1] : L"";
    }
    auto& recs = m_model.GetRecords();
    int ri = RecordIndex(y - 1);
    if (ri < 0 || (size_t)ri >= recs.size()) return L"";
    const auto& cells = recs[ri].cells;
    return (size_t)(x - 1) < cells.size() ? cells[x - 1] : L"";
}

// ============================================================================
// Розмітка
// ============================================================================
void TableWindow::SyncColumns() {
    int n = m_model.GetTableSize().first;
    m_colW.resize(n + 1, m_defColW);
    m_colW[0] = m_indexW;
}

int TableWindow::ContentWidth() {
    SyncColumns();
    int s = 0;
    for (size_t c = 1; c < m_colW.size(); ++c) s += m_colW[c];
    return s;
}

int TableWindow::ContentHeight() {
    return m_model.GetTableSize().second * m_rowH;
}

RECT TableWindow::CellRect(int x, int y) {
    SyncColumns();
    int offset = 0;
    for (int c = 1; c < x; ++c) offset += m_colW[c];

    RECT r;
    r.left   = (x == 0) ? 0 : m_indexW - m_scrollX + offset;
    r.right  = r.left + m_colW[x];
    r.top    = (y == 0) ? 0 : m_rowH - m_scrollY + (y - 1) * m_rowH;
    r.bottom = r.top + m_rowH;
    return r;
}

bool TableWindow::HitTest(int px, int py, int& x, int& y) {
    SyncColumns();
    auto size = m_model.GetTableSize();
    int nCols = size.first, nRows = size.second;

    // рядок
    if (py < m_rowH) {
        y = 0;
    } else {
        int r = (py - m_rowH + m_scrollY) / m_rowH;
        if (r >= nRows) return false;
        y = r + 1;
    }

    // колонка
    if (px < m_indexW) {
        x = 0;
    } else {
        int pos = px - m_indexW + m_scrollX;
        x = -1;
        for (int c = 1; c <= nCols; ++c) {
            if (pos < m_colW[c]) { x = c; break; }
            pos -= m_colW[c];
        }
        if (x < 0) return false;
    }
    return true;
}

// ============================================================================
// Скролінг
// ============================================================================
void TableWindow::UpdateScrollBars() {
    // Поява одного скролбара змінює клієнтську область, тому робимо 2 проходи
    for (int pass = 0; pass < 2; ++pass) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int viewW = std::max(0, (int)rc.right  - m_indexW);
        int viewH = std::max(0, (int)rc.bottom - m_rowH);
        int contentW = ContentWidth();
        int contentH = ContentHeight();

        m_scrollX = std::clamp(m_scrollX, 0, std::max(0, contentW - viewW));
        m_scrollY = std::clamp(m_scrollY, 0, std::max(0, contentH - viewH));

        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin   = 0;

        si.nMax  = std::max(0, contentW - 1);
        si.nPage = viewW;
        si.nPos  = m_scrollX;
        SetScrollInfo(m_hwnd, SB_HORZ, &si, TRUE);

        si.nMax  = std::max(0, contentH - 1);
        si.nPage = viewH;
        si.nPos  = m_scrollY;
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
    }
}

void TableWindow::ScrollTo(int newX, int newY) {
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int viewW = std::max(0, (int)rc.right  - m_indexW);
    int viewH = std::max(0, (int)rc.bottom - m_rowH);

    newX = std::clamp(newX, 0, std::max(0, ContentWidth()  - viewW));
    newY = std::clamp(newY, 0, std::max(0, ContentHeight() - viewH));
    if (newX == m_scrollX && newY == m_scrollY) return;

    EndEdit(true);
    m_scrollX = newX;
    m_scrollY = newY;
    SetScrollPos(m_hwnd, SB_HORZ, m_scrollX, TRUE);
    SetScrollPos(m_hwnd, SB_VERT, m_scrollY, TRUE);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void TableWindow::HandleScroll(bool horizontal, WPARAM wParam) {
    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask  = SIF_ALL;
    GetScrollInfo(m_hwnd, horizontal ? SB_HORZ : SB_VERT, &si);

    int pos = si.nPos;
    switch (LOWORD(wParam)) {
        case SB_LINEUP:        pos -= m_rowH;       break; // = SB_LINELEFT
        case SB_LINEDOWN:      pos += m_rowH;       break; // = SB_LINERIGHT
        case SB_PAGEUP:        pos -= (int)si.nPage; break;
        case SB_PAGEDOWN:      pos += (int)si.nPage; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: pos = si.nTrackPos;  break; // 32-бітна позиція
        case SB_TOP:           pos = 0;             break;
        case SB_BOTTOM:        pos = si.nMax;       break;
        default: return;
    }
    if (horizontal) ScrollTo(pos, m_scrollY);
    else            ScrollTo(m_scrollX, pos);
}

void TableWindow::EnsureVisible(int x, int y) {
    SyncColumns();
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int viewW = std::max(0, (int)rc.right  - m_indexW);
    int viewH = std::max(0, (int)rc.bottom - m_rowH);

    int nx = m_scrollX, ny = m_scrollY;

    if (x >= 1) {
        int left = 0;
        for (int c = 1; c < x; ++c) left += m_colW[c];
        int right = left + m_colW[x];
        if (left < nx)              nx = left;
        else if (right > nx + viewW) nx = right - viewW;
    }
    if (y >= 1) {
        int top = (y - 1) * m_rowH;
        int bottom = top + m_rowH;
        if (top < ny)               ny = top;
        else if (bottom > ny + viewH) ny = bottom - viewH;
    }
    ScrollTo(nx, ny);
}

// ============================================================================
// Сортування (ПКМ по заголовку)
// asc -> desc -> дефолт (стовпець 0 за зростанням)
// ============================================================================
void TableWindow::CycleSort(int col) {
    EndEdit(true);
    std::pair<bool, int> state = m_model.GetSortingState(); // {is_ascending, column}
    bool asc = state.first;
    int curCol = state.second;

    if (curCol == col) {
        if (asc) m_model.SortBy(col, false);
        else     m_model.SortBy(0, true);
    } else {
        m_model.SortBy(col, true);
    }
    m_selX = m_selY = -1; // позиції комірок змінилися
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

// ============================================================================
// Редагування комірки
// ============================================================================
void TableWindow::BeginEdit(int x, int y) {
    auto size = m_model.GetTableSize();
    if (x < 1 || x > size.first || y < 0 || y > size.second) return;

    EndEdit(true);
    EnsureVisible(x, y);

    RECT r = CellRect(x, y);
    m_editX = x;
    m_editY = y;

    std::wstring text = GetCellText(x, y);
    m_edit = CreateWindowExW(0, L"EDIT", text.c_str(),
                             WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT,
                             r.left, r.top, r.right - r.left, r.bottom - r.top,
                             m_hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
    if (!m_edit) return;

    SendMessageW(m_edit, WM_SETFONT, (WPARAM)m_font, TRUE);
    SetWindowSubclass(m_edit, EditSubclassProc, 1, (DWORD_PTR)this);
    SetFocus(m_edit);
    SendMessageW(m_edit, EM_SETSEL, 0, -1);
}

void TableWindow::EndEdit(bool commit) {
    if (!m_edit) return;

    HWND edit = m_edit;
    m_edit = nullptr; // спершу обнуляємо: WM_KILLFOCUS від DestroyWindow буде проігноровано
    bool hadFocus = (GetFocus() == edit);

    if (commit) {
        int len = GetWindowTextLengthW(edit);
        std::wstring text(len + 1, L'\0');
        GetWindowTextW(edit, &text[0], len + 1);
        text.resize(len);

        // Координати саме ті, що бачить користувач; модель сама мапить рядок
        m_model.SetCell(m_editX, m_editY, text);
    }

    DestroyWindow(edit);
    if (hadFocus) SetFocus(m_hwnd);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

LRESULT CALLBACK TableWindow::EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                               UINT_PTR id, DWORD_PTR refData)
{
    HWND parent = GetParent(hwnd);
    switch (msg) {
        case WM_CHAR:
            if (wParam == VK_RETURN) {
                PostMessageW(parent, WM_EDIT_DONE, TRUE, (LPARAM)hwnd);
                return 0;
            }
            if (wParam == VK_ESCAPE) {
                PostMessageW(parent, WM_EDIT_DONE, FALSE, (LPARAM)hwnd);
                return 0;
            }
            break;
        case WM_KILLFOCUS:
            PostMessageW(parent, WM_EDIT_DONE, TRUE, (LPARAM)hwnd);
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, EditSubclassProc, id);
            break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// ============================================================================
// Публічний Refresh
// ============================================================================
void TableWindow::Refresh() {
    EndEdit(false);
    SyncColumns();
    auto size = m_model.GetTableSize();
    if (m_selX > size.first || m_selY > size.second) m_selX = m_selY = -1;
    UpdateScrollBars();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

// ============================================================================
// Повідомлення
// ============================================================================
LRESULT TableWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1; // малюємо все самі через double buffering

        case WM_SIZE:
            EndEdit(true);
            UpdateScrollBars();
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;

        case WM_HSCROLL:
            HandleScroll(true, wParam);
            return 0;

        case WM_VSCROLL:
            HandleScroll(false, wParam);
            return 0;

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            UINT lines = 3;
            SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
            if (lines > 100) lines = 3; // WHEEL_PAGESCROLL
            int dist = MulDiv(delta, (int)lines * m_rowH, WHEEL_DELTA);
            if (GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT) ScrollTo(m_scrollX - dist, m_scrollY);
            else                                        ScrollTo(m_scrollX, m_scrollY - dist);
            return 0;
        }

        case WM_MOUSEHWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            ScrollTo(m_scrollX + MulDiv(delta, 3 * m_rowH, WHEEL_DELTA), m_scrollY);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            EndEdit(true);
            SetFocus(m_hwnd);

            int x, y;
            if (HitTest((short)LOWORD(lParam), (short)HIWORD(lParam), x, y)) {
                DWORD now = (DWORD)GetMessageTime();
                bool dbl = (x == m_lastClickX && y == m_lastClickY &&
                            now - m_lastClickTime <= GetDoubleClickTime());

                if (x >= 1) {
                    m_selX = x;
                    m_selY = y;
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                    if (dbl) BeginEdit(x, y);
                }
                m_lastClickX = x;
                m_lastClickY = y;
                m_lastClickTime = dbl ? 0 : now;
            }
            return 0;
        }

        case WM_RBUTTONDOWN: {
            EndEdit(true);
            SetFocus(m_hwnd);

            int x, y;
            if (HitTest((short)LOWORD(lParam), (short)HIWORD(lParam), x, y) && y == 0)
                CycleSort(x);
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_F2 && m_selX >= 1 && m_selY >= 0)
                BeginEdit(m_selX, m_selY);
            return 0;

        case WM_EDIT_DONE:
            if ((HWND)lParam == m_edit) // ігноруємо застарілі повідомлення
                EndEdit(wParam != 0);
            return 0;
    }
    return Window::HandleMessage(msg, wParam, lParam);
}

// ============================================================================
// Малювання
// ============================================================================
void TableWindow::DrawCell(HDC dc, const RECT& r, const std::wstring& text,
                           COLORREF bg, UINT hAlign, bool selected)
{
    HBRUSH brush = (HBRUSH)GetStockObject(DC_BRUSH);

    SetDCBrushColor(dc, bg);
    FillRect(dc, &r, brush);

    // Сітка: права та нижня межі
    SetDCBrushColor(dc, RGB(205, 205, 205));
    RECT line = { r.right - 1, r.top, r.right, r.bottom };
    FillRect(dc, &line, brush);
    line = { r.left, r.bottom - 1, r.right, r.bottom };
    FillRect(dc, &line, brush);

    RECT tr = r;
    tr.left  += 6;
    tr.right -= 7;
    DrawTextW(dc, text.c_str(), -1, &tr,
              DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS | hAlign);

    if (selected) {
        SetDCBrushColor(dc, RGB(0, 120, 215));
        RECT f = r;
        FrameRect(dc, &f, brush);
        InflateRect(&f, -1, -1);
        FrameRect(dc, &f, brush);
    }
}

void TableWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    const int w = rc.right, h = rc.bottom;

    if (w > 0 && h > 0) {
        SyncColumns();
        auto size = m_model.GetTableSize();
        const int nCols = size.first, nRows = size.second;
        const int indexW = m_indexW, headerH = m_rowH;

        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);
        HFONT oldFont = (HFONT)SelectObject(mem, m_font);

        SetBkMode(mem, TRANSPARENT);
        SetTextColor(mem, GetSysColor(COLOR_WINDOWTEXT));
        FillRect(mem, &rc, (HBRUSH)(COLOR_WINDOW + 1));

        const COLORREF white  = GetSysColor(COLOR_WINDOW);
        const COLORREF header = GetSysColor(COLOR_BTNFACE);

        std::pair<bool, int> sort = m_model.GetSortingState(); // {asc, column}
        auto headerText = [&](int col, std::wstring base) {
            if (col == sort.second && !(col == 0 && sort.first))
                base += sort.first ? L" \u25B2" : L" \u25BC";
            return base;
        };

        // ---- 1. Клітинки даних (скролляться в обох напрямках) ----
        if (nRows > 0) {
            int saved = SaveDC(mem);
            IntersectClipRect(mem, indexW, headerH, w, h);

            int firstRow = m_scrollY / m_rowH;
            int lastRow  = std::min(nRows - 1, (m_scrollY + h - headerH) / m_rowH);
            int left = indexW - m_scrollX;

            for (int c = 1; c <= nCols; ++c) {
                int cw = m_colW[c];
                if (left + cw > indexW && left < w) {
                    for (int r = firstRow; r <= lastRow; ++r) {
                        RECT cell;
                        cell.left   = left;
                        cell.right  = left + cw;
                        cell.top    = headerH - m_scrollY + r * m_rowH;
                        cell.bottom = cell.top + m_rowH;
                        DrawCell(mem, cell, GetCellText(c, r + 1), white, DT_LEFT,
                                 c == m_selX && (r + 1) == m_selY);
                    }
                }
                left += cw;
                if (left >= w) break;
            }
            RestoreDC(mem, saved);
        }

        // ---- 2. Рядок заголовків (скролиться лише горизонтально) ----
        {
            int saved = SaveDC(mem);
            IntersectClipRect(mem, indexW, 0, w, headerH);
            int left = indexW - m_scrollX;
            for (int c = 1; c <= nCols; ++c) {
                int cw = m_colW[c];
                if (left + cw > indexW && left < w) {
                    RECT cell = { left, 0, left + cw, headerH };
                    DrawCell(mem, cell, headerText(c, GetCellText(c, 0)), header, DT_LEFT,
                             c == m_selX && m_selY == 0);
                }
                left += cw;
                if (left >= w) break;
            }
            RestoreDC(mem, saved);
        }

        // ---- 3. Стовпець нумерації (скролиться лише вертикально) ----
        if (nRows > 0) {
            int saved = SaveDC(mem);
            IntersectClipRect(mem, 0, headerH, indexW, h);

            int firstRow = m_scrollY / m_rowH;
            int lastRow  = std::min(nRows - 1, (m_scrollY + h - headerH) / m_rowH);
            for (int r = firstRow; r <= lastRow; ++r) {
                RECT cell;
                cell.left   = 0;
                cell.right  = indexW;
                cell.top    = headerH - m_scrollY + r * m_rowH;
                cell.bottom = cell.top + m_rowH;
                DrawCell(mem, cell, std::to_wstring(RowNumber(r)), header, DT_CENTER, false);
            }
            RestoreDC(mem, saved);
        }

        // ---- 4. Верхній лівий кут (не скролиться) ----
        {
            RECT corner = { 0, 0, indexW, headerH };
            DrawCell(mem, corner, headerText(0, L"\u2116"), header, DT_CENTER, false);
        }

        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);

        SelectObject(mem, oldFont);
        SelectObject(mem, oldBmp);
        DeleteObject(bmp);
        DeleteDC(mem);
    }

    EndPaint(m_hwnd, &ps);
}
