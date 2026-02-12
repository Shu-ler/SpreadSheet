#include "sheet.h"

#include "cell.h"
#include "common.h"

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

    // Увеличиваем размеры, если нужно
    if (pos.row >= static_cast<int>(cells_.size())) {
        cells_.resize(pos.row + 1);
    }
    if (pos.col >= static_cast<int>(cells_[pos.row].size())) {
        cells_[pos.row].resize(pos.col + 1);
    }

    // Создаем или перезаписываем ячейку
    if (!cells_[pos.row][pos.col]) {
        cells_[pos.row][pos.col] = std::make_unique<Cell>();
    }
    cells_[pos.row][pos.col]->Set(std::move(text));

    // Обновляем размер печатной области
    UpdatePrintSize();
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (pos.row >= static_cast<int>(cells_.size()) || 
        pos.col >= static_cast<int>(cells_[pos.row].size()) || 
        !cells_[pos.row][pos.col]) {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}

CellInterface* Sheet::GetCell(Position pos) {
    return const_cast<CellInterface*>(std::as_const(*this).GetCell(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (pos.row < static_cast<int>(cells_.size()) && 
        pos.col < static_cast<int>(cells_[pos.row].size()) && 
        cells_[pos.row][pos.col]) {
        cells_[pos.row][pos.col].reset();
        ShrinkPrintSize();
    }
}

Size Sheet::GetPrintableSize() const {
    return print_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int r = 0; r < print_size_.rows; ++r) {
        for (int c = 0; c < print_size_.cols; ++c) {
            if (r < static_cast<int>(cells_.size()) && 
                c < static_cast<int>(cells_[r].size()) && 
                cells_[r][c]) {
                CellInterface::Value value = cells_[r][c]->GetValue();
                if (std::holds_alternative<std::string>(value)) {
                    output << std::get<std::string>(value);
                } else if (std::holds_alternative<double>(value)) {
                    output << std::get<double>(value);
                } else if (std::holds_alternative<FormulaError>(value)) {
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
            if (r < static_cast<int>(cells_.size()) && 
                c < static_cast<int>(cells_[r].size()) && 
                cells_[r][c]) {
                std::string text = cells_[r][c]->GetText();
                output << text;
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

    for (size_t r = 0; r < cells_.size(); ++r) {
        for (size_t c = 0; c < cells_[r].size(); ++c) {
            if (cells_[r][c]) {
                max_row = std::max(max_row, static_cast<int>(r) + 1);
                max_col = std::max(max_col, static_cast<int>(c) + 1);
                found = true;
            }
        }
    }

    if (found) {
        print_size_.rows = max_row;
        print_size_.cols = max_col;
    } else {
        print_size_ = Size{};
    }
}

void Sheet::ShrinkPrintSize() {
    int max_row = 0;
    int max_col = 0;
    bool found = false;

    for (size_t r = 0; r < cells_.size(); ++r) {
        for (size_t c = 0; c < cells_[r].size(); ++c) {
            if (cells_[r][c]) {
                max_row = std::max(max_row, static_cast<int>(r) + 1);
                max_col = std::max(max_col, static_cast<int>(c) + 1);
                found = true;
            }
        }
    }

    if (found) {
        print_size_.rows = max_row;
        print_size_.cols = max_col;
    } else {
        print_size_ = Size{};
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
