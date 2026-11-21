# Полное руководство по структуре, классам и методам (Лабораторная работа №4, Вариант 11)

В данном документе представлен детальный разбор всей кодовой базы 4-й лабораторной работы: назначение каждой директории и каждого файла, архитектура классов, назначение полей, конструкторов и каждого метода с описанием их сигнатур и внутренней логики.

---

## 1. Общая структура каталогов и файлов проекта

```text
lab4/
├── CMakeLists.txt                      # Главный сборочный конфигуратор CMake
├── COMMANDS.md                         # Шпаргалка со всеми консольными командами
├── CLASSES_AND_METHODS_GUIDE.md        # Настоящее подробное руководство по классам и методам
├── README.md                           # Описание варианта, архитектуры и отчёт о покрытии
├── cli/
│   └── main.cpp                        # Интерактивное консольное приложение (13 пунктов меню)
├── include/
│   ├── graph.hpp                       # Фасадный заголовок для графа и всех его итераторов
│   ├── sorts.hpp                       # Фасадный заголовок для алгоритмов сортировки
│   ├── student.hpp                     # Фасадный заголовок для класса Student
│   ├── graph/
│   │   ├── EdgeNode.hpp                # Узел двусвязного списка инцидентных рёбер
│   │   ├── VertexNode.hpp              # Узел двусвязного списка вершин
│   │   ├── Graph.hpp                   # Основной шаблонный класс-контейнер Graph<T>
│   │   └── iterators/
│   │       ├── AdjacentVertexIterator.hpp         # Двунаправленный итератор по смежным вершинам
│   │       ├── EdgeIterator.hpp                   # Двунаправленный итератор по уникальным рёбрам
│   │       ├── ForwardAdjacentVertexIterator.hpp  # Однонаправленный итератор смежных вершин (Вариант 11)
│   │       ├── IncidentEdgeIterator.hpp           # Двунаправленный итератор по инцидентным рёбрам
│   │       └── VertexIterator.hpp                 # Двунаправленный итератор по вершинам (обычный и const)
│   ├── models/
│   │   └── Student.hpp                 # Заголовок пользовательского демонстрационного класса Student
│   └── sorts/
│       ├── BogoSort.hpp                # Реализация алгоритма Bogosort
│       ├── InPlaceMergeSort.hpp        # Реализация алгоритма In-place merge sort
│       └── SortUtils.hpp               # Вспомогательный предикат проверки упорядоченности
├── src/
│   └── models/
│       └── Student.cpp                 # Реализация методов класса Student
└── tests/
    ├── test_graph.cpp                  # Модульные тесты контейнера Graph и всех 5 итераторов
    └── test_sorts.cpp                  # Модульные тесты сортировок и класса Student
```

---

## 2. Группа: Алгоритмы сортировки (`lab4/include/sorts/`)

### 2.1. Файл `SortUtils.hpp` (Пространство имён `lab4`)
Содержит вспомогательные шаблоны для проверки упорядоченности последовательности.

* `template <typename ForwardIt, typename Compare = std::less<>> bool is_sorted_custom(ForwardIt first, ForwardIt last, Compare comp = Compare())`:
  * **Назначение:** Проверяет, упорядочен ли полуинтервал `[first, last)` в соответствии с отношением строгого слабого порядка `comp`.
  * **Параметры:**
    * `first`, `last` — итераторы, задающие проверяемый диапазон (категория: Forward Iterator или выше).
    * `comp` — бинарный предикат/компаратор (по умолчанию `std::less<>`).
  * **Логика работы:**
    1. Если диапазон пуст (`first == last`), немедленно возвращает `true`.
    2. Создаёт второй итератор `next = first` и сдвигает его на шаг вперёд (`++next`).
    3. В цикле проверяет: если `comp(*next, *first)` истинно, значит текущая пара нарушает порядок, функция возвращает `false`.
    4. Если цикл завершился без нарушений, возвращает `true`.
  * **Сложность:** В худшем случае $O(N)$ сравнений, где $N$ — число элементов; по памяти $O(1)$.

---

### 2.2. Файл `BogoSort.hpp` (Пространство имён `lab4`)
Реализация алгоритма Bogosort (случайная / «обезьянья» сортировка) согласно Варианту 11.

* `template <typename RandomIt, typename Compare = std::less<>> void bogo_sort(RandomIt first, RandomIt last, Compare comp = Compare())`:
  * **Назначение:** Сортирует последовательность `[first, last)` случайным перемешиванием до тех пор, пока она не станет упорядоченной.
  * **Параметры:** Итераторы произвольного доступа `RandomIt`, компаратор `comp`.
  * **Логика работы:**
    1. Базовый случай: если в диапазоне 0 или 1 элемент (`first == last || (first + 1) == last`), завершает работу.
    2. Инициализирует псевдослучайный генератор `std::mt19937` аппаратной энтропией `std::random_device`.
    3. Пока предикат `!is_sorted_custom(first, last, comp)` возвращает `true`, перемешивает элементы диапазона через `std::shuffle(first, last, generator)`.
  * **Сложность:** В лучшем случае $O(N)$, в среднем $O(N \cdot N!)$, в худшем случае не ограничена. Память: $O(1)$.

* `template <typename Container, typename Compare = std::less<>> void bogo_sort_container(Container& container, Compare comp = Compare())`:
  * **Назначение:** Перегрузка-обёртка для контейнеров (например, `std::vector` или C-массивов). Вызывает `bogo_sort(std::begin(container), std::end(container), comp)`.

---

### 2.3. Файл `InPlaceMergeSort.hpp` (Пространство имён `lab4`)
Реализация рекурсивной сортировки слиянием на месте (In-place merge sort) согласно Варианту 11. Не требует выделения дополнительного вспомогательного динамического массива $O(N)$.

* `template <typename RandomIt, typename Compare> void in_place_merge_custom(RandomIt first, RandomIt middle, RandomIt last, Compare comp)`:
  * **Назначение:** Сливает два смежных упорядоченных подмассива `[first, middle)` и `[middle, last)` на месте.
  * **Логика работы:**
    1. Устанавливает два указателя: `left = first` (в левую часть) и `right = middle` (в правую часть).
    2. Если один из диапазонов пуст (`left == right || right == last`), завершает работу.
    3. В цикле пока `left < right && right < last`:
       * Если элемент слева уже меньше или равен элементу справа (`!comp(*right, *left)`), элемент слева стоит на своём месте — сдвигаем `++left`.
       * Иначе элемент справа меньше: сохраняем его во временную переменную через `std::move(*right)`, сдвигаем элементы диапазона `[left, right)` вправо на 1 позицию (`std::move(*(shiftIndex - 1))`), помещаем сохранённый элемент на позицию `*left`, затем сдвигаем указатели `++left` и `++right`.
  * **Сложность:** По дополнительной памяти $O(1)$. По времени слияния $O(N^2)$ перемещений в худшем случае при отсутствии дополнительной памяти.

* `template <typename RandomIt, typename Compare = std::less<>> void in_place_merge_sort(RandomIt first, RandomIt last, Compare comp = Compare())`:
  * **Назначение:** Главная рекурсивная функция сортировки.
  * **Логика работы:**
    1. Вычисляет размер диапазона `count = std::distance(first, last)`.
    2. Базовый случай: если `count <= 1`, массив уже тривиально отсортирован — возврат.
    3. Находит середину: `middle = first + count / 2`.
    4. Рекурсивно сортирует левую половину: `in_place_merge_sort(first, middle, comp)`.
    5. Рекурсивно сортирует правую половину: `in_place_merge_sort(middle, last, comp)`.
    6. Вызывает слияние половин на месте: `in_place_merge_custom(first, middle, last, comp)`.
  * **Сложность:** Время $O(N \log^2 N)$ / $O(N^2 \log N)$, память $O(\log N)$ на стек вызовов, $O(1)$ на данные.

* `template <typename Container, typename Compare = std::less<>> void in_place_merge_sort_container(Container& container, Compare comp = Compare())`:
  * **Назначение:** Удобная обёртка для применения алгоритма к целым контейнерам.

---

## 3. Группа: Пользовательский класс данных (`lab4/include/models/Student.hpp`, `lab4/src/models/Student.cpp`)

Класс `Student` демонстрирует применимость шаблонных алгоритмов и структур данных к сложным типам предметной области с разделением на заголовочный файл `.hpp` и файл реализации `.cpp`.

### Поля класса:
* `int id_`: уникальный целочисленный идентификатор студента (ключ сортировки).
* `std::string name_`: фамилия и имя студента.
* `double gpa_`: средний балл успеваемости.

### Методы класса:
* `Student()`:
  * Конструктор по умолчанию. Инициализирует поля: `id_ = 0`, `name_ = ""`, `gpa_ = 0.0`.
* `Student(int id, std::string name, double gpa)`:
  * Параметризованный конструктор. Принимает строку по значению и перемещает её через `std::move(name)`.
* `int getId() const`:
  * Константный геттер для получения идентификатора.
* `const std::string& getName() const`:
  * Константный геттер для получения имени по константной ссылке (без копирования строки).
* `double getGpa() const`:
  * Константный геттер для получения среднего балла.
* `bool operator<(const Student& other) const`:
  * Оператор строгого порядка. Сравнивает студентов по `id_ < other.id_`. Обеспечивает работу `std::less` и дефолтных сортировок.
* `bool operator==(const Student& other) const`:
  * Оператор проверки на равенство. Возвращает `true`, только если совпадают все три поля: `id_`, `name_` и `gpa_`.
* `bool operator!=(const Student& other) const`:
  * Оператор проверки на неравенство: `!(*this == other)`.

---

## 4. Группа: Структура данных графа по Н. Вирту (`lab4/include/graph/`)

Реализует динамическое списковое представление неориентированного графа, предложенное Никлаусом Виртом в книге «Алгоритмы и структуры данных».

```text
  [VertexNode 1] <---> [VertexNode 2] <---> [VertexNode 3] (список вершин)
        |                     |
     firstEdge             firstEdge
        |                     |
        v                     v
  [EdgeNode -> 2]       [EdgeNode -> 1]
        |
     nextEdge
        v
  [EdgeNode -> 3]
```

---

### 4.1. Файл `EdgeNode.hpp` (Узел ребра)
* **Структура `EdgeNode<T>`**:
  * `VertexNode<T>* target`: указатель на вершину-инцидент (смежную вершину графа).
  * `EdgeNode<T>* nextEdge`: указатель на следующее ребро в списке инцидентности данной вершины.
  * `EdgeNode<T>* prevEdge`: указатель на предыдущее ребро в списке инцидентности данной вершины (двусвязный список инцидентности).
  * `explicit EdgeNode(VertexNode<T>* targetVertex)`:
    * Конструктор. Инициализирует `target = targetVertex`, указатели `nextEdge` и `prevEdge` равными `nullptr`.

---

### 4.2. Файл `VertexNode.hpp` (Узел вершины)
* **Структура `VertexNode<T>`**:
  * `T value`: полезная нагрузка (значение/метка вершины произвольного типа `T`).
  * `EdgeNode<T>* firstEdge`: указатель на голову двусвязного списка инцидентных рёбер.
  * `VertexNode<T>* nextVertex`: указатель на следующую вершину в глобальном списке вершин графа.
  * `VertexNode<T>* prevVertex`: указатель на предыдущую вершину в глобальном списке вершин графа.
  * `explicit VertexNode(const T& val)`:
    * Конструктор копирования значения `val`. Инициализирует все списочные указатели в `nullptr`.
  * `explicit VertexNode(T&& val)`:
    * Перемещающий конструктор для эффективного перемещения значения без лишних аллокаций.

---

## 5. Группа: Семейство итераторов графа (`lab4/include/graph/iterators/`)

Все итераторы оформлены в соответствии с требованиями стандарта C++ (содержат `iterator_category`, `value_type`, `difference_type`, `pointer`, `reference`).

---

### 5.1. Файл `VertexIterator.hpp` (Итераторы вершин)
* **Класс `VertexIterator<T>`**:
  * Категория: `std::bidirectional_iterator_tag` (Двунаправленный итератор).
  * Поля: `VertexNode<T>* node_`, `const Graph<T>* graph_`.
  * `reference operator*() const`: разыменование; возвращает ссылку `node_->value`. Бросает `std::out_of_range`, если итератор равен `end()`.
  * `pointer operator->() const`: доступ по стрелке к полям значения `&(node_->value)`. Бросает `std::out_of_range`, если `node_ == nullptr`.
  * `VertexIterator& operator++()` / `VertexIterator operator++(int)`: префиксный и постфиксный инкремент. Переходит к `node_ = node_->nextVertex`.
  * `VertexIterator& operator--()` / `VertexIterator operator--(int)`: префиксный и постфиксный декремент. Если итератор в `end()`, переходит на `tail_` графа, иначе на `node_->prevVertex`.
  * `bool operator==(const VertexIterator& other) const` / `bool operator!=(...)`: сравнение итераторов по равенству указателя `node_`.

* **Класс `ConstVertexIterator<T>`**:
  * Константная версия итератора вершин (`const T&`, `const T*`).
  * Позволяет конструирование из неконстантного `VertexIterator<T>`.

---

### 5.2. Файл `EdgeIterator.hpp` (Итератор уникальных рёбер графа)
* **Класс `EdgeIterator<T>`**:
  * Категория: `std::bidirectional_iterator_tag`.
  * `value_type`: `std::pair<T, T>`.
  * **Канонический порядок:** В неориентированном графе ребро между $U$ и $V$ хранится в двух списках (узел $U$ видит $V$, а $V$ видит $U$). Чтобы ребро не повторялось дважды при обходе, `EdgeIterator` возвращает пару только при условии `currentVertex_->value <= currentEdge_->target->value`.
  * `std::pair<T, T> operator*() const`: формирует и возвращает пару `(u, v)`.
  * `EdgeIterator& operator++()` / `EdgeIterator operator++(int)`: перемещается к следующему каноническому ребру графа.
  * `EdgeIterator& operator--()` / `EdgeIterator operator--(int)`: перемещается к предыдущему каноническому ребру графа.
  * `bool operator==(const EdgeIterator& other) const`: сравнение по текущей вершине и текущему ребру.

---

### 5.3. Файл `AdjacentVertexIterator.hpp` (Итератор смежных вершин)
* **Класс `AdjacentVertexIterator<T>`**:
  * Категория: `std::bidirectional_iterator_tag`.
  * Назначение: Обход вершин, смежных с заданной вершиной (проход по её списку рёбер).
  * `reference operator*() const`: возвращает метку смежной вершины `edge_->target->value`.
  * `pointer operator->() const`: возвращает указатель `&(edge_->target->value)`.
  * `operator++()` / `operator++(int)`: переход к следующему смежному ребру `edge_ = edge_->nextEdge`.
  * `operator--()` / `operator--(int)`: переход к предыдущему смежному ребру `edge_ = edge_->prevEdge`.

---

### 5.4. Файл `ForwardAdjacentVertexIterator.hpp` (Специфика Варианта 11)
* **Класс `ForwardAdjacentVertexIterator<T>`**:
  * Категория: **`std::forward_iterator_tag`** (**Однонаправленный итератор** — строго по заданию Варианта 11).
  * Назначение: Однонаправленный обход смежных вершин. Операторы `--` намеренно отсутствуют, перемещение возможно только вперед.
  * `reference operator*() const`: разыменование `edge_->target->value`.
  * `pointer operator->() const`: доступ через стрелочку к смежной вершине.
  * `operator++()` / `operator++(int)`: префиксный и постфиксный инкремент (`edge_ = edge_->nextEdge`).
  * `operator==()`, `operator!=()`: проверка равенства по указателю `edge_`.

---

### 5.5. Файл `IncidentEdgeIterator.hpp` (Итератор инцидентных рёбер)
* **Класс `IncidentEdgeIterator<T>`**:
  * Категория: `std::bidirectional_iterator_tag`.
  * `value_type`: `std::pair<T, T>`.
  * Назначение: Обход всех рёбер, инцидентных конкретной вершине $U$.
  * `std::pair<T, T> operator*() const`: возвращает пару `std::pair<T, T>(origin_->value, edge_->target->value)`.
  * `operator++()` / `operator--()`: двунаправленное перемещение по списку инцидентности.

---

## 6. Группа: Контейнер `Graph<T>` (`lab4/include/graph/Graph.hpp`)

Основной класс контейнера, инкапсулирующий всю работу с неориентированным графом.

### 6.1. Внутренние поля
* `VertexNode<T>* head_`: указатель на начальную вершину в списке вершин.
* `VertexNode<T>* tail_`: указатель на последнюю вершину в списке вершин.
* `size_type vertexCount_`: текущее число вершин $|V|$.
* `size_type edgeCount_`: текущее число уникальных ненаправленных рёбер $|E|$.

---

### 6.2. Приватные вспомогательные методы
* `VertexNode<T>* findVertexNode(const T& val) const`:
  * Ищет узел вершины со значением `val`. Сложность $O(V)$. Возвращает указатель или `nullptr`.
* `EdgeNode<T>* findEdgeNode(VertexNode<T>* from, VertexNode<T>* to) const`:
  * Ищет ребро от вершины `from` к вершине `to`. Сложность $O(\deg(from))$.
* `void removeEdgeNodes(VertexNode<T>* u, VertexNode<T>* v)`:
  * Удаляет взаимные направленные звенья из списков инцидентности вершин `u` и `v` с корректной перевязкой указателей `prevEdge` и `nextEdge`.

---

### 6.3. Конструкторы, деструктор и Rule of 5
* `Graph()`:
  * Конструктор по умолчанию. Создаёт пустой граф (`head_ = tail_ = nullptr`, счетчики 0).
* `Graph(std::initializer_list<T> init)`:
  * Конструктор со списком инициализации. Добавляет каждую вершину через `add_vertex(item)`.
* `Graph(const Graph& other)`:
  * Конструктор глубокого копирования (Deep Copy). Полностью дублирует узлы вершин и создаёт аналогичные рёбра.
* `Graph(Graph&& other) noexcept`:
  * Перемещающий конструктор. За $O(1)$ забирает указатели `head_`, `tail_` и счетчики, обнуляя поля источника.
* `Graph& operator=(const Graph& other)`:
  * Оператор присваивания глубоким копированием с защитой от самоприсваивания (`this != &other`).
* `Graph& operator=(Graph&& other) noexcept`:
  * Перемещающий оператор присваивания с очисткой текущих ресурсов.
* `~Graph()`:
  * Деструктор. Полностью освобождает всю динамическую память через `clear()`.

---

### 6.4. Модификаторы графа
* `void clear() noexcept`:
  * Полностью удаляет все рёбра всех вершин, а затем сами вершины. Обнуляет `vertexCount_` и `edgeCount_`.
* `bool add_vertex(const T& val)` / `bool add_vertex(T&& val)`:
  * Добавляет изолированную вершину в конец списка вершин. Если вершина с таким значением уже существует, возвращает `false`, иначе `true`.
* `bool add_edge(const T& u, const T& v)`:
  * Добавляет ненаправленное ребро между $U$ и $V$.
  * Бросает `std::invalid_argument`, если $U = V$ (петли в простом графе запрещены) или если одна из вершин не найдена.
  * Если ребро уже существует, возвращает `false`.
  * При создании добавляет узел ребра в конец списка рёбер $U$ и в конец списка рёбер $V$. Увеличивает `edgeCount_`.
* `bool remove_edge(const T& u, const T& v)`:
  * Удаляет ребро между $U$ и $V$. Возвращает `false`, если вершины или ребро не найдены. Уменьшает `edgeCount_`.
* `bool remove_vertex(const T& val)`:
  * Находит вершину, удаляет все обратные рёбра из смежных вершин, удаляет список инцидентности, исключает вершину из списка вершин и освобождает память.
* `iterator erase(const iterator& pos)`:
  * Удаляет вершину, на которую указывает итератор `pos`. Возвращает итератор на следующую за ней вершину.
* `edge_iterator erase(const edge_iterator& pos)`:
  * Удаляет ребро по итератору `pos`. Возвращает итератор на следующее ребро.
* `void swap(Graph& other) noexcept`:
  * Быстрый обмен полями двух графов за $O(1)$.

---

### 6.5. Запросы состояния и метрик
* `bool empty() const`: возвращает `vertexCount_ == 0`.
* `size_type vertex_count() const`: возвращает $|V|$.
* `size_type edge_count() const`: возвращает $|E|$.
* `bool has_vertex(const T& val) const`: проверяет наличие вершины в графе.
* `bool has_edge(const T& u, const T& v) const`: проверяет наличие ребра между $U$ и $V$.
* `size_type degree(const T& val) const`:
  * Возвращает степень вершины (количество инцидентных ей рёбер). Бросает `std::invalid_argument`, если вершины нет.
* `size_type edge_degree(const T& u, const T& v) const`:
  * **Степень ребра:** Количество рёбер, смежных с данным ребром. Вычисляется по канонической формуле теории графов: $\deg(u) + \deg(v) - 2$. Бросает `std::invalid_argument`, если ребро не найдено.

---

### 6.6. Фабрики итераторов
* По вершинам: `begin()`, `end()`, `cbegin()`, `cend()`, `rbegin()`, `rend()`, `crbegin()`, `crend()`.
* По уникальным рёбрам: `edge_begin()`, `edge_end()`, `edge_cbegin()`, `edge_cend()`, `edge_rbegin()`, `edge_rend()`, `edge_crbegin()`, `edge_crend()`.
* По смежным вершинам: `adjacent_vertex_begin(u)`, `adjacent_vertex_end(u)`.
* По смежным вершинам (Вариант 11, forward): `forward_adjacent_begin(u)`, `forward_adjacent_end(u)`.
* По инцидентным рёбрам: `incident_edge_begin(u)`, `incident_edge_end(u)`.

---

### 6.7. Операторы сравнения и форматированного вывода
* `bool operator==(const Graph& other) const`:
  * Проверяет графы на полное совпадение: одинаковое число вершин и рёбер, наличие всех вершин и всех рёбер.
* `bool operator!=(const Graph& other) const`: `!(*this == other)`.
* `bool operator<(const Graph& other) const`:
  * Лексикографическое сравнение графов (сначала по числу вершин, затем по числу рёбер, затем по элементам).
* `operator>`, `operator<=`, `operator>=`: реализованы через `operator<`.
* `friend std::ostream& operator<<(std::ostream& os, const Graph<T>& g)`:
  * Потоковый вывод графа в формате:
    `Graph(vertices: N, edges: M) [ v1 v2 ... ] { (u, v) ... }`
  * В соответствии с чеклистом преподавателя вывод списков вершин и рёбер реализован через **алгоритм `std::for_each`**.

---

## 7. Группа: Консольное приложение (`lab4/cli/main.cpp`)

Интерактивное консольное меню, позволяющее наглядно проверить всю функциональность:

1. **`1. Add vertex`**: Добавление новой вершины (пользователь вводит число).
2. **`2. Remove vertex`**: Удаление вершины и всех её инцидентных рёбер.
3. **`3. Add edge`**: Добавление неориентированного ребра между двумя вершинами.
4. **`4. Remove edge`**: Удаление ребра по паре вершин.
5. **`5. Print all vertices (VertexIterator)`**: Демонстрация прямого обхода всех вершин графа через `VertexIterator`.
6. **`6. Print all edges (EdgeIterator)`**: Демонстрация обхода уникальных рёбер через канонический `EdgeIterator`.
7. **`7. Print adjacent vertices (Variant 11 Forward Iterator)`**: Демонстрация специального однонаправленного итератора `ForwardAdjacentVertexIterator` для выбранной вершины.
8. **`8. Print vertex degree and edge degree`**: Ввод вершины для вывода её степени и пары вершин для вывода степени ребра $\deg(u) + \deg(v) - 2$.
9. **`9. Demonstrate Bogosort`**: Генерация случайного небольшого вектора и его сортировка алгоритмом Bogosort.
10. **`10. Demonstrate In-place merge sort`**: Сортировка массива алгоритмом слияния на месте без дополнительной памяти.
11. **`11. Demonstrate custom class Student sorting`**: Сортировка коллекции объектов `Student` сначала по ID (по умолчанию), а затем по среднему баллу GPA с помощью лямбда-компаратора.
12. **`12. Print graph (operator<<)`**: Вывод графа в поток вывода через `operator<<` и `std::for_each`.
13. **`0. Exit`**: Завершение работы программы.

---

## 8. Группа: Модульные тесты (`lab4/tests/`)

Проект протестирован с помощью фреймворка `UnitTest++` с суммарным покрытием строк **98.0%**:

* **`test_sorts.cpp`**:
  * `IsSortedCustom_VariousScenarios`: проверка пустого вектора, одного элемента, отсортированного, неотсортированного, обратного порядка, C-массивов.
  * `BogoSort_VectorAndArray`: проверка сортировки векторов, статических массивов и обратного компаратора.
  * `InPlaceMergeSort_BasicTypes`: проверка пустого массива, одного элемента, уже отсортированного, инвертированного и массива с повторяющимися элементами.
  * `InPlaceMergeCustom_DirectInvocationsAndEdgeCases`: прямая проверка функции `in_place_merge_custom` и граничных условий (`left == right`, `right == last`).
  * `InPlaceMergeSort_RawArray`: сортировка сырого C-массива `int[8]`.
  * `InPlaceMergeSort_CustomComparator`: сортировка строк с компаратором `std::greater<std::string>`.
  * `InPlaceMergeSort_CustomUserClassStudent`: проверка методов класса `Student`, сортировка по умолчанию (по ID) и сортировка с пользовательской лямбдой (по GPA).

* **`test_graph.cpp`**:
  * `DefaultConstructor_IsEmpty`: проверка инвариантов пустого графа.
  * `InitializerList_AddsVertices`: создание графа со списком инициализации.
  * `AddVertex_DuplicateReturnsFalse`: проверка rvalue/lvalue добавлений и дубликатов.
  * `AddEdge_BasicAndInvalid`: проверка добавления рёбер, петель и исключений.
  * `RemoveEdge_DecrementsCount`: удаление начального, среднего и конечного ребра.
  * `RemoveVertex_RemovesIncidentEdges`: удаление изолированной вершины, промежуточной вершины в цепи, начальной и конечной вершины.
  * `DegreeAndEdgeDegree`: проверка степеней вершин и рёбер, обработка исключений при несуществующих элементах.
  * `VertexIterator_BidirectionalAndConst`: двунаправленный обход (`++`, `--`, постфиксные версии, `rbegin`, `rend`), константные итераторы (`cbegin`, `cend`).
  * `EdgeIterator_BidirectionalAndReverse`: обход уникальных ненаправленных рёбер, реверсивный обход.
  * `IncidentEdgeAndAdjacentVertexIterators`: обход инцидентных рёбер и смежных вершин.
  * `Variant11_ForwardAdjacentVertexIterator`: детальное тестирование однонаправленного итератора смежных вершин по заданию 11 варианта.
  * `EraseByIterators`: удаление элементов по итераторам вершин и рёбер.
  * `DeepCopyAndMoveSemantics`: глубокое копирование, перемещающие конструкторы, операторы присваивания и `swap`.
  * `ComparisonOperators`: проверка всех операторов `<`, `>`, `<=`, `>=`, `==`, `!=`.
  * `StreamOutputOperator`: проверка форматированного вывода через `operator<<` и `std::for_each`.
  * `IteratorDereferenceExceptions`: проверка генерации исключений `std::out_of_range` при попытке разыменования итераторов `end()`.
