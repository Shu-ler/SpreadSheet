#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {}

const CellInterface* Sheet::GetCell(Position pos) const {}
CellInterface* Sheet::GetCell(Position pos) {}

void Sheet::ClearCell(Position pos) {}

Size Sheet::GetPrintableSize() const {}

void Sheet::PrintValues(std::ostream& output) const {}
void Sheet::PrintTexts(std::ostream& output) const {}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
