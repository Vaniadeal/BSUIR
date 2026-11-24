import os
os.environ.setdefault('MPLCONFIGDIR', '/tmp')
import pandas as pd
import matplotlib.pyplot as plt

if os.path.exists("frag.csv"):
    frag = pd.read_csv("frag.csv")
    frag["wasted_kib"] = frag["wasted_bytes"] / 1024.0
    fig, ax = plt.subplots(figsize=(10, 5.5), dpi=150)
    ax.plot(frag["block_size"], frag["wasted_kib"], color='#1f77b4', linewidth=2.2, marker='o', markersize=4)
    ax.set_title("Внутренняя фрагментация в зависимости от размера блока", fontsize=13, pad=12)
    ax.set_xlabel("Размер запроса, байт", fontsize=11)
    ax.set_ylabel("Потери памяти, КиБ", fontsize=11)
    ax.grid(True, linestyle='-', alpha=0.3)
    plt.tight_layout()
    plt.savefig("frag.png")
    plt.close()
    print("saved: frag.png")

if os.path.exists("accum_frag.csv"):
    accum = pd.read_csv("accum_frag.csv")
    accum["wasted_mib"] = accum["wasted_bytes"] / (1024.0 * 1024.0)
    fig, ax = plt.subplots(figsize=(10, 5.5), dpi=150)
    ax.plot(accum["allocations"], accum["wasted_mib"], color='#1f77b4', linewidth=2.2)
    ax.set_title("Накопление внутренней фрагментации при выделении блоков", fontsize=13, pad=12)
    ax.set_xlabel("Количество выделенных блоков", fontsize=11)
    ax.set_ylabel("Суммарные потери памяти, МиБ", fontsize=11)
    ax.grid(True, linestyle='-', alpha=0.3)
    plt.tight_layout()
    plt.savefig("accum_frag.png")
    plt.close()
    print("saved: accum_frag.png")