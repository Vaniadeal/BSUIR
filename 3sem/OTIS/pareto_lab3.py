import numpy as np
import matplotlib.pyplot as plt

# Данные: [цена (BYN, минимизация), автономность (мин, максимизация)]
data = np.array([
    [350,  60],   # M1 — Philips Series 5000 S5585
    [400,  50],   # M2 — Braun Series 5 50-B1200s
    [800,  45],   # M3 — Panasonic ES-LS9A
    [150,  90],   # M4 — Xiaomi Mi Electric Shaver S500
    [120,  40],   # M5 — Remington F5-5800
    [200, 120],   # M6 — Wahl Lithium Ion Plus 9864
], dtype=float)

names = [
    "Philips S5585",
    "Braun 50-B1200s",
    "Panasonic ES-LS9A",
    "Xiaomi S500",
    "Remington F5-5800",
    "Wahl 9864",
]

price = data[:, 0]   # цена
autonomy = data[:, 1]  # автономность

n = len(data)
pareto = []           # True, если альтернатива Парето-оптимальна
dominated_by = []     # индекс доминирующей альтернативы

# Поиск доминируемых и Парето-оптимальных альтернатив
for i in range(n):
    is_dominated = False
    dominator = -1
    for k in range(n):
        if k == i:
            continue
        cheaper_or_equal = price[k] <= price[i]
        longer_or_equal = autonomy[k] >= autonomy[i]
        strictly_better = (price[k] < price[i]) or (autonomy[k] > autonomy[i])
        if cheaper_or_equal and longer_or_equal and strictly_better:
            is_dominated = True
            dominator = k
            break
    pareto.append(not is_dominated)
    dominated_by.append(dominator)

print("Множество Парето:")
for i in range(n):
    status = "Парето-оптимальна" if pareto[i] else f"доминируется ({names[dominated_by[i]]})"
    print(f"  {names[i]:>20}: цена = {price[i]:.0f} BYN, автономность = {autonomy[i]:.0f} мин  ->  {status}")

# Точки фронта Парето, отсортированные по цене
front_idx = [i for i in range(n) if pareto[i]]
front_idx.sort(key=lambda i: price[i])
front_x = [price[i] for i in front_idx]
front_y = [autonomy[i] for i in front_idx]

# График
fig, ax = plt.subplots(figsize=(9, 5.5), dpi=200)

colors = ["#1f77b4" if p else "#d62728" for p in pareto]
ax.scatter(price, autonomy, s=90, c=colors, zorder=3)

for i in range(n):
    ax.annotate(names[i], (price[i], autonomy[i]),
                textcoords="offset points", xytext=(8, 6), fontsize=9)

ax.plot(front_x, front_y, "r-", lw=2, label="Фронт Парето", zorder=2)

# Стрелки от доминирующей альтернативы к доминируемой
for i in range(n):
    if not pareto[i]:
        k = dominated_by[i]
        ax.annotate("", xy=(price[i], autonomy[i]), xytext=(price[k], autonomy[k]),
                    arrowprops=dict(arrowstyle="->", color="gray", ls="--", lw=1.2))

ax.set_xlabel("Цена, BYN (q1, минимизация)", fontsize=12)
ax.set_ylabel("Автономность, мин (q3, максимизация)", fontsize=12)
ax.set_title("Множество Парето: электробритвы\n(критерии: цена и автономность)", fontsize=13)
ax.grid(True, alpha=0.3)
ax.legend(fontsize=11)
ax.set_xlim(80, 850)
ax.set_ylim(30, 130)

plt.tight_layout()
plt.savefig("pareto.png", bbox_inches="tight")
plt.show()