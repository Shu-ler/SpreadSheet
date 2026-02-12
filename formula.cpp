#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, const FormulaError& fe) {
	return output << "#ARITHM!";
}

namespace {
	class Formula : public FormulaInterface {
	public:
		explicit Formula(std::string expression) try : ast_(ParseFormulaAST(expression)) {
		}
		catch (const ParsingError&) {
			throw FormulaException("Invalid formula syntax");
		}

		Value Evaluate() const override {
			try {
				return ast_.Execute();
			}
			catch (const std::runtime_error& e) {
				return FormulaError(e.what());
			}
		}

		std::string GetExpression() const override {
			std::ostringstream out;
			ast_.PrintFormula(out);
			return out.str();
		}
	private:
		FormulaAST ast_;
	};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
	return std::make_unique<Formula>(std::move(expression));
}