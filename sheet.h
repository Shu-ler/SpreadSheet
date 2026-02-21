#pragma once

#include "common.h"
#include "cell.h"
#include <unordered_map>

#include <functional>

/*
 * Основной класс таблицы, реализующий интерфейс SheetInterface.
 * Управляет набором ячеек, их содержимым, размерами печатной области,
 * а также обеспечивает операции установки, чтения и очистки ячеек.
 */
class Sheet : public SheetInterface {
public:
    Sheet() = default;
    ~Sheet() override = default;

    // Устанавливает содержимое ячейки по указанной позиции.
    // Если текст начинается с '=', интерпретируется как формула.
    // Проверяет корректность позиции, синтаксис формулы и циклические зависимости.
    // Выбрасывает исключения при ошибках.
    // Обновляет размер печатной области.
    // При необходимости создаёт зависимые ячейки
    // Инвалидирует кэш ячейки и зависимых ячеек
    void SetCell(Position pos, std::string text) override;

    // Возвращает константный указатель на ячейку по позиции.
    // Возвращает nullptr, если ячейка пуста или позиция вне диапазона.
    const CellInterface* GetCell(Position pos) const override;
    
    // Неконстантная версия GetCell — позволяет модифицировать ячейку.
    CellInterface* GetCell(Position pos) override;

    // Очищает содержимое ячейки (устанавливает в пустое состояние).
    // Если ячейка существовала, освобождает её ресурсы.
    // Корректирует размер печатной области, если нужно.
    void ClearCell(Position pos) override;

    // Возвращает позицию ячейки
    Position GetPosition(const Cell* cell) const;

    // Возвращает размер прямоугольной области, содержащей все непустые ячейки.
    // Используется для определения границ вывода таблицы.
    Size GetPrintableSize() const override;

    // Печатает значения всех ячеек в заданный поток.
    // Для каждой ячейки вызывает GetValue():
    // - строки выводятся как есть,
    // - числа выводятся как double,
    // - ошибки формул выводятся через operator<<.
    // Пустые ячейки — пустые строки. Столбцы разделены табуляцией.
    void PrintValues(std::ostream& output) const override;

    // Печатает текстовое содержимое всех ячеек (как при редактировании).
    // Для текстовых ячеек — текст, для формул — выражение (например, "=A1+B1").
    // Пустые ячейки — пустые строки. Столбцы разделены табуляцией.
    void PrintTexts(std::ostream& output) const override;

    // Хэш-функция для Position — требуется для использования в unordered_map.
    struct PositionHash {
        size_t operator()(const Position& pos) const {
            // Комбинируем row и col через битовый сдвиг.
            return static_cast<size_t>(pos.row) ^
                (static_cast<size_t>(pos.col) << 16);
        }
    };

private:
    // Обновляет размер печатной области (print_size_) на основе текущих ячеек.
    // Проходит по всем занятым позициям, определяет максимальные индексы.
    void UpdatePrintSize();

    // Аналогично UpdatePrintSize, но вызывается после очистки ячейки,
    // чтобы уменьшить размер области, если последние данные были удалены.
    void ShrinkPrintSize();

    // Проверяет строку на предмет, не формула ли это
    inline bool IsFormula(std::string text);

    // Проверяет, не ссылается ли формула на саму себя
    void CheckSelfReference(const std::vector<Position>& referenced_cells, Position cell_pos);

    // Проверяет, не возникнет ли циклическая зависимость при установке формулы
    void CheckCircularDependency(const std::vector<Position>& refs, Position target_pos);

    // Создаёт пустые ячейки для указанных позиций, если они ещё не существуют
    void EnsureCellsExist(const std::vector<Position>& positions);

private:
    // Хранение ячеек: разреженная таблица на основе хэш-карты.
    // Ключ — позиция (row, col), значение — уникальный указатель на Cell.
    // Эффективно по памяти при разреженных данных.
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHash> cells_;

    // Размер прямоугольника, содержащего все непустые ячейки.
    // Используется для эффективного вывода таблицы (PrintValues/PrintTexts).
    Size print_size_;
};
