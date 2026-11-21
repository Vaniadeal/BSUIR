# Руководство по командам сборки, тестирования и запуска (Лабораторная работа №4)

Данный файл содержит исчерпывающий перечень консольных команд для сборки, тестирования, проверки покрытия кода, статического анализа и запуска приложения на Windows (PowerShell / MinGW UCRT64) и Linux / macOS (Bash).

---

## 1. Сборка проекта

### Вариант А: Сборка в режиме Debug с включенным профилированием покрытия (Coverage)

Используется для запуска тестов и расчета покрытия кода (`gcov` / `gcovr`):

#### PowerShell (Windows):
```powershell
# 1. Создать/обновить конфигурацию CMake
cmake -S lab4 -B lab4/build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON '-DCMAKE_POLICY_VERSION_MINIMUM=3.5'

# 2. Скомпилировать все цели (библиотеку, приложение и тесты)
cmake --build lab4/build
```

#### Bash (Linux / macOS):
```bash
# 1. Конфигурация CMake
cmake -S lab4 -B lab4/build \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_COVERAGE=ON \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# 2. Компиляция
cmake --build lab4/build
```

---

### Вариант Б: Чистая пересборка с нуля (Rebuild)

Если нужно полностью удалить старый кэш сборки и скомпилировать проект начисто:

#### PowerShell (Windows):
```powershell
Remove-Item -Recurse -Force lab4/build
cmake -S lab4 -B lab4/build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON '-DCMAKE_POLICY_VERSION_MINIMUM=3.5'
cmake --build lab4/build
```

#### Bash (Linux / macOS):
```bash
rm -rf lab4/build
cmake -S lab4 -B lab4/build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build lab4/build
```

---

### Вариант В: Сборка релиза (Release) с оптимизацией

Используется для релизной версии без отладочных символов:

#### PowerShell (Windows):
```powershell
cmake -S lab4 -B lab4/build-release -G Ninja -DCMAKE_BUILD_TYPE=Release '-DCMAKE_POLICY_VERSION_MINIMUM=3.5'
cmake --build lab4/build-release
```

---

## 2. Запуск unit-тестов (UnitTest++)

В проекте настроены два набора тестов: `sorts_tests` (тесты сортировок и класса `Student`) и `graph_tests` (тесты графа и всех 5 видов итераторов).

### Способ 1: Через утилиту `ctest`
```powershell
ctest --test-dir lab4/build --output-on-failure
```

### Способ 2: Запуск исполняемых файлов тестов напрямую

#### Windows:
```powershell
# Тесты сортировок:
./lab4/build/sorts_tests.exe

# Тесты графа и итераторов:
./lab4/build/graph_tests.exe
```

#### Linux:
```bash
./lab4/build/sorts_tests
./lab4/build/graph_tests
```

---

## 3. Проверка покрытия кода (Code Coverage >= 90%)

> **Важно:** Перед расчетом покрытия необходимо хотя бы один раз запустить тесты (`sorts_tests` и `graph_tests`), чтобы сгенерировались файлы счетчиков `.gcda`.

### Консольный отчет с проверкой порога 90% (как в GitHub Actions CI):

#### Windows (PowerShell):
```powershell
Set-Location lab4
gcovr --root . --filter "include/.*" --filter "src/.*" --print-summary --fail-under-line 90
Set-Location ..
```

#### Linux (Bash):
```bash
cd lab4
gcovr --root . \
      --filter "include/.*" \
      --filter "src/.*" \
      --print-summary \
      --fail-under-line 90
cd ..
```

### Генерация интерактивного HTML-отчета с подсветкой каждой строки:
```powershell
Set-Location lab4
gcovr --root . --filter "include/.*" --filter "src/.*" --html-details -o coverage_report.html
Set-Location ..
# Открыть отчет в браузере: lab4/coverage_report.html
```

---

## 4. Статический анализ кода (`cppcheck`)

Проверка исходного кода на возможные утечки памяти, ошибки производительности, переносимости и стиля (в точности как в GitHub Actions):

#### PowerShell (Windows через MinGW UCRT64):
```powershell
cppcheck --enable=warning,performance,portability `
         --error-exitcode=1 `
         --inline-suppr `
         --suppress=missingIncludeSystem `
         --suppress=normalCheckLevelMaxBranches `
         -I lab4/include `
         lab4/src lab4/cli lab4/include
```

#### Linux / macOS:
```bash
cppcheck --enable=warning,performance,portability \
         --error-exitcode=1 \
         --inline-suppr \
         --suppress=missingIncludeSystem \
         --suppress=normalCheckLevelMaxBranches \
         -I lab4/include \
         lab4/src lab4/cli lab4/include
```

---

## 5. Запуск интерактивного консольного приложения (`lab4_app`)

Интерактивное консольное меню с 13 пунктами для проверки работы графа, сортировок и класса `Student`.

#### Windows:
```powershell
./lab4/build/lab4_app.exe
```

#### Linux:
```bash
./lab4/build/lab4_app
```

---

## 6. Быстрый скрипт «Все в одном» (Полная локальная валидация)

Выполните эту команду, чтобы за один шаг очистить кэш, собрать проект, прогнать все тесты, рассчитать покрытие и запустить cppcheck:

```powershell
cmake -S lab4 -B lab4/build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON '-DCMAKE_POLICY_VERSION_MINIMUM=3.5'; `
cmake --build lab4/build; `
& ./lab4/build/sorts_tests.exe; `
& ./lab4/build/graph_tests.exe; `
Set-Location lab4; `
gcovr --root . --filter "include/.*" --filter "src/.*" --print-summary --fail-under-line 90; `
Set-Location ..; `
cppcheck --enable=warning,performance,portability --error-exitcode=1 --inline-suppr --suppress=missingIncludeSystem --suppress=normalCheckLevelMaxBranches -I lab4/include lab4/src lab4/cli lab4/include
```
