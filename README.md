# Описание проекта

## Оптимизированный класс `ring_buffer`

Этот проект представляет собой оптимизированную реализацию класса `ring_buffer`, предназначенного для использования в многопоточных приложениях. Основные изменения и улучшения включают:

1. **Оптимизация выравнивания**:
   - Применение спецификатора `alignas(64)` для переменных `head_` и `tail_` предотвращает ложное совместное использование и улучшает производительность за счет выравнивания переменных по границе кэш-линии.

2. **Улучшенное управление памятью**:
   - Использование битовой маски для эффективного управления оборачиванием индексов в кольцевом буфере. Это позволяет избежать затратных операций с остатком от деления.

3. **Улучшенная синхронизация**:
   - Применение порядков операций памяти `std::memory_order_relaxed`, `std::memory_order_acquire` и `std::memory_order_release` для обеспечения корректной синхронизации между потоками при выполнении операций чтения и записи.

Результаты оптимизации класса `ring_buffer`:

`ring_buffer` до оптимизации:

```
 *  Executing task: /home/igor/projects/CPP/initi/main.out

producer_time: 1081ms; consumer_time: 1081ms
producer_time: 1101ms; consumer_time: 1101ms
producer_time: 1093ms; consumer_time: 1093ms
producer_time: 1087ms; consumer_time: 1087ms
producer_time: 1087ms; consumer_time: 1087ms
producer_time: 1079ms; consumer_time: 1079ms
producer_time: 1059ms; consumer_time: 1058ms
producer_time: 1074ms; consumer_time: 1074ms
producer_time: 1075ms; consumer_time: 1075ms
producer_time: 1089ms; consumer_time: 1089ms
```
`ring_buffer` после оптимизации:
```
 *  Executing task: /home/igor/projects/CPP/initi/mainOptimized.out 

producer_time: 89ms; consumer_time: 89ms
producer_time: 52ms; consumer_time: 52ms
producer_time: 80ms; consumer_time: 80ms
producer_time: 59ms; consumer_time: 59ms
producer_time: 85ms; consumer_time: 85ms
producer_time: 74ms; consumer_time: 74ms
producer_time: 82ms; consumer_time: 82ms
producer_time: 69ms; consumer_time: 69ms
producer_time: 89ms; consumer_time: 89ms
producer_time: 68ms; consumer_time: 68ms
```
