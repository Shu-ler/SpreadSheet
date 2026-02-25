#pragma once

#include "common.h"

#include <memory>
#include <unordered_set>
#include <vector>
#include <string>
#include <string_view>

class Sheet;  // Forward declaration

/*
 * Класс Cell представляет ячейку в электронной таблице.
 * Поддерживает три типа содержимого: пустое, текстовое и формулу.
 * Использует паттерн Pimpl (указатель на интерфейс реализации).
 */
class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell();

    // Устанавливает содержимое ячейки.
    // Если текст начинается с '=', интерпретируется как формула.
    // Пустая строка делает ячейку пустой.
    // Бросает FormulaException при синтаксической ошибке.
    void Set(std::string text);

    // Очищает содержимое ячейки, делает её пустой.
    void Clear();

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

    // Возвращает список позиций ячеек, которые завясят от текущей.
    std::unordered_set<Cell*> GetDependentsCells() const;

    // Инвалидирует кэш ячейки и зависимых ячеек (рекурсия)
    void InvalidateCache();

    // Добавляет ячейку в контейнер зависимых ячеек
    void AddDependentCell(Cell* dependent);
    
    // Удаляет ячейку из контейнера зависимых ячеек
    void RemoveDependentCell(Cell* dependent);

    // Проверяет, является ли текст формулой: начинается с '=' и длина > 1
    static bool IsFormulaText(std::string_view text);

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

private:
    std::unique_ptr<Impl> impl_;

    // Ссылка на лист — нужна для проверки циклических зависимостей
    Sheet& sheet_;

    // Ячейки, которые зависят от этой (для инвалидации кэша)
    std::unordered_set<Cell*> dependents_;
};