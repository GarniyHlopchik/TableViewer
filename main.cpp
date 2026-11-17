#include "table_window.h"

TableWindow myWindow;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    if (!myWindow.Create(L"Переглядач таблиць", WS_OVERLAPPEDWINDOW, 0, 100, 100, 800, 600)) {
        return 0;
    }
    myWindow.Refresh();
    myWindow.Show(nCmdShow);

    // Цикл обробки повідомлень
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
