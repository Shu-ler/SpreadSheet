#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, const FormulaError& fe) {
	return output << fe.ToString();
}

namespace {

	class Formula : public FormulaInterface {
	public:
		explicit Formula(std::string expression) try 
			: ast_(ParseFormulaAST(expression)) {
		}
		catch (const ParsingError& error) {
			throw FormulaException(error.what());
		}

		Value Evaluate(const SheetInterface& sheet) const override {
			try {
				return ast_.Execute(sheet);
			}
			catch (const FormulaError& error) {
				return error;
			}
		}

		std::string GetExpression() const override {
			std::ostringstream out;
			ast_.PrintFormula(out);
			return out.str();
		}

		std::vector<Position> GetReferencedCells() const override {
			std::vector<Position> cells;
			for (const Position& pos : ast_.GetCells()) {
				if (pos.IsValid()) {
					cells.push_back(pos);
				}
			}
			
			// Сортируем и удаляем дубликаты
			std::sort(cells.begin(), cells.end());
			cells.erase(unique(cells.begin(), cells.end()), cells.end());
			return cells;
		}

	private:
		FormulaAST ast_;
	};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
	return std::make_unique<Formula>(std::move(expression));
}