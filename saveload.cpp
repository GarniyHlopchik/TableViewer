#include "saveload.h"
#include <vector>

bool SaveLoad::GetSavePath(HWND owner, std::wstring& out_path){
    wchar_t path[MAX_PATH] = L"";

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = owner;
    ofn.lpstrFilter = L"Text (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrDefExt = L"txt";   // appended if the user types a name without an extension
    ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetSaveFileNameW(&ofn)) return false;
    out_path = path;
    return true;
}

bool SaveLoad::GetOpenPath(HWND owner, std::wstring &out_path){
    wchar_t path[MAX_PATH] = L"";

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = owner;
    ofn.lpstrFilter = L"Text (*.txt)\0*.txt\0All files (*.*)\0*.*\0";  // pairs of "label\0pattern\0"
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&ofn)) return false;   // user pressed Cancel (or an error occurred)
    out_path = path;
    return true;
}

bool SaveLoad::ReadFileBytes(const std::wstring& path, std::vector<unsigned char>& out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size;
    if (!GetFileSizeEx(h, &size) || size.QuadPart > 0xFFFFFFFFLL) {   // keep it simple: < 4 GB
        CloseHandle(h);
        return false;
    }

    out.resize((size_t)size.QuadPart);
    DWORD bytesRead = 0;
    BOOL ok = out.empty() || ReadFile(h, out.data(), (DWORD)out.size(), &bytesRead, nullptr);
    CloseHandle(h);
    return ok && bytesRead == out.size();
}

std::wstring SaveLoad::BytesToWString(const std::vector<unsigned char>& bytes) {
    std::wstring result;
    result.reserve(bytes.size());

    for (unsigned char byte : bytes) {
        if (byte == '\0') {
            break;
        }

        // Convert to wide character via unsigned char so high-bit bytes don't sign-extend
        result.push_back(static_cast<wchar_t>(byte));
    }

    return result;
}

std::wstring SaveLoad::TableToWString(TableModel& model){
    auto headers = model.GetHeaders();
    auto records = model.GetRecords();

    std::wstring result;
    for(auto& h : headers){
        result += h + L"\t";
    }
    result.pop_back();
    result += L"\n";

    for(auto& r : records){
        for(auto& c : r.cells){
            result += c + L"\t";
        }
        result.pop_back();
        result += L"\n";
    }
    return result;
}

bool SaveLoad::SaveWString(const std::wstring& path, const std::wstring& string){
    if (path.empty()) {
        return false;
    }

    // Convert std::wstring to narrow ASCII / ANSI bytes (+1 for null delimiter)
    // Using WC_NO_BEST_FIT_CHARS replaces unmappable non-ASCII characters with '?'
    const int requiredBytes = WideCharToMultiByte(
        CP_ACP,
        WC_NO_BEST_FIT_CHARS,
        string.c_str(),
        static_cast<int>(string.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (requiredBytes < 0) {
        return false;
    }

    // +1 byte for the trailing '\0' delimiter
    std::vector<char> buffer(static_cast<size_t>(requiredBytes) + 1, '\0');

    if (requiredBytes > 0) {
        const int written = WideCharToMultiByte(
            CP_ACP,
            WC_NO_BEST_FIT_CHARS,
            string.c_str(),
            static_cast<int>(string.size()),
            buffer.data(),
            requiredBytes,
            nullptr,
            nullptr
        );

        if (written <= 0) {
            return false;
        }
    }

    // Ensure null delimiter at the end of the buffer
    buffer.back() = '\0';

    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        0,                      // No sharing while writing
        nullptr,
        CREATE_ALWAYS,          // Overwrite existing file or create a new one
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD totalWritten = 0;
    const DWORD bytesToWrite = static_cast<DWORD>(buffer.size());

    while (totalWritten < bytesToWrite) {
        DWORD chunkWritten = 0;
        const BOOL ok = WriteFile(
            hFile,
            buffer.data() + totalWritten,
            bytesToWrite - totalWritten,
            &chunkWritten,
            nullptr
        );

        if (!ok || chunkWritten == 0) {
            CloseHandle(hFile);
            return false;
        }

        totalWritten += chunkWritten;
    }

    CloseHandle(hFile);
    return true;
}

void SaveLoad::WStringToTable(const std::wstring& string, TableModel& model){
    int cursor_x = 1;
    int cursor_y = 0;
    std::wstring buf;
    for(auto c : string){
        switch(c){
            case '\t':{
                model.SetCell(cursor_x, cursor_y, buf);
                cursor_x += 1;
                buf.clear();
                break;
            }
            case '\r':{
                break;
            }
            case '\n':{
                model.SetCell(cursor_x, cursor_y, buf);
                cursor_x = 1;
                cursor_y += 1;
                buf.clear();
                break;
            }
            default:{
                buf += c;
                break;
            }
        }
    }
}
