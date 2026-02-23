#include "sheet.h"
#include "cell.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <cassert>
#include <utility>

using namespace std::literals;

//void Sheet::SetCell(Position pos, std::string text) {
//	EnsurePositionValid(pos);
//
//	// Сначала анализируем новый текст, не трогая ячейку
//	std::vector<Position> new_refs;
//	bool is_formula = Cell::IsFormulaText(text);
//
//	if (is_formula) {
//		try {
//			auto formula = ParseFormula(text.substr(1));
//			new_refs = formula->GetReferencedCells();
//
//			CheckSelfReference(new_refs, pos);
//			CheckCircularDependency(new_refs, pos);
//		}
//		catch (const FormulaException&) {
//			throw;
//		}
//		catch (const CircularDependencyException&) {
//			throw;
//		}
//	}
//
//	// Теперь можно безопасно создать или получить ячейку
//	Cell* cell = GetOrCreateCell(pos);
//
//	// Сохраняем старые ссылки
//	std::vector<Position> old_refs;
//	if (Cell::IsFormulaText(cell->GetText())) {
//		old_refs = cell->GetReferencedCells();
//	}
//
//	// Гарантированно создаём любые зависимые ячейки
//	EnsureCellsExist(new_refs);
//
//	// Устанавливаем новое содержимое
//	cell->Set(std::move(text));
//
//	// Обновляем зависимости
//	UpdateDependencies(cell, old_refs, new_refs);
//
//	// Инвалидируем кэш
//	cell->InvalidateCache();
//
//	// Обновляем размер печатной области
//	UpdatePrintSize();
//}

void Sheet::SetCell(Position pos, std::string text) {
	// 1. Проверяем корректность позиции
	EnsurePositionValid(pos);

	// 2. Анализируем содержимое: это формула?
	bool is_formula = Cell::IsFormulaText(text);
	std::vector<Position> new_refs;

	// Указатель на существующую ячейку (если есть)
	Cell* cell = dynamic_cast<Cell*>(GetCell(pos));

	// 3. Если это формула — парсим и проверяем до любых изменений
	if (is_formula) {
		try {
			// Парсим выражение (без '=')
			auto formula = ParseFormula(text.substr(1));
			new_refs = formula->GetReferencedCells();

			// Проверка: не ссылается ли на саму себя?
			CheckSelfReference(new_refs, pos);

			// Проверка: не будет ли циклической зависимости?
			CheckCircularDependency(new_refs, pos);
		}
		catch (const FormulaException&) {
			// Синтаксическая ошибка в формуле — пробрасываем
			throw;
		}
		catch (const CircularDependencyException&) {
			// Циклическая зависимость — пробрасываем
			throw;
		}
	}

	// 4. Теперь безопасно получаем или создаём ячейку
	// Важно: делаем это ТОЛЬКО после успешной валидации!
	if (!cell) {
		cell = GetOrCreateCell(pos);
	}

	// 5. Сохраняем старые зависимости (до изменения)
	std::vector<Position> old_refs;
	if (Cell::IsFormulaText(cell->GetText())) {
		old_refs = cell->GetReferencedCells();
	}

	// 6. Создаём пустые ячейки для всех новых ссылок (если ещё не существуют)
	// Это нужно, чтобы они были доступны при вычислении
	EnsureCellsExist(new_refs);

	// 7. Устанавливаем новое содержимое ячейки
	// Может изменить impl_, вызвать InvalidateCache внутри FormulaImpl
	cell->Set(std::move(text));

	// 8. Обновляем граф зависимостей:
	// - удаляем эту ячейку из dependents_ старых зависимостей
	// - добавляем в dependents_ новых
	UpdateDependencies(cell, old_refs, new_refs);

	// 9. Инвалидируем кэш текущей ячейки и всех, кто от неё зависит
	cell->InvalidateCache();

	// 10. Обновляем размер печатной области
	UpdatePrintSize();
}

const CellInterface* Sheet::GetCell(Position pos) const {
	EnsurePositionValid(pos);

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
	EnsurePositionValid(pos);

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

void Sheet::EnsurePositionValid(Position pos) const {
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid position");
	}
}

Cell* Sheet::GetOrCreateCell(Position pos) {
	auto& cell_ptr = cells_[pos];
	if (!cell_ptr) {
		cell_ptr = std::make_unique<Cell>(*this);
	}
	return cell_ptr.get();
}

inline bool Sheet::IsFormula(std::string text) {
	return text.size() > 1 && text[0] == FORMULA_SIGN;
}

void Sheet::CheckSelfReference(const std::vector<Position>& referenced_cells, Position cell_pos) {
	if (std::find(referenced_cells.begin(), referenced_cells.end(), cell_pos) != referenced_cells.end()) {
		throw CircularDependencyException("Cyclic dependency: cell references itself");
	}
}

void Sheet::CheckCircularDependency(const std::vector<Position>& refs, Position target_pos) {
	std::unordered_set<const Cell*> visited;

	// Рекурсивная лямбда для DFS
	std::function<bool(const Cell*)> has_cycle = [&](const Cell* cell) -> bool {
		if (visited.find(cell) != visited.end()) {
			return true; // цикл найден
		}
		visited.insert(cell);

		for (const auto& dep_pos : cell->GetReferencedCells()) {
			if (dep_pos == target_pos) {
				return true; // достигли целевой ячейки → цикл
			}
			if (const auto* dep_cell = dynamic_cast<const Cell*>(GetCell(dep_pos))) {
				if (has_cycle(dep_cell)) {
					return true;
				}
			}
		}

		visited.erase(cell);
		return false;
		};

	// Запускаем DFS из каждой ячейки, на которую ссылается формула
	for (const auto& ref_pos : refs) {
		const Cell* start_cell = dynamic_cast<const Cell*>(GetCell(ref_pos));
		if (start_cell && has_cycle(start_cell)) {
			throw CircularDependencyException("Cyclic dependency detected");
		}
	}
}

void Sheet::EnsureCellsExist(const std::vector<Position>& positions) {
	for (const auto& pos : positions) {
		if (!pos.IsValid()) {
			throw FormulaException("Invalid cell position in formula: " + pos.ToString());
		}
		if (cells_.find(pos) == cells_.end()) {
			cells_[pos] = std::make_unique<Cell>(*this);
		}
	}
}

void Sheet::UpdateDependencies(Cell* cell, 
			const std::vector<Position>& old_refs, 
			const std::vector<Position>& new_refs) {
	// Удаляем эту ячейку из dependents_ старых зависимостей
	for (const auto& ref_pos : old_refs) {
		if (ref_pos == GetPosition(cell)) continue; // на всякий случай исключаем самоссылку
		if (auto* dep_cell = dynamic_cast<Cell*>(GetCell(ref_pos))) {
			dep_cell->RemoveDependentCell(cell);
		}
	}

	// Добавляем эту ячейку в dependents_ новых зависимостей
	for (const auto& ref_pos : new_refs) {
		if (ref_pos == GetPosition(cell)) continue;
		auto* dep_cell = dynamic_cast<Cell*>(GetCell(ref_pos));
		assert(dep_cell && "Dependency cell must exist after EnsureCellsExist");
		dep_cell->AddDependentCell(cell);
	}
}

std::unique_ptr<SheetInterface> CreateSheet() {
	return std::make_unique<Sheet>();
}