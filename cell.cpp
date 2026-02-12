#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <memory>

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    Value GetValue() const override {
        return "";
    }
    
    std::string GetText() const override {
        return "";
    }
};

class Cell::TextImpl : public Cell::Impl {
public:
    TextImpl(std::string text) : text_(std::move(text)) {}

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
    
private:
    std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    FormulaImpl(std::string expression) : formula_(ParseFormula(std::move(expression))) {}

    Value GetValue() const override {
        auto value = formula_->Evaluate();
        if (std::holds_alternative<double>(value)) {
            return std::get<double>(value);
        } else {
            return std::get<FormulaError>(value);
        }
    }
    
    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }
    
private:
    std::unique_ptr<FormulaInterface> formula_;
};

Cell::Cell() : impl_(std::make_unique<EmptyImpl>()) {}

Cell::~Cell() = default;

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
        } catch (const FormulaException&) {
            throw;
        }
        return;
    }
    
    impl_ = std::make_unique<TextImpl>(std::move(text));
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}