#include "sheet.h"
#include "cell.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid position");
	}

	// Сохраняем указатель на старую ячейку (если была)
	auto& cell_ptr = cells_[pos];
	Cell* cell = nullptr;

	if (!cell_ptr) {
		cell_ptr = std::make_unique<Cell>(*this);
	}
	cell = cell_ptr.get();

	// --- Шаг 1: Проверка (если формула) ---
	if (IsFormula(text)) {
		try {
			auto formula = ParseFormula(text.substr(1));
			auto referenced_cells = formula->GetReferencedCells();

			CheckSelfReference(referenced_cells, pos);
			CheckCircularDependency(cell, referenced_cells);  // временная ячейка уже создана
		}
		catch (const FormulaException&) {
			throw;
		}
	}

	// --- Шаг 2: Получаем старые зависимости (на кого ссылалась ячейка ДО) ---
	std::vector<Position> old_refs;
	if (IsFormula(cell->GetText())) {
		// Можно вызвать GetReferencedCells(), но лучше хранить в Cell?
		old_refs = cell->GetReferencedCells();
	}

	// --- Шаг 3: Устанавливаем новое значение ---
	cell->Set(std::move(text));  // ← здесь impl_ меняется, но кэш и зависимости не обновляются

	// --- Шаг 4: Обновляем граф зависимостей ---
	std::vector<Position> new_refs;
	if (IsFormula(cell->GetText())) {
		new_refs = cell->GetReferencedCells();
	}

	// Удаляем эту ячейку из dependents_ старых зависимостей
	for (const auto& ref_pos : old_refs) {
		if (auto* dep_cell = dynamic_cast<Cell*>(GetCell(ref_pos))) {
			dep_cell->RemoveDependentCell(cell);
		}
	}

	// Добавляем эту ячейку в dependents_ новых зависимостей
	for (const auto& ref_pos : new_refs) {
		if (auto* dep_cell = dynamic_cast<Cell*>(GetCell(ref_pos))) {
			dep_cell->AddDependentCell(cell);
		}
	}

	// --- Шаг 5: Инвалидируем кэш этой ячейки и всех, кто от неё зависит ---
	cell->InvalidateCache();  // теперь это безопасно и логично

	// --- Шаг 6: Обновляем размер печатной области ---
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

inline bool Sheet::IsFormula(std::string text) {
	return text.size() > 1 && text[0] == FORMULA_SIGN;
}

void Sheet::CheckSelfReference(const std::vector<Position>& referenced_cells, Position cell_pos) {
	if (std::find(referenced_cells.begin(), referenced_cells.end(), cell_pos) != referenced_cells.end()) {
		throw CircularDependencyException("Cyclic dependency: cell references itself");
	}
}

void Sheet::CheckCircularDependency(const Cell* target_cell, const std::vector<Position>& referenced_cells) {
	// Множество посещённых ячеек для отслеживания пути при DFS.
	// Используется для обнаружения циклов в графе зависимостей.
	std::unordered_set<const Cell*> visited;

	// Рекурсивная лямбда для проверки наличия цикла в графе зависимостей.
	// Обходит все ячейки, от которых зависит текущая, и проверяет,
	// не приведёт ли зависимость к повторному посещению уже пройденной ячейки.
	std::function<bool(const Cell*)> has_cycle = [&](const Cell* cell) -> bool {
		// Если ячейка уже в пути — найден цикл
		if (visited.find(cell) != visited.end()) {
			return true;
		}

		// Помечаем ячейку как посещённую
		visited.insert(cell);

		// Рекурсивно проверяем все ячейки, на которые ссылается текущая
		for (const auto& dep_pos : cell->GetReferencedCells()) {
			// Получаем указатель на зависимую ячейку
			if (const auto* dep_cell = dynamic_cast<const Cell*>(GetCell(dep_pos))) {
				if (has_cycle(dep_cell)) {
					return true; // Цикл обнаружен в поддереве
				}
			}
		}

		// Убираем ячейку из множества при возврате (возврат по стеку)
		// Это важно: один и тот же узел может быть частью разных путей без цикла
		visited.erase(cell);
		return false;
		};

	// Проверяем каждую ячейку, на которую ссылается новая формула:
	// если начать обход из неё, мы не должны добраться до целевой ячейки (target_cell),
	// иначе будет циклическая зависимость.
	for (const auto& ref_pos : referenced_cells) {
		if (const auto* dep_cell = dynamic_cast<const Cell*>(GetCell(ref_pos))) {
			if (has_cycle(dep_cell)) {
				throw CircularDependencyException("Cyclic dependency detected");
			}
		}
	}
}
std::unique_ptr<SheetInterface> CreateSheet() {
	return std::make_unique<Sheet>();
}