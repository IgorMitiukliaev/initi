#include <atomic>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

template <typename T>
class ring_buffer {
   public:
    explicit ring_buffer(size_t capacity) : capacity_(next_power_of_two(capacity)),
                                            mask_(capacity_ - 1),
                                            storage_(capacity_),
                                            head_(0),
                                            tail_(0) {
        assert(capacity > 0);
    }

    bool push(T value) {
        // Так как хвост может обновить только производитель, то используем memory_order_relaxed
        size_t curr_tail = tail_.load(std::memory_order_relaxed);
        // Так как голову может обновить только потребитель, то используем memory_order_acquire
        size_t curr_head = head_.load(std::memory_order_acquire);

        // Проверка на заполнение буфера
        if (((curr_tail + 1) & mask_) == curr_head) {
            return false;  // Буфер заполнен
        }

        storage_[curr_tail] = std::move(value);

        // Обновление хвоста
        // Гарантируем, что все предыдущие операции
        // будут видны другим потокам, которые выполняют операции memory_order_acquire
        tail_.store((curr_tail + 1) & mask_, std::memory_order_release);
        return true;
    }

    // Метод извлечения элемента из буфера
    bool pop(T& value) {
        // Так как голову может обновить только потребитель, то используем memory_order_relaxed
        size_t curr_head = head_.load(std::memory_order_relaxed);
        // Так как хвост может обновить только производитель, то используем memory_order_acquire
        size_t curr_tail = tail_.load(std::memory_order_acquire);

        // Проверка на пустоту буфера
        if (curr_head == curr_tail) {
            return false;  // Буфер пуст
        }

        value = std::move(storage_[curr_head]);

        // Обновление головы
        // Гарантируем, что все предыдущие операции
        // будут видны другим потокам, которые выполняют операции memory_order_acquire
        head_.store((curr_head + 1) & mask_, std::memory_order_release);
        return true;
    }

   private:
    // Вычисление следующей степени двойки для оптимизации работы с индексами
    static size_t next_power_of_two(size_t n) {
        size_t power = 1;
        while (power < n) {
            power <<= 1;
        }
        return power;
    }

    const size_t capacity_;   // Вместимость буфера (степень двойки)
    const size_t mask_;       // Маска для быстрого вычисления индексов
    std::vector<T> storage_;  // Хранилище для элементов
    // Исключаем ложное разделение кэш-строк
    alignas(64) std::atomic<size_t> head_;  // Индекс головы
    alignas(64) std::atomic<size_t> tail_;  // Индекс хвоста
};

#define M_TO_STRING_WRAPPER(x) #x
#define M_TO_STRING(x) M_TO_STRING_WRAPPER(x)
#define M_SOURCE __FILE__ ":" M_TO_STRING(__LINE__)

class stopwatch {
    using clock_type = std::chrono::steady_clock;

   public:
    stopwatch() {
        start_ = clock_type::now();
    }

    template <typename t_duration>
    t_duration elapsed_duration() const {
        using namespace std::chrono;

        auto delta = clock_type::now() - start_;
        return duration_cast<t_duration>(delta);
    }

   private:
    clock_type::time_point start_;
};

class hash_calculator {
   public:
    template <typename t_value>
    void set(const t_value& _value) {
        digest_ = std::hash<t_value>()(_value) ^ (digest_ << 1);
    }

    size_t value() const {
        return digest_;
    }

   private:
    size_t digest_ = 0;
};

void test() {
    constexpr size_t k_count = 10'000'000;
    constexpr size_t k_size = 1024;

    ring_buffer<int> buffer(k_size);

    size_t producer_hash = 0;
    std::chrono::milliseconds producer_time;

    size_t consumer_hash = 0;
    std::chrono::milliseconds consumer_time;

    std::thread producer([&]() {
        hash_calculator hash;
        stopwatch watch;

        for (int i = 0; i < k_count; ++i) {
            hash.set(i);

            while (!buffer.push(i)) {
                std::this_thread::yield();
            }
        }

        producer_time = watch.elapsed_duration<std::chrono::milliseconds>();
        producer_hash = hash.value();
    });

    std::thread consumer([&]() {
        hash_calculator hash;
        stopwatch watch;

        for (int i = 0; i < k_count; ++i) {
            int value;

            while (!buffer.pop(value)) {
                std::this_thread::yield();
            }

            hash.set(value);
        }

        consumer_time = watch.elapsed_duration<std::chrono::milliseconds>();
        consumer_hash = hash.value();
    });

    producer.join();
    consumer.join();

    if (producer_hash != consumer_hash) {
        throw std::runtime_error(M_SOURCE ": workers hash must be equal");
    }

    std::cout << "producer_time: " << producer_time.count() << "ms; "
              << "consumer_time: " << consumer_time.count() << "ms"
              << std::endl;
}

int main() {
    int i = 0;
    while (i++ < 10) {
        test();
    }

    return 0;
}

// g++ -std=c++17 -O2 -pthread main.cpp
// g++ -std=c++17 -O2 -pthread -fsanitize=thread main.cpp
