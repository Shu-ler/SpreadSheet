#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <memory>

/*
 * Cell
 */

Cell::Cell(Sheet& sheet)
	: impl_(std::make_unique<EmptyImpl>()) {
}

void Cell::Set(std::string text) {
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
			impl_ = std::make_unique<FormulaImpl>(text.substr(1));
		}
		catch (const FormulaException&) {
			throw;
		}
		return;
	}

	impl_ = std::make_unique<TextImpl>(std::move(text));
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
}

bool Cell::IsReferenced() const {
	return false;
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return std::vector<Position>();
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


/*
 * Cell::FormulaImpl
 */

Cell::FormulaImpl::FormulaImpl(std::string expression)
	: formula_(ParseFormula(std::move(expression))) {
}

Value Cell::FormulaImpl::GetValue() const {
	auto value = formula_->Evaluate();
	if (std::holds_alternative<double>(value)) {
		return std::get<double>(value);
	}
	else {
		return std::get<FormulaError>(value);
	}
}

std::string Cell::FormulaImpl::GetText() const {
	return FORMULA_SIGN + formula_->GetExpression();
}
