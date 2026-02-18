#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>
#include <variant>

/*
 *  ласс Cell представл€ет €чейку в электронной таблице.
 * ѕоддерживает три типа содержимого: пустое, текстовое и формулу.
 * »спользует паттерн Pimpl (указатель на интерфейс реализации).
 */
class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell() = default;

    // ”станавливает содержимое €чейки.
    // ≈сли текст начинаетс€ с '=', интерпретируетс€ как формула.
    // ѕуста€ строка делает €чейку пустой.
    // Ѕросает FormulaException при синтаксической ошибке.
    // Ѕросает CircularDependencyException при циклической зависимости.
    void Set(std::string text) override;

    // ќчищает содержимое €чейки, делает еЄ пустой.
    void Clear();

    // ¬озвращает true, если на эту €чейку ссылаютс€ другие €чейки.
    bool IsReferenced() const override;

    // ¬озвращает значение €чейки:
    // - текст: строка (без экранирующего символа)
    // - формула: double или FormulaError
    // - пусто: ""
    // ¬алидирует кэш €чейки
    Value GetValue() const override;

    // ¬озвращает текстовое содержимое (как при редактировании):
    // - текст: оригинальный текст (включа€ ESCAPE_SIGN)
    // - формула: "=..."
    // - пусто: ""
    std::string GetText() const override;

    // ¬озвращает список позиций €чеек, на которые ссылаетс€ формула.
    // ƒл€ текстовых и пустых €чеек Ч пустой вектор.
    // –езультат отсортирован и не содержит дубликатов.
    std::vector<Position> GetReferencedCells() const override;

    // »нвалидирует кэш €чейки и зависимых €чеек (рекурси€)
    void InvalidateCache();

private:
    // јбстрактный базовый класс дл€ реализации содержимого €чейки.
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

private:
    std::unique_ptr<Impl> impl_;

    // —сылка на лист Ч нужна дл€ проверки циклических зависимостей
    Sheet& sheet_;

    // ѕозиции €чеек, от которых зависит эта (дл€ проверки циклов)
    std::vector<Position> referenced_cells_;

    // ячейки, которые завис€т от этой (дл€ обновлени€ при изменени€х)
    std::unordered_set<Cell*> dependents_;

    //  эш значени€ формулы (оптимизаци€ повторных вычислений)
    mutable std::optional<FormulaInterface::Value> cache_;
};

/*
 * »нтерфейс реализации €чейки (Pimpl)
 */
class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

/*
 * ѕуста€ €чейка
 */
class Cell::EmptyImpl : public Cell::Impl {
public:
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
};

/*
 * “екстова€ €чейка
 */
class Cell::TextImpl : public Cell::Impl {
public:
    explicit TextImpl(std::string text);
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    std::string text_;
};

/*
 * ‘ормульна€ €чейка
 */
class Cell::FormulaImpl : public Cell::Impl {
public:
    explicit FormulaImpl(std::string expression);
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    std::unique_ptr<FormulaInterface> formula_;
};