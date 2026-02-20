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
	virtual std::vector<Position> GetReferencedCells() const {
		return {};
	};
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
};

/*
 * Формульная ячейка
 */
class Cell::FormulaImpl : public Cell::Impl {
private:
	std::unique_ptr<FormulaInterface> formula_;
	Sheet& sheet_;

	// Позиции ячеек, на которые ссылается формула (для проверки циклов)
	std::vector<Position> referenced_cells_;

public:
	explicit FormulaImpl(std::string expression, Sheet& sheet)
		: formula_(ParseFormula(std::move(expression)))
		, sheet_(sheet) {
		referenced_cells_ = formula_->GetReferencedCells();
		std::sort(referenced_cells_.begin(), referenced_cells_.end());
		referenced_cells_.erase(std::unique(referenced_cells_.begin(), referenced_cells_.end()), referenced_cells_.end());
	}

	Value GetValue() const override {
		auto value = formula_->Evaluate(sheet_);
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
		return referenced_cells_;
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
	if (text.empty()) {
		impl_ = std::make_unique<EmptyImpl>();
		return;
	}

	if (text[0] == FORMULA_SIGN && text.size() > 1) {
		try {
			impl_ = std::make_unique<FormulaImpl>(std::move(text.substr(1)), sheet_);
			return;
		}
		catch (const FormulaException&) {
			throw;
		}
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

void Cell::AddDependentCell(Cell* dependent) {
	dependents_.insert(dependent);
}

void Cell::RemoveDependentCell(Cell* dependent) {
	dependents_.erase(dependent);
}


