# Консольные команды для Лабораторных работ №3 и №4 (PPOIS)

В данном документе собраны все консольные команды для локальной пересборки, запуска unit-тестов, расчета покрытия кода (порог `≥ 90%`) и отправки изменений на GitHub для создания и прохождения Pull Request.

> [!NOTE]
> Все команды выполняются из корня репозитория: `D:\clonerepositories\PPOIS`.
> Слияние (Merge) выполняется вами вручную на сайте GitHub после успешного прохождения автоматических проверок CI/CD.

---

## 1. Лабораторная работа №3: Онлайн-платформа обучения

* **Ветка:** `feature/lab3-online-learning`
* **Целевая ветка для PR:** `main`

### Шаг 1. Переключение на ветку лабы
```powershell
git checkout feature/lab3-online-learning
```

### Шаг 2. Полный чистый перебилд с флагом покрытия
```powershell
# Удаление старой папки сборки (если есть)
Remove-Item -Path "lab3/build" -Recurse -Force -ErrorAction SilentlyContinue

# Конфигурация CMake в Debug-режиме с генератором покрытия
cmake -S lab3 -B lab3/build -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DENABLE_COVERAGE=ON"
```

### Шаг 3. Компиляция проекта
```powershell
cmake --build lab3/build
```

### Шаг 4. Запуск тестов
```powershell
# Вариант А: через CTest с выводом ошибок
ctest --test-dir lab3/build --output-on-failure

# Вариант Б: прямой запуск исполняемого файла тестов
./lab3/build/learning_platform_tests.exe
```

### Шаг 5. Расчет покрытия кода (порог ≥ 90%)
```powershell
cd lab3
gcovr --root . --filter "src/.*" --print-summary --fail-under-line 90
cd ..
```

### Шаг 6. Фиксация изменений и отправка на GitHub для PR
```powershell
git add lab3/ .github/workflows/ci.yml
git commit -m "feat(lab3): verify tests, coverage and prepare PR"
git push origin feature/lab3-online-learning
```

### Шаг 7. Слияние на GitHub
1. Перейдите в репозиторий: [Vaniadeal/PPOIS/pulls](https://github.com/Vaniadeal/PPOIS/pulls).
2. Откройте (или создайте) Pull Request: `base: main` ← `compare: feature/lab3-online-learning`.
3. Дождитесь завершения проверки **`Lab 3: Tests & Coverage (>= 90%)`** (зеленая галочка).
4. Нажмите кнопку **Merge pull request** → **Confirm merge**.

---

## 2. Лабораторная работа №4: Обобщенные структуры и Граф Вирта

* **Ветка:** `feature/lab4`
* **Целевая ветка для PR:** `main`

### Шаг 1. Переключение на ветку лабы
```powershell
git checkout feature/lab4
```

### Шаг 2. Полный чистый перебилд с флагом покрытия
```powershell
# Удаление старой папки сборки (если есть)
Remove-Item -Path "lab4/build" -Recurse -Force -ErrorAction SilentlyContinue

# Конфигурация CMake в Debug-режиме с генератором покрытия
cmake -S lab4 -B lab4/build -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DENABLE_COVERAGE=ON"
```

### Шаг 3. Компиляция проекта
```powershell
cmake --build lab4/build
```

### Шаг 4. Запуск тестов (сортировки + граф)
```powershell
# Вариант А: через CTest с выводом ошибок
ctest --test-dir lab4/build --output-on-failure

# Вариант Б: прямой запуск бинарников тестов
./lab4/build/sorts_tests.exe
./lab4/build/graph_tests.exe
```

### Шаг 5. Расчет покрытия кода (порог ≥ 90%)
```powershell
cd lab4
gcovr --root . --filter "include/.*" --filter "src/.*" --print-summary --fail-under-line 90
cd ..
```

### Шаг 6. Фиксация изменений и отправка на GitHub для PR
```powershell
git add lab4/ .github/workflows/ci.yml
git commit -m "feat(lab4): verify tests, coverage and prepare PR"
git push origin feature/lab4
```

### Шаг 7. Слияние на GitHub
1. Перейдите в репозиторий: [Vaniadeal/PPOIS/pulls](https://github.com/Vaniadeal/PPOIS/pulls).
2. Откройте Pull Request: `base: main` ← `compare: feature/lab4`.
3. Дождитесь завершения проверки **`Lab 4: Tests & Coverage (>= 90%)`** (зеленая галочка).
4. Нажмите кнопку **Merge pull request** → **Confirm merge**.

---

## 3. Экспресс-скрипты (копирование одной строкой)

### Для Лабы 3:
```powershell
git checkout feature/lab3-online-learning; cmake -S lab3 -B lab3/build -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DENABLE_COVERAGE=ON"; cmake --build lab3/build; ctest --test-dir lab3/build --output-on-failure; cd lab3; gcovr --root . --filter "src/.*" --print-summary --fail-under-line 90; cd ..; git add lab3/; git commit -m "feat(lab3): pass tests & coverage"; git push origin feature/lab3-online-learning
```

### Для Лабы 4:
```powershell
git checkout feature/lab4; cmake -S lab4 -B lab4/build -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DENABLE_COVERAGE=ON"; cmake --build lab4/build; ctest --test-dir lab4/build --output-on-failure; cd lab4; gcovr --root . --filter "include/.*" --filter "src/.*" --print-summary --fail-under-line 90; cd ..; git add lab4/; git commit -m "feat(lab4): pass tests & coverage"; git push origin feature/lab4
```
