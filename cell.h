#pragma once

#include "common.h"
#include "formula.h"
#include <memory>

class Cell : public CellInterface {
public:
	using CellDeps = std::vector<Position>;

	Cell(Sheet& sheet);
	~Cell() = default;

	void Set(std::string text);
	void Clear();

	bool IsReferenced() const;

	Value GetValue() const override;
	std::string GetText() const override;
	std::vector<Position> GetReferencedCells() const override;

private:
	class Impl;
	class EmptyImpl;
	class TextImpl;
	class FormulaImpl;

private:
	std::unique_ptr<Impl> impl_;

	CellDeps dependencies_;
	CellDeps dependents_;

	// Êýø
	mutable std::optional<Value> cache_;
	mutable bool cache_valid_ = false;
};

class Cell::Impl {
public:
	virtual ~Impl() = default;

	virtual Value GetValue() const = 0;
	virtual std::string GetText() const = 0;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
	Value GetValue() const override;
	std::string GetText() const override;
};

class Cell::TextImpl : public Cell::Impl {
public:
	TextImpl(std::string text);

	Value GetValue() const override;
	std::string GetText() const override;

private:
	std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
	FormulaImpl(std::string expression);

	Value GetValue() const override;
	std::string GetText() const override;

private:
	std::unique_ptr<FormulaInterface> formula_;
};
