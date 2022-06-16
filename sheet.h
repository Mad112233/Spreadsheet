#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <vector>
#include <variant>
#include <unordered_map>

enum class RequestType {
    TEXT,
    VALUE
};

struct Hasher {
    size_t operator()(const Position& val)const;
};

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, const std::string& text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void Print(std::ostream& output, RequestType type) const;
    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    void ResizePrintArea();
    void CycleCheck(Sheet& sheet, Cell* start, Cell* current, std::unordered_set<Cell*>& visited);
    void ClearCache(Sheet& sheet, Cell* cell_ptr);

    std::unordered_map<Position, std::unique_ptr<Cell>, Hasher>table_;
    Size print_area_ = { 0,0 };
};