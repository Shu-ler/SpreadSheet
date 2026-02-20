#pragma once

#include "common.h"
#include "formula.h"

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>
#include <variant>

class Sheet;  // Forward declaration

/*
 * Класс Cell представляет ячейку в электронной таблице.
 * Поддерживает три типа содержимого: пустое, текстовое и формулу.
 * Использует паттерн Pimpl (указатель на интерфейс реализации).
 */
class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell() = default;

    // Устанавливает содержимое ячейки.
    // Если текст начинается с '=', интерпретируется как формула.
    // Пустая строка делает ячейку пустой.
    // Бросает FormulaException при синтаксической ошибке.
    // Бросает CircularDependencyException при циклической зависимости.
    void Set(std::string text);

    // Очищает содержимое ячейки, делает её пустой.
    void Clear();

    // Возвращает true, если на эту ячейку ссылаются другие ячейки.
    bool IsReferenced() const;

    // Возвращает значение ячейки:
    // - текст: строка (без экранирующего символа)
    // - формула: double или FormulaError
    // - пусто: ""
    // Валидирует кэш ячейки
    Value GetValue() const override;

    // Возвращает текстовое содержимое (как при редактировании):
    // - текст: оригинальный текст (включая ESCAPE_SIGN)
    // - формула: "=..."
    // - пусто: ""
    std::string GetText() const override;

    // Возвращает список позиций ячеек, на которые ссылается формула.
    // Для текстовых и пустых ячеек — пустой вектор.
    // Результат отсортирован и не содержит дубликатов.
    std::vector<Position> GetReferencedCells() const override;

    // Инвалидирует кэш ячейки и зависимых ячеек (рекурсия)
    void InvalidateCache();

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

private:
    std::unique_ptr<Impl> impl_;

    // Ссылка на лист — нужна для проверки циклических зависимостей
    Sheet& sheet_;

    // Ячейки, которые зависят от этой (для обновления при изменениях)
    std::unordered_set<Cell*> dependents_;

    // Кэш значения формулы (оптимизация повторных вычислений)
    mutable std::optional<FormulaInterface::Value> cache_;
};
