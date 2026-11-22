#include <string>
#include <vector>
#include "table_model.h"
#include "windows.h"

class SaveLoad{
    public:
    static bool GetSavePath(HWND owner, std::wstring& out_path);
    static bool GetOpenPath(HWND owner, std::wstring& out_path);

    static bool ReadFileBytes(const std::wstring& path, std::vector<unsigned char>& out);
    static std::wstring BytesToWString(const std::vector<unsigned char>& bytes);

    static std::wstring TableToWString(TableModel& model);
    static bool SaveWString(const std::wstring& path, const std::wstring& string);

    static void WStringToTable(const std::wstring& string, TableModel& model);
};
