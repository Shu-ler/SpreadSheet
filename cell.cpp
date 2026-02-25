#include "cell.h"
#include "sheet.h"
#include "formula.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

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
	virtual void InvalidateCacheImpl() {}
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

	// Возвращает значение.
	// Сюда перенесена попытка преобразования текста в число
	// из CellExpr::Evaluate
	Value GetValue() const override {
		// Для пустого - 0.0
		if (text_.empty()) {
			return 0.0;
		}

		// Для состоящего из одного префикса - 0.0
		std::string_view content = text_;
		if (content[0] == ESCAPE_SIGN) {
			content.remove_prefix(1);
			if (content.empty()) {
				return 0.0;
			}
		}

		char* end;
		double num = strtod(content.data(), &end);

		// Пропускаем пробелы после
		while (*end == ' ' || *end == '\t') ++end;

		if (end != content.data() && *end == '\0') {
			if (!std::isfinite(num)) {
				return FormulaError(FormulaError::Category::Arithmetic);
			}
			return num;
		}

		return std::string(content);
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

	// Кэш значения формулы (оптимизация повторных вычислений)
	mutable std::optional<Value> cache_;

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
		if (!cache_.has_value()) {
			FormulaInterface::Value result = formula_->Evaluate(sheet_);

			// Преобразуем FormulaInterface::Value в CellInterface::Value
			if (std::holds_alternative<double>(result)) {
				cache_ = std::get<double>(result);
			}
			else {
				cache_ = std::get<FormulaError>(result);
			}
		}
		return *cache_;
	}

	std::string GetText() const override {
		return FORMULA_SIGN + formula_->GetExpression();
	}

	std::vector<Position> GetReferencedCells() const override {
		return referenced_cells_;
	}

	void InvalidateCacheImpl() override {
		cache_.reset();
	}
};

/*
 * Реализация класса Cell
 */

Cell::Cell(Sheet& sheet)
	: impl_(std::make_unique<EmptyImpl>())
	, sheet_(sheet) {
}

// Деструктор вынесен сюда,
// иначе тренажер не может обработать cell.h с неполной реализацией Cell::Impl
Cell::~Cell() = default;

void Cell::Set(std::string text) {
	if (text.empty()) {
		impl_ = std::make_unique<EmptyImpl>();
		return;
	}

	if (IsFormulaText(text)) {
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

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

std::unordered_set<Cell*> Cell::GetDependentsCells() const {
	return dependents_;
}

void Cell::InvalidateCache() {
	impl_->InvalidateCacheImpl();
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

bool Cell::IsFormulaText(std::string_view text) {
	return text.size() > 1 && text[0] == FORMULA_SIGN;
}