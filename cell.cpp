#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <memory>
#include <algorithm>

/*
 * Интерфейс реализации ячейки (Pimpl)
 */
class Cell::Impl {
public:
	virtual ~Impl() = default;
	virtual Value GetValue() const = 0;
	virtual std::string GetText() const = 0;
	virtual std::vector<Position> GetReferencedCells() const;
};

/*
 * Пустая ячейка
 */
class Cell::EmptyImpl : public Cell::Impl {
public:
	Value GetValue() const override {
		return 0.0;
	}

	std::string GetText() const override {
		return "";
	}

	std::vector<Position> GetReferencedCells() const override {
		return {};
	};
};

/*
 * Текстовая ячейка
 */
class Cell::TextImpl : public Cell::Impl {
private:
	std::string text_;

public:
	explicit TextImpl(std::string text)
		: text_(std::move(text)) {
	}

	Value GetValue() const override {
		if (text_.empty()) {
			return "";
		}

		if (text_[0] == ESCAPE_SIGN) {
			return text_.substr(1);
		}

		return text_;
	}

	std::string GetText() const override {
		return text_;
	}

	std::vector<Position> GetReferencedCells() const override {
		return {};
	}
};

/*
 * Формульная ячейка
 */
class Cell::FormulaImpl : public Cell::Impl {
private:
	std::unique_ptr<FormulaInterface> formula_;

public:
	explicit FormulaImpl(std::string expression)
		: formula_(ParseFormula(std::move(expression))) {
	}

	Value GetValue() const override {
		auto value = formula_->Evaluate();
		if (std::holds_alternative<double>(value)) {
			return std::get<double>(value);
		}
		else {
			return std::get<FormulaError>(value);
		}
	}

	std::string GetText() const override {
		return FORMULA_SIGN + formula_->GetExpression();
	}

	std::vector<Position> GetReferencedCells() const override {
		return formula_->GetReferencedCells();
	}
};

/*
 * Реализация класса Cell
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
	return impl_->GetValue();
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


