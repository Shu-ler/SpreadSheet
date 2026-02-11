#include "common.h"
#include "test_runner_p.h"
#include "FormulaAST.h"

#include <iostream>
#include <string>
#include <string_view>

inline std::ostream& operator<<(std::ostream& output, Position pos) {
	return output << "(" << pos.row << ", " << pos.col << ")";
}

inline Position operator"" _pos(const char* str, std::size_t) {
	return Position::FromString(str);
}

namespace {
	double ExecuteASTFormula(const std::string& expression) {
		return ParseFormulaAST(expression).Execute();
	}
}  // namespace

namespace tests {
	void TestPositionAndStringConversion() {
		auto test_single = [](Position pos, std::string_view str) {
			ASSERT_EQUAL(pos.ToString(), str);
			ASSERT_EQUAL(Position::FromString(str), pos);
			};

		for (int i = 0; i < 25; ++i) {
			test_single(Position{ i, i }, char('A' + i) + std::to_string(i + 1));
		}

		test_single(Position{ 0, 0 }, "A1");
		test_single(Position{ 0, 1 }, "B1");
		test_single(Position{ 0, 25 }, "Z1");
		test_single(Position{ 0, 26 }, "AA1");
		test_single(Position{ 0, 27 }, "AB1");
		test_single(Position{ 0, 51 }, "AZ1");
		test_single(Position{ 0, 52 }, "BA1");
		test_single(Position{ 0, 53 }, "BB1");
		test_single(Position{ 0, 77 }, "BZ1");
		test_single(Position{ 0, 78 }, "CA1");
		test_single(Position{ 0, 701 }, "ZZ1");
		test_single(Position{ 0, 702 }, "AAA1");
		test_single(Position{ 136, 2 }, "C137");
		test_single(Position{ Position::MAX_ROWS - 1, Position::MAX_COLS - 1 }, "XFD16384");
	}

	void TestPositionToStringInvalid() {
		ASSERT_EQUAL((Position::NONE).ToString(), "");
		ASSERT_EQUAL((Position{ -10, 0 }).ToString(), "");
		ASSERT_EQUAL((Position{ 1, -3 }).ToString(), "");
	}

	void TestStringToPositionInvalid() {
		ASSERT(!Position::FromString("").IsValid());
		ASSERT(!Position::FromString("A").IsValid());
		ASSERT(!Position::FromString("1").IsValid());
		ASSERT(!Position::FromString("e2").IsValid());
		ASSERT(!Position::FromString("A0").IsValid());
		ASSERT(!Position::FromString("A-1").IsValid());
		ASSERT(!Position::FromString("A+1").IsValid());
		ASSERT(!Position::FromString("R2D2").IsValid());
		ASSERT(!Position::FromString("C3PO").IsValid());
		ASSERT(!Position::FromString("XFD16385").IsValid());
		ASSERT(!Position::FromString("XFE16384").IsValid());
		ASSERT(!Position::FromString("A1234567890123456789").IsValid());
		ASSERT(!Position::FromString("ABCDEFGHIJKLMNOPQRS8").IsValid());
	}

	void TestExecuteASTFormula() {
		ASSERT_EQUAL(ExecuteASTFormula("1"), 1.0);
		ASSERT_EQUAL(ExecuteASTFormula("1+2*3-4/5"), 6.2);
		
		try {
			ExecuteASTFormula("1/0");
		}
		catch (const FormulaError& fe) {
			std::cout << fe.what() << std::endl;
		}

		std::cout << "Tests Passed" << std::endl;
	}

	void RunAll(TestRunner& tr) {
		RUN_TEST(tr, tests::TestPositionAndStringConversion);
		RUN_TEST(tr, tests::TestPositionToStringInvalid);
		RUN_TEST(tr, tests::TestStringToPositionInvalid);

		TestExecuteASTFormula();
	}
} // namespace tests

int main() {
	TestRunner tr;
	tests::RunAll(tr);
	return 0;
}