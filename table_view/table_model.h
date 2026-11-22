#pragma once
#include <vector>
#include <string>
#include "record.h"
#include <utility>
//table view class містить лише логіку роботи з таблицями - додавання, порядок, видалення, редагування тощо

class TableModel{
    public:
        //тут x - номер стовпчика, притому 0 це для стовпчику нумерації
        //y - номер рядку, притому 0 це рядок хеадерів
        void SetCell(int x, int y, std::wstring value);
        void SortBy(int col, bool is_ascending);
        void Clear();
        std::pair<bool,int> GetSortingState(); // is_ascending + column
        std::pair<int,int> GetTableSize(); //header amount + record amount
        std::vector<std::wstring>& GetHeaders();
        std::vector<Record>& GetRecords();
        const std::vector<int>& GetSortIndices();
    private:
        //тут x це завжди саме про кількість хеадерів, а y про кількість записів.
        //наприклад, (3,4) - значить є 3 хеадери та 4 записи.
        void ExpandTo(int size_x, int size_y);
        void ShrinkTo(int size_x, int size_y);

        std::vector<std::wstring> m_headers;
        std::vector<Record> m_records;
        std::vector<int> m_sorting_indices;
        int m_sort_col = 0;
        bool is_ascending_order = true;

};
