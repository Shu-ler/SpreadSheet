#include "sheet.h"

#include "cell.h"
//#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    // Создаём ячейку или перезаписываем существующую
    auto& cell_ptr = cells_[pos];
    if (!cell_ptr) {
        cell_ptr = std::make_unique<Cell>(*this);  // Передаём ссылку на таблицу
    }
    cell_ptr->Set(std::move(text));

    // Обновляем размер печатной области
    UpdatePrintSize();
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    auto it = cells_.find(pos);
    if (it == cells_.end() || !it->second) {
        return nullptr;
    }
    return it->second.get();
}

CellInterface* Sheet::GetCell(Position pos) {
    return const_cast<CellInterface*>(std::as_const(*this).GetCell(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    auto it = cells_.find(pos);
    if (it != cells_.end()) {
        cells_.erase(it);
        ShrinkPrintSize();
    }
}

Position Sheet::GetPosition(const Cell* cell) const {
    for (const auto& [pos, ptr] : cells_) {
        if (ptr.get() == cell) {
            return pos;
        }
    }
    return Position::NONE;
}

Size Sheet::GetPrintableSize() const {
    return print_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int r = 0; r < print_size_.rows; ++r) {
        for (int c = 0; c < print_size_.cols; ++c) {
            Position pos{ r, c };
            auto it = cells_.find(pos);
            if (it != cells_.end() && it->second) {
                CellInterface::Value value = it->second->GetValue();
                if (std::holds_alternative<std::string>(value)) {
                    output << std::get<std::string>(value);
                }
                else if (std::holds_alternative<double>(value)) {
                    output << std::get<double>(value);
                }
                else if (std::holds_alternative<FormulaError>(value)) {
                    output << std::get<FormulaError>(value);
                }
            }
            if (c + 1 < print_size_.cols) {
                output << "\t";
            }
        }
        output << "\n";
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int r = 0; r < print_size_.rows; ++r) {
        for (int c = 0; c < print_size_.cols; ++c) {
            Position pos{ r, c };
            auto it = cells_.find(pos);
            if (it != cells_.end() && it->second) {
                output << it->second->GetText();
            }
            if (c + 1 < print_size_.cols) {
                output << "\t";
            }
        }
        output << "\n";
    }
}

void Sheet::UpdatePrintSize() {
    int max_row = 0;
    int max_col = 0;
    bool found = false;

    for (const auto& [pos, cell_ptr] : cells_) {
        if (cell_ptr) {
            max_row = std::max(max_row, pos.row + 1);
            max_col = std::max(max_col, pos.col + 1);
            found = true;
        }
    }

    if (found) {
        print_size_.rows = max_row;
        print_size_.cols = max_col;
    }
    else {
        print_size_ = Size{};
    }
}

void Sheet::ShrinkPrintSize() {
    // Просто вызываем UpdatePrintSize(), так как область может уменьшиться
    // после удаления ячейки.
    UpdatePrintSize();
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}