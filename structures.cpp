#include "common.h"

#include <cctype>
#include <sstream>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = { -1, -1 };

/*
 * Реализация класса FormulaError
 */

FormulaError::FormulaError(Category category) : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
	return category_;
}

std::string_view FormulaError::ToString() const {
	switch (category_) {
		case Category::Ref:			return "#REF!";
		case Category::Value:		return "#VALUE!";
		case Category::Arithmetic:	return "#ARITHM!";
		default:					return "#ERROR!";
	}
}

bool FormulaError::operator==(FormulaError rhs) const {
	return category_ == rhs.category_;
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
	return output << fe.ToString();
}


/*
 * Реализация структуры Position
 */
bool Position::operator==(const Position rhs) const {
	return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
	if (row != rhs.row) {
		return row < rhs.row;
	}
	return col < rhs.col;
}

bool Position::IsValid() const {
	return row >= 0 && col >= 0 && row < MAX_ROWS && col < MAX_COLS;
}

std::string Position::ToString() const {
	if (!IsValid()) {
		return "";
	}

	std::string result;
	int col = this->col;
	while (col >= 0) {
		result = char('A' + col % LETTERS) + result;
		col = col / LETTERS - 1;
	}
	return result + std::to_string(row + 1);
}

namespace {

	// Разбивает текстовое представление адреса ячейки на представление столбца и строки
	std::pair<std::string_view, std::string_view> SplitCellIndex(const std::string_view& str) {
		size_t col_end = 0;
		while (col_end < str.size() && std::isalpha(str[col_end])) {
			++col_end;
		}

		if (col_end == 0 || col_end == str.size()) {
			return { std::string_view(), std::string_view() };
		}

		return { str.substr(0, col_end), str.substr(col_end) };
	}

	// Проверяет корректность представления столбца и возвращает номер столбца
	// При ошибке возвращает -1
	int ColumnIndexToInt(const std::string_view& col_str) {
		if (col_str.empty() || col_str.size() > 3) {
			return -1;
		}

		int col = 0;
		for (char c : col_str) {
			if (c < 'A' || c > 'Z') {
				return -1;
			}
			col = col * LETTERS + (c - 'A' + 1);
		}

		col -= 1;

		if (col >= Position::MAX_COLS) {
			return -1;
		}

		return col;
	}

	// Проверяет корректность представления строки и возвращает её номер
	// При ошибке возвращает -1
	int RowIndexToInt(const std::string_view& row_str) {
		if (row_str.empty() || row_str[0] == '0') {
			return -1;
		}

		for (char c : row_str) {
			if (!std::isdigit(c)) {
				return -1;
			}
		}

		try {
			int row = std::stoi(std::string(row_str)) - 1;
			if (row < 0 || row >= Position::MAX_ROWS) {
				return -1;
			}
			return row;
		}
		catch (...) {
			return -1;
		}
	}

} // namespace

Position Position::FromString(std::string_view str) {
	// Проверка на минимальную длину
	if (str.size() < 2) {
		return Position::NONE;
	}

	// Разбивка на столбец и строку
	auto [col_str, row_str] = SplitCellIndex(str);

	if (col_str.empty() || row_str.empty()) {
		return Position::NONE;
	}

	// Преобразование в числа
	int col = ColumnIndexToInt(col_str);
	int row = RowIndexToInt(row_str);

	if (col == -1 || row == -1) {
		return Position::NONE;
	}

	return { row, col };
}

bool Size::operator==(Size rhs) const {
	return cols == rhs.cols && rows == rhs.rows;
}