#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <memory>
#include <algorithm>

/*
 * Cell
 */

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {
}

void Cell::Set(std::string text) {
	// Очищаем кэш перед любыми изменениями
	cache_.reset();

	// Сначала удаляем эту ячейку из dependents_ всех, на кого ссылались
	for (const auto& pos : referenced_cells_) {
		if (auto* cell = sheet_.GetCell(pos)) {
			// Предполагается, что GetCell возвращает Cell*, а не const CellInterface*
			static_cast<Cell*>(cell)->dependents_.erase(this);
		}
	}
	referenced_cells_.clear();

	if (text.empty()) {
		impl_ = std::make_unique<EmptyImpl>();
		return;
	}

	if (text[0] == FORMULA_SIGN) {
		if (text.size() == 1) {
			impl_ = std::make_unique<TextImpl>(std::move(text));
			return;
		}

		try {
			auto formula = ParseFormula(text.substr(1));

			// Получаем список ссылок до установки
			auto refs = formula->GetReferencedCells();
			std::sort(refs.begin(), refs.end());
			refs.erase(std::unique(refs.begin(), refs.end()), refs.end());

			// Проверяем циклические зависимости
			std::unordered_set<Position> visited;
			std::function<bool(const Cell*)> has_cycle = [&](const Cell* cell) {
				if (!cell) return false;
				Position this_pos = sheet_.GetPosition(this);
				if (visited.count(this_pos)) return true;
				visited.insert(this_pos);

				for (const auto& dep_pos : cell->referenced_cells_) {
					if (auto* dep_cell = dynamic_cast<Cell*>(sheet_.GetCell(dep_pos))) {
						if (has_cycle(dep_cell)) {
							return true;
						}
					}
				}
				return false;
				};

			// Временно устанавливаем ссылки для проверки
			referenced_cells_ = refs;
			if (has_cycle(this)) {
				referenced_cells_.clear();
				throw CircularDependencyException("Cyclic dependency detected");
			}

			// Сохраняем зависимости
			for (const auto& pos : referenced_cells_) {
				if (auto* cell = dynamic_cast<Cell*>(sheet_.GetCell(pos))) {
					cell->dependents_.insert(this);
				}
			}

			impl_ = std::make_unique<FormulaImpl>(std::move(formula));
		}
		catch (const FormulaException&) {
			throw;
		}
		return;
	}

	impl_ = std::make_unique<TextImpl>(std::move(text));
}

void Cell::Clear() {
    Set("");  // используем Set для корректной очистки зависимостей
}

bool Cell::IsReferenced() const {
    return !dependents_.empty();
}

Cell::Value Cell::GetValue() const {
    if (impl_->GetValue().index() == 0 && cache_.has_value()) {
        // Если значение — строка, но у нас кэш (например, после вычисления формулы)
        // На самом деле, кэш нужен только для формул
        // Лучше: кэш только для FormulaImpl
    }

    if (cache_) {
        return *cache_;
    }

    Value value = impl_->GetValue();

    // Только формулы могут кэшироваться
    if (dynamic_cast<const FormulaImpl*>(impl_.get())) {
        cache_ = value;
    }

    return value;
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::InvalidateCache() {
    cache_.reset();
    for (Cell* dependent : dependents_) {
        dependent->InvalidateCache();
    }
}

/*
 * Cell::EmptyImpl
 */

Value Cell::EmptyImpl::GetValue() const {
    return "";
}

std::string Cell::EmptyImpl::GetText() const {
    return "";
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const {
    return {};
}


/*
 * Cell::TextImpl
 */

Cell::TextImpl::TextImpl(std::string text)
    : text_(std::move(text)) {
}

Value Cell::TextImpl::GetValue() const {
    if (text_.empty()) {
        return "";
    }

    if (text_[0] == ESCAPE_SIGN) {
        return text_.substr(1);
    }

    return text_;
}

std::string Cell::TextImpl::GetText() const {
    return text_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const {
    return {};
}


/*
 * Cell::FormulaImpl
 */

Cell::FormulaImpl::FormulaImpl(std::unique_ptr<FormulaInterface> formula)
    : formula_(std::move(formula)) {
}

Value Cell::FormulaImpl::GetValue() const {
    return formula_->Evaluate();
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}