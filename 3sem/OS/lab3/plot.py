import pandas as pd
import matplotlib.pyplot as plt

frag = pd.read_csv("frag.csv")
fig, ax = plt.subplots(figsize=(8, 5))
ax.plot(frag.block_size, frag.wasted_bytes, marker='o', color='tab:blue')
ax.set_xlabel("Размер запрошенного блока, байт")
ax.set_ylabel("Потеряно памяти, байт")
ax.set_title("Внутренняя фрагментация vs размер блока")
ax.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig("frag.png", dpi=120)
plt.close()

faults = pd.read_csv("faults.csv")
fig, ax = plt.subplots(figsize=(8, 5))
ax.plot(faults.frames, faults.page_faults, marker='o', color='tab:red')
ax.set_xscale('log', base=2)
ax.set_xlabel("Число физических фреймов")
ax.set_ylabel("Число page fault")
ax.set_title("Page faults vs размер физической памяти")
ax.grid(True, which='both', alpha=0.3)
plt.tight_layout()
plt.savefig("faults.png", dpi=120)
plt.close()

loc = pd.read_csv("locality.csv")
fig, ax = plt.subplots(figsize=(8, 5))
for pat, grp in loc.groupby("pattern"):
    label = "последовательный" if pat == "seq" else "случайный"
    ax.plot(grp.frames, grp.page_faults, marker='o', label=label)
ax.set_xscale('log', base=2)
ax.set_xlabel("Число физических фреймов")
ax.set_ylabel("Число page fault")
ax.set_title("Последовательный vs случайный доступ")
ax.legend()
ax.grid(True, which='both', alpha=0.3)
plt.tight_layout()
plt.savefig("locality.png", dpi=120)
plt.close()

print("saved: frag.png, faults.png, locality.png")