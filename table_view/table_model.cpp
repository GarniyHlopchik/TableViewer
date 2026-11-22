#include <algorithm>
#include <numeric>
#define NOMINMAX
#include <windows.h> // для CompareStringOrdinal
#include "table_model.h"

void TableModel::Clear(){
    m_headers.clear();
    m_records.clear();
}

void TableModel::SetCell(int x, int y, std::wstring value){ // x > 0; y >= 0
    if (x <= 0 || y < 0) return;

    const size_t ux = static_cast<size_t>(x);
    const size_t uy = static_cast<size_t>(y);
    const bool is_empty = value.empty();   // запам'ятовуємо ДО std::move
    const bool out_of_bounds = (ux > m_headers.size() || uy > m_records.size());

    if (is_empty && out_of_bounds) return;   // нема що очищати за межами таблиці

    if (!is_empty && out_of_bounds) {
        ExpandTo(static_cast<int>(std::max(ux, m_headers.size())),
                 static_cast<int>(std::max(uy, m_records.size())));
    }

    if (y == 0) {
        m_headers[x - 1] = std::move(value);
    } else {
        // GetSortIndices() перераховує індекси, тож вони завжди актуальні
        const std::vector<int>& idx = GetSortIndices();
        m_records[idx[y - 1]].cells[x - 1] = std::move(value);
    }

    if (!is_empty) return;

    // ---- згортання порожніх країв ----
    if (ux == m_headers.size()) {
        int shrink_amount = 0;
        while (shrink_amount < static_cast<int>(m_headers.size())) {
            size_t col = m_headers.size() - 1 - shrink_amount;
            bool empty_col = m_headers[col].empty();
            for (auto& r : m_records) {
                if (!r.cells[col].empty()) { empty_col = false; break; }
            }
            if (!empty_col) break;
            ++shrink_amount;
        }
        ShrinkTo(static_cast<int>(m_headers.size()) - shrink_amount,
                 static_cast<int>(m_records.size()));
    }

    if (uy == m_records.size()) {
        int shrink_amount = 0;
        while (shrink_amount < static_cast<int>(m_records.size())) {
            const auto& cells = m_records[m_records.size() - 1 - shrink_amount].cells;
            bool empty_row = std::all_of(cells.begin(), cells.end(),
                                         [](const std::wstring& s){ return s.empty(); });
            if (!empty_row) break;
            ++shrink_amount;
        }
        ShrinkTo(static_cast<int>(m_headers.size()),
                 static_cast<int>(m_records.size()) - shrink_amount);
    }
}

void TableModel::ExpandTo(int size_x, int size_y){
    m_headers.resize(size_x, L"");
    //існуючі записи добиваємо до тої ж кількості рядків
    for(auto& r: m_records){
        r.cells.resize(size_x, L"");
    }
    //додаємо нові з правильним row_id (1-indexed, 0 для хеадерів)
    //вважає size_y >= m_records.size() - саме expand
    int iter_amount = size_y-m_records.size();
    for(int i = 0; i < iter_amount; i++){
        Record rec;
        rec.row_id = m_records.size() + 1;
        rec.cells.resize(size_x, L"");
        m_records.push_back(rec);
    }
}

void TableModel::ShrinkTo(int size_x, int size_y){
    m_records.resize(size_y);

    m_headers.resize(size_x);
    for(auto& r : m_records){
        r.cells.resize(size_x);
    }
}

std::pair<bool,int> TableModel::GetSortingState(){
    return std::make_pair(is_ascending_order, m_sort_col);
}

std::pair<int,int> TableModel::GetTableSize(){
    return std::make_pair(m_headers.size(), m_records.size());
}

std::vector<std::wstring>& TableModel::GetHeaders(){
    return m_headers;
}

std::vector<Record>& TableModel::GetRecords(){
    return m_records;
}

const std::vector<int>& TableModel::GetSortIndices() {
    m_sorting_indices.resize(m_records.size());
    // Заповнюємо: 0, 1, 2, ..., size - 1
    std::iota(m_sorting_indices.begin(), m_sorting_indices.end(), 0);

    if (m_sort_col == 0) {
        if (!is_ascending_order) {
            std::reverse(m_sorting_indices.begin(), m_sorting_indices.end());
        }
        return m_sorting_indices;
    }

    const size_t col_idx = static_cast<size_t>(m_sort_col - 1);

    // std::stable_sort зберігає відносний порядок однакових елементів
    std::stable_sort(m_sorting_indices.begin(), m_sorting_indices.end(),
        [this, col_idx](int idx_a, int idx_b) {
            const std::wstring& str_a = m_records[idx_a].cells[col_idx];
            const std::wstring& str_b = m_records[idx_b].cells[col_idx];

            // Порівняння без урахування регістру (працює і для латиниці, і для кирилиці)
            int cmp = CompareStringOrdinal(
                str_a.c_str(), static_cast<int>(str_a.size()),
                str_b.c_str(), static_cast<int>(str_b.size()),
                TRUE // bIgnoreCase = TRUE
            );

            // CompareStringOrdinal повертає:
            // CSTR_LESS_THAN (1), CSTR_EQUAL (2), CSTR_GREATER_THAN (3)
            if (is_ascending_order) {
                return cmp == CSTR_LESS_THAN;
            } else {
                return cmp == CSTR_GREATER_THAN;
            }
        });

    return m_sorting_indices;
}

void TableModel::SortBy(int col, bool is_ascending){
    is_ascending_order = is_ascending;
    m_sort_col = col;
}
