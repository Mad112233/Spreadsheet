#include "sheet.h"
#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

size_t Hasher::operator()(const Position& val)const {
    return val.col * 31 + val.row;
}

Sheet::~Sheet() {}

void Sheet::ResizePrintArea() {
    print_area_.rows = print_area_.cols = 0;

    for (const auto& val : table_) {
        print_area_.rows = std::max(print_area_.rows, val.first.row + 1);
        print_area_.cols = std::max(print_area_.cols, val.first.col + 1);
    }
}

void Sheet::SetCell(Position pos, const std::string& text) {
    if (!pos.IsValid())
        throw InvalidPositionException("The cell goes outside the table!");

    if (table_.count(pos)) {
        table_[pos]->Set(text);
    }
    else {
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->Set(text);
        table_[pos] = std::move(new_cell);
    }

    for (const auto& cell : table_[pos]->GetReferencedCells()) {
        if (!cell.IsValid()) {
            return;
        }

        if (this->GetCell(cell) == nullptr) {
            auto new_cell = std::make_unique<Cell>(*this);
            table_[cell] = std::move(new_cell);

            ResizePrintArea();

            return;
        }
    }

    std::unordered_set<Cell*> visited;
    CycleCheck(*this, table_[pos].get(), table_[pos].get(), visited);

    for (const auto& cell : table_[pos]->GetReferencedCells()) {
        auto ptr = reinterpret_cast<Cell*>(this->GetCell(cell));

        if (ptr != nullptr)
            ptr->AddDependentCell(pos);
    }
    
    ClearCache(*this, table_[pos].get());
    ResizePrintArea();
}

void Sheet::ClearCache(Sheet& sheet, Cell* ptr_cell) {
    for (const auto& cell : ptr_cell->GetDependentCells()) {
        auto* ptr = reinterpret_cast<Cell*>(sheet.GetCell(cell));
        ptr->ClearCache();
        ClearCache(sheet, ptr);
    }
}


const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid())
        throw InvalidPositionException("The cell goes outside the table!");
    
    if (!table_.count(pos) || table_.at(pos)->GetText().empty())
        return nullptr;

    return table_.at(pos).get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid())
        throw InvalidPositionException("The cell goes outside the table!");

    if (!table_.count(pos) || table_.at(pos)->GetText().empty())
        return nullptr;

    return table_.at(pos).get();
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid())
        throw InvalidPositionException("The cell goes outside the table!");

    if (!table_.count(pos))
        return;

    table_.erase(pos);
    ResizePrintArea();
}

Size Sheet::GetPrintableSize() const {
    return print_area_;
}

void Sheet::Print(std::ostream& output, RequestType type) const {
    for (int i = 0; i < print_area_.rows; ++i) {
        for (int u = 0; u < print_area_.cols; ++u) {
            if (type == RequestType::TEXT) {
                if (table_.count({ i,u }))
                    output << table_.at({ i, u })->GetText() << (u + 1 == print_area_.cols ? "" : "\t");
                else
                    output << (u + 1 == print_area_.cols ? "" : "\t");
            }
            else {
                if (table_.count({ i,u }))
                    output << table_.at({ i, u })->GetValue() << (u + 1 == print_area_.cols ? "" : "\t");
                else
                    output << (u + 1 == print_area_.cols ? "" : "\t");
            }
        }
        output << '\n';
    }
}

void Sheet::PrintValues(std::ostream& output) const {
    Print(output, RequestType::VALUE);
}

void Sheet::PrintTexts(std::ostream& output) const {
    Print(output, RequestType::TEXT);
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

void Sheet::CycleCheck(Sheet& sheet, Cell* start, Cell* current, std::unordered_set<Cell*>& visited) {
    if (current) {
        for (const auto& cell : current->GetReferencedCells()) {
            if (!cell.IsValid())
                continue;

            auto* ptr = reinterpret_cast<Cell*>(sheet.GetCell(cell));
            if (visited.find(ptr) != visited.end()) {
                continue;
            }

            if (start == ptr) {
                throw CircularDependencyException("Cycle found!");
            }

            visited.insert(ptr);
            CycleCheck(sheet, start, ptr, visited);
        }
    }
}