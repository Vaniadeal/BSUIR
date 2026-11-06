from itertools import combinations

# Вариант 14: Выбор меры поддержки многодетных семей
# A — Выдать материальную помощь в размере 50 базовых величин
# B — Отправить детей на оплачиваемые курсы для получения специальности
# C — Выделить путевки в санатории и дома отдыха

# Индивидуальные ранжирования экспертов
rankings = [
    ['A', 'B', 'C'],  # Э1 — Представитель соцзащиты
    ['A', 'C', 'B'],  # Э2 — Представитель многодетных семей
    ['B', 'A', 'C'],  # Э3 — Экономист администрации
]

# Список уникальных альтернатив
alternatives = sorted({alt for ranking in rankings for alt in ranking})

# Инициализация матрицы побед
wins = {a: {b: 0 for b in alternatives if b != a} for a in alternatives}

# Попарные сравнения
for ranking in rankings:
    for a, b in combinations(alternatives, 2):
        if ranking.index(a) < ranking.index(b):
            wins[a][b] += 1
        else:
            wins[b][a] += 1

print("=" * 45)
print("РЕЗУЛЬТАТЫ ПАРНЫХ СРАВНЕНИЙ:")
print("=" * 45)
for a, b in combinations(alternatives, 2):
    winner = a if wins[a][b] > wins[b][a] else (b if wins[b][a] > wins[a][b] else "Ничья")
    print(f"Альтернатива {a} против {b}: {wins[a][b]} : {wins[b][a]}  -->  Победитель: {winner}")

# Поиск победителя по Кондорсе
condorcet_winner = None
for a in alternatives:
    if all(wins[a][b] > wins[b][a] for b in alternatives if b != a):
        condorcet_winner = a
        break

print("=" * 45)
if condorcet_winner:
    print(f"Победитель по Кондорсе: Альтернатива {condorcet_winner}")
    print("Парадокс Кондорсе отсутствует.")
else:
    print("Победителя по Кондорсе нет: возник парадокс Кондорсе.")
print("=" * 45)
