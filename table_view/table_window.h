#ifndef _TABLE_WINDOW_H_
#define _TABLE_WINDOW_H_

#include "window.h"
#include "table_model.h"
//#include <minwindef.h>
#include <string>
#include <vector>

#define BTN_SAVE_AS 1
#define BTN_SAVE 2
#define BTN_OPEN 3

class TableWindow : public Window {
public:
    TableWindow();
    ~TableWindow() override;

    // Додає WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN до переданого стилю
    bool Create(PCWSTR title, DWORD style, DWORD exStyle = 0,
                int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
                int width = CW_USEDEFAULT, int height = CW_USEDEFAULT,
                HWND parent = nullptr, HMENU menu = nullptr) override;

    TableModel& GetModel() { return m_model; }

    // Викликати після зовнішніх змін моделі (додали/видалили записи чи заголовки)
    void Refresh();

protected:
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;
    LRESULT OnCommand(int id, int eventCode, HWND controlHandle) override;
    void OnPaint() override;
    PCWSTR GetClassName() const override;

private:
    std::wstring m_last_save_path;

    // Скільки "порожніх" рядків і стовпців показувати за межами моделі
        static constexpr int kExtra = 2;
        int DispCols() { return m_model.GetTableSize().first  + kExtra; }
        int DispRows() { return m_model.GetTableSize().second + kExtra; }

    // ---- допоміжні типи ----
    static constexpr UINT WM_EDIT_DONE = WM_APP + 1; // wParam: 1 = зберегти, 0 = скасувати; lParam: HWND edit-а

    static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR id, DWORD_PTR refData);

    // ---- дані ----
    int  RecordIndex(int displayRow) const;       // displayRow (з 0) -> індекс у GetRecords()
    int  RowNumber(int displayRow) const;         // число у стовпці нумерації
    std::wstring GetCellText(int x, int y);       // x,y - координати інтерфейсу (0 = нумерація/заголовки)

    // ---- розмітка ----
    void SyncColumns();
    int  ContentWidth();                          // ширина всіх колонок, крім нумерації
    int  ContentHeight();                         // висота всіх записів, без заголовка
    RECT CellRect(int x, int y);                  // клієнтські координати з урахуванням скролу
    bool HitTest(int px, int py, int& x, int& y);

    // ---- скролінг ----
    void UpdateScrollBars();
    void ScrollTo(int newX, int newY);
    void HandleScroll(bool horizontal, WPARAM wParam);
    void EnsureVisible(int x, int y);

    // ---- взаємодія ----
    void CycleSort(int col);
    void BeginEdit(int x, int y);
    void EndEdit(bool commit);

    // ---- малювання ----
    void DrawCell(HDC dc, const RECT& r, const std::wstring& text,
                  COLORREF bg, UINT hAlign, bool selected);

    TableModel m_model;

    std::vector<int> m_colW;      // m_colW[0] - стовпець нумерації
    int m_rowH     = 20;
    int m_defColW  = 150;
    int m_indexW   = 50;
    int m_scrollX  = 0;
    int m_scrollY  = 0;
    HFONT m_font   = nullptr;

    int m_selX = -1, m_selY = -1;

    HWND m_edit = nullptr;
    int  m_editX = 0, m_editY = 0;

    // Ручне розпізнавання подвійного кліку (базовий клас не реєструє CS_DBLCLKS)
    DWORD m_lastClickTime = 0;
    int   m_lastClickX = -1, m_lastClickY = -1;
};

#endif // _TABLE_WINDOW_H_
