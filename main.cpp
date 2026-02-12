//#include "common.h"
//#include "test_runner_p.h"
//#include "FormulaAST.h"
//
//#include <iostream>
//#include <string>
//#include <string_view>
//
//inline std::ostream& operator<<(std::ostream& output, Position pos) {
//	return output << "(" << pos.row << ", " << pos.col << ")";
//}
//
//inline Position operator"" _pos(const char* str, std::size_t) {
//	return Position::FromString(str);
//}
//
//namespace {
//	double ExecuteASTFormula(const std::string& expression) {
//		return ParseFormulaAST(expression).Execute();
//	}
//}  // namespace
//
//namespace tests {
//	void TestPositionAndStringConversion() {
//		auto test_single = [](Position pos, std::string_view str) {
//			ASSERT_EQUAL(pos.ToString(), str);
//			ASSERT_EQUAL(Position::FromString(str), pos);
//			};
//
//		for (int i = 0; i < 25; ++i) {
//			test_single(Position{ i, i }, char('A' + i) + std::to_string(i + 1));
//		}
//
//		test_single(Position{ 0, 0 }, "A1");
//		test_single(Position{ 0, 1 }, "B1");
//		test_single(Position{ 0, 25 }, "Z1");
//		test_single(Position{ 0, 26 }, "AA1");
//		test_single(Position{ 0, 27 }, "AB1");
//		test_single(Position{ 0, 51 }, "AZ1");
//		test_single(Position{ 0, 52 }, "BA1");
//		test_single(Position{ 0, 53 }, "BB1");
//		test_single(Position{ 0, 77 }, "BZ1");
//		test_single(Position{ 0, 78 }, "CA1");
//		test_single(Position{ 0, 701 }, "ZZ1");
//		test_single(Position{ 0, 702 }, "AAA1");
//		test_single(Position{ 136, 2 }, "C137");
//		test_single(Position{ Position::MAX_ROWS - 1, Position::MAX_COLS - 1 }, "XFD16384");
//	}
//
//	void TestPositionToStringInvalid() {
//		ASSERT_EQUAL((Position::NONE).ToString(), "");
//		ASSERT_EQUAL((Position{ -10, 0 }).ToString(), "");
//		ASSERT_EQUAL((Position{ 1, -3 }).ToString(), "");
//	}
//
//	void TestStringToPositionInvalid() {
//		ASSERT(!Position::FromString("").IsValid());
//		ASSERT(!Position::FromString("A").IsValid());
//		ASSERT(!Position::FromString("1").IsValid());
//		ASSERT(!Position::FromString("e2").IsValid());
//		ASSERT(!Position::FromString("A0").IsValid());
//		ASSERT(!Position::FromString("A-1").IsValid());
//		ASSERT(!Position::FromString("A+1").IsValid());
//		ASSERT(!Position::FromString("R2D2").IsValid());
//		ASSERT(!Position::FromString("C3PO").IsValid());
//		ASSERT(!Position::FromString("XFD16385").IsValid());
//		ASSERT(!Position::FromString("XFE16384").IsValid());
//		ASSERT(!Position::FromString("A1234567890123456789").IsValid());
//		ASSERT(!Position::FromString("ABCDEFGHIJKLMNOPQRS8").IsValid());
//	}
//
//	void TestExecuteASTFormula() {
//		ASSERT_EQUAL(ExecuteASTFormula("1"), 1.0);
//		ASSERT_EQUAL(ExecuteASTFormula("1+2*3-4/5"), 6.2);
//		
//		try {
//			ExecuteASTFormula("1/0");
//		}
//		catch (const FormulaError& fe) {
//			std::cout << fe.what() << std::endl;
//		}
//
//		std::cout << "Tests Passed" << std::endl;
//	}
//
//	void RunAll(TestRunner& tr) {
//		RUN_TEST(tr, tests::TestPositionAndStringConversion);
//		RUN_TEST(tr, tests::TestPositionToStringInvalid);
//		RUN_TEST(tr, tests::TestStringToPositionInvalid);
//
//		TestExecuteASTFormula();
//	}
//} // namespace tests
//
//int main() {
//	TestRunner tr;
//	tests::RunAll(tr);
//	return 0;
//}

//#include "FormulaAST.h"
//#include "common.h"
//#include "formula.h"
//#include "test_runner_p.h"
//
//using Value = FormulaInterface::Value;
//
//namespace {
//	Value ExecuteFormula(const std::string& expression) {
//		return ParseFormula(expression)->Evaluate();
//	}
//}  // namespace
//
//int main() {
//	try {
//		auto empty_formula = ParseFormula("");
//	}
//	catch (const FormulaException& exc) {
//		std::cout << exc.what() << std::endl;
//	}
//
//	try {
//		auto wrong_formula = ParseFormula("A1");
//	}
//	catch (const FormulaException& exc) {
//		std::cout << exc.what() << std::endl;
//	}
//
//	try {
//		auto wrong_formula = ParseFormula("22/(22)+++++");
//	}
//	catch (const FormulaException& exc) {
//		std::cout << exc.what() << std::endl;
//	}
//
//	ASSERT_EQUAL(std::get<double>(ExecuteFormula("1")), 1.0);
//	ASSERT_EQUAL(std::get<double>(ExecuteFormula("1+2*3-4/5")), 6.2);
//
//	std::cout << std::get<FormulaError>(ExecuteFormula("1/0")) << std::endl;
//
//	auto complex_formula = ParseFormula("((22/(22+3))-(8*5))");
//	ASSERT_EQUAL(complex_formula->GetExpression(), "22/(22+3)-8*5");
//
//	std::cout << "Tests Passed" << std::endl;
//}

//#include "cell.h"
//#include "common.h"
//#include "formula.h"
//#include "test_runner_p.h"
//
//inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
//    std::visit([&](const auto& x) { output << x; }, value);
//    return output;
//}
//
//std::unique_ptr<CellInterface> CreateCell(const std::string& str) {
//    std::unique_ptr<CellInterface> cell = std::make_unique<Cell>();
//    cell->Set(str);
//    return cell;
//}
//
//int main()
//{
//
//    auto simple_text = CreateCell("simple_text");
//    ASSERT_EQUAL(simple_text->GetText(), "simple_text");
//    ASSERT_EQUAL(std::get<std::string>(simple_text->GetValue()), "simple_text");
//
//    auto empty_apostrophe = CreateCell("'");
//    ASSERT_EQUAL(empty_apostrophe->GetText(), "'");
//    ASSERT_EQUAL(std::get<std::string>(empty_apostrophe->GetValue()), "");
//
//    auto apostrophe = CreateCell("'apostrophe");
//    ASSERT_EQUAL(apostrophe->GetText(), "'apostrophe");
//    ASSERT_EQUAL(std::get<std::string>(apostrophe->GetValue()), "apostrophe");
//
//    auto text_formula = CreateCell("'=1+2");
//    ASSERT_EQUAL(text_formula->GetText(), "'=1+2");
//    ASSERT_EQUAL(std::get<std::string>(text_formula->GetValue()), "=1+2");
//
//    auto empty_formula = CreateCell("=");
//    ASSERT_EQUAL(empty_formula->GetText(), "=");
//    ASSERT_EQUAL(std::get<std::string>(empty_formula->GetValue()), "=");
//
//
//    auto formula = CreateCell("=1+2");
//    ASSERT_EQUAL(formula->GetText(), "=1+2");
//    ASSERT_EQUAL(std::get<double>(formula->GetValue()), 3);
//
//    auto switch_text = CreateCell("1+2");
//    ASSERT_EQUAL(switch_text->GetText(), "1+2");
//    ASSERT_EQUAL(std::get<std::string>(switch_text->GetValue()), "1+2");
//
//    switch_text->Set("=1+2");
//    ASSERT_EQUAL(switch_text->GetText(), "=1+2");
//    ASSERT_EQUAL(std::get<double>(switch_text->GetValue()), 3);
//
//    switch_text->Set("=1/0");
//    ASSERT_EQUAL(switch_text->GetText(), "=1/0");
//    std::cout << std::get<FormulaError>(switch_text->GetValue()) << std::endl;
//
//    std::cout << "All tests passed" << std::endl;
//}

#include "common.h"
#include "test_runner_p.h"

inline std::ostream& operator<<(std::ostream& output, Position pos) {
    return output << "(" << pos.row << ", " << pos.col << ")";
}

inline Position operator"" _pos(const char* str, std::size_t) {
    return Position::FromString(str);
}

inline std::ostream& operator<<(std::ostream& output, Size size) {
    return output << "(" << size.rows << ", " << size.cols << ")";
}

inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit(
        [&](const auto& x) {
            output << x;
        },
        value);
    return output;
}

namespace {

    void TestEmpty() {
        auto sheet = CreateSheet();
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 0, 0 }));
    }

    void TestInvalidPosition() {
        auto sheet = CreateSheet();
        try {
            sheet->SetCell(Position{ -1, 0 }, "");
        }
        catch (const InvalidPositionException&) {
        }
        try {
            sheet->GetCell(Position{ 0, -2 });
        }
        catch (const InvalidPositionException&) {
        }
        try {
            sheet->ClearCell(Position{ Position::MAX_ROWS, 0 });
        }
        catch (const InvalidPositionException&) {
        }
    }

    void TestSetCellPlainText() {
        auto sheet = CreateSheet();

        auto checkCell = [&](Position pos, std::string text) {
            sheet->SetCell(pos, text);
            CellInterface* cell = sheet->GetCell(pos);
            ASSERT(cell != nullptr);
            ASSERT_EQUAL(cell->GetText(), text);
            ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), text);
            };

        checkCell("A1"_pos, "Hello");
        checkCell("A1"_pos, "World");
        checkCell("B2"_pos, "Purr");
        checkCell("A3"_pos, "Meow");

        const SheetInterface& constSheet = *sheet;
        ASSERT_EQUAL(constSheet.GetCell("B2"_pos)->GetText(), "Purr");

        sheet->SetCell("A3"_pos, "'=escaped");
        CellInterface* cell = sheet->GetCell("A3"_pos);
        ASSERT_EQUAL(cell->GetText(), "'=escaped");
        ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), "=escaped");
    }

    void TestClearCell() {
        auto sheet = CreateSheet();

        sheet->SetCell("C2"_pos, "Me gusta");
        sheet->ClearCell("C2"_pos);
        ASSERT(sheet->GetCell("C2"_pos) == nullptr);

        sheet->ClearCell("A1"_pos);
        sheet->ClearCell("J10"_pos);
    }
    void TestPrint() {
        auto sheet = CreateSheet();
        sheet->SetCell("A2"_pos, "meow");
        sheet->SetCell("B2"_pos, "=1+2");
        sheet->SetCell("A1"_pos, "=1/0");

        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 2, 2 }));

        std::ostringstream texts;
        sheet->PrintTexts(texts);
        ASSERT_EQUAL(texts.str(), "=1/0\t\nmeow\t=1+2\n");

        std::ostringstream values;
        sheet->PrintValues(values);
        ASSERT_EQUAL(values.str(), "#ARITHM!\t\nmeow\t3\n");

        sheet->ClearCell("B2"_pos);
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 2, 1 }));
    }

}  // namespace

int main() {
    TestRunner tr;
    RUN_TEST(tr, TestEmpty);
    RUN_TEST(tr, TestInvalidPosition);
    RUN_TEST(tr, TestSetCellPlainText);
    RUN_TEST(tr, TestClearCell);
    RUN_TEST(tr, TestPrint);
}