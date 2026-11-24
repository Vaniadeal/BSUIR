#!/usr/bin/env python3
import os
import sys
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

def run_benchmarks():
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(base_dir)

    print("=== Running Filesystem Benchmark Binary ===")
    bench_bin = os.path.join(base_dir, "build", "benchmark_app")
    if not os.path.exists(bench_bin):
        print(f"Error: {bench_bin} not found. Please build the project first.")
        sys.exit(1)

    result = subprocess.run([bench_bin], check=True)
    if result.returncode != 0:
        print("Benchmark failed.")
        sys.exit(result.returncode)

    charts_dir = os.path.join(base_dir, "benchmarks", "charts")
    data_dir = os.path.join(base_dir, "benchmarks", "data")
    os.makedirs(charts_dir, exist_ok=True)

    # Style configuration
    plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
    plt.rcParams['font.sans-serif'] = 'DejaVu Sans'
    plt.rcParams['font.size'] = 11

    # --- Chart 1: Random Access Overhead ---
    csv1 = os.path.join(data_dir, "random_access.csv")
    if os.path.exists(csv1):
        df1 = pd.read_csv(csv1)
        fig, ax1 = plt.subplots(figsize=(9, 5.5), dpi=150)

        color1 = '#1f77b4'
        color2 = '#d62728'
        color3 = '#2ca02c'

        ax1.plot(df1['blocks'], df1['random_seek_time_us'], label='Произвольное чтение (Seek к последнему блоку)', color=color2, linewidth=2.2, marker='s', markersize=4)
        ax1.plot(df1['blocks'], df1['seq_read_time_us'], label='Последовательное чтение всего файла', color=color1, linewidth=2.0, linestyle='--', marker='o', markersize=4)

        ax1.set_xlabel('Размер файла (в блоках по 512 байт)', fontweight='bold')
        ax1.set_ylabel('Время выполнения операции (мкс)', fontweight='bold')
        ax1.set_title('Тест 1: Задержка произвольного доступа vs Размер файла\n(Демонстрация O(N) сложности обхода связного списка)', fontsize=12, fontweight='bold', pad=12)

        ax2 = ax1.twinx()
        ax2.plot(df1['blocks'], df1['traversed_hops'], label='Число переходов по указателям (hops)', color=color3, linewidth=1.5, linestyle=':')
        ax2.set_ylabel('Число переходов по блокам (hops)', color=color3, fontweight='bold')
        ax2.tick_params(axis='y', labelcolor=color3)

        lines1, labels1 = ax1.get_legend_handles_labels()
        lines2, labels2 = ax2.get_legend_handles_labels()
        ax1.legend(lines1 + lines2, labels1 + labels2, loc='upper left', frameon=True)

        plt.tight_layout()
        out1 = os.path.join(charts_dir, "random_access_overhead.png")
        plt.savefig(out1)
        plt.close()
        print(f"Generated chart: {out1}")

    # --- Chart 2: Churn and Fragmentation ---
    csv2 = os.path.join(data_dir, "churn_fragmentation.csv")
    if os.path.exists(csv2):
        df2 = pd.read_csv(csv2)
        fig, ax1 = plt.subplots(figsize=(9, 5.5), dpi=150)

        color1 = '#ff7f0e'
        color2 = '#9467bd'

        ax1.plot(df2['step'], df2['avg_hop_distance'], label='Средняя дистанция перехода между блоками (блоки)', color=color1, linewidth=2.2, marker='^', markersize=4)
        ax1.set_xlabel('Итерация нагрузки (создание / удаление файлов)', fontweight='bold')
        ax1.set_ylabel('Средняя дистанция перехода (блоков)', color=color1, fontweight='bold')
        ax1.tick_params(axis='y', labelcolor=color1)
        ax1.set_title('Тест 2: Рост внешней фрагментации при создании и удалении файлов\n(Деградация непрерывности цепочек связного списка)', fontsize=12, fontweight='bold', pad=12)

        ax2 = ax1.twinx()
        ax2.plot(df2['step'], df2['fragmentation_ratio'], label='Доля фрагментированных переходов (%)', color=color2, linewidth=2.0, linestyle='--', marker='o', markersize=4)
        ax2.set_ylabel('Доля фрагментированных цепочек (%)', color=color2, fontweight='bold')
        ax2.tick_params(axis='y', labelcolor=color2)
        ax2.set_ylim(0, 105)

        lines1, labels1 = ax1.get_legend_handles_labels()
        lines2, labels2 = ax2.get_legend_handles_labels()
        ax1.legend(lines1 + lines2, labels1 + labels2, loc='lower right', frameon=True)

        plt.tight_layout()
        out2 = os.path.join(charts_dir, "churn_fragmentation.png")
        plt.savefig(out2)
        plt.close()
        print(f"Generated chart: {out2}")

    # --- Chart 3: Disk Fill vs Allocation Latency ---
    csv3 = os.path.join(data_dir, "disk_fill.csv")
    if os.path.exists(csv3):
        df3 = pd.read_csv(csv3)
        fig, ax = plt.subplots(figsize=(9, 5.5), dpi=150)

        ax.plot(df3['fill_percentage'], df3['alloc_latency_us'], color='#8c564b', linewidth=2.5, marker='D', markersize=6)
        ax.fill_between(df3['fill_percentage'], df3['alloc_latency_us'], color='#8c564b', alpha=0.15)
        ax.set_xlabel('Степень заполнения диска (%)', fontweight='bold')
        ax.set_ylabel('Время аллокации файла из 5 блоков (мкс)', fontweight='bold')
        ax.set_title('Тест 3: Время выделения блоков в зависимости от заполнения диска\n(Поиск свободных блоков по битовой карте)', fontsize=12, fontweight='bold', pad=12)
        ax.xaxis.set_major_locator(ticker.MultipleLocator(10))

        plt.tight_layout()
        out3 = os.path.join(charts_dir, "allocation_vs_disk_fill.png")
        plt.savefig(out3)
        plt.close()
        print(f"Generated chart: {out3}")

    # --- Chart 4: Storage Efficiency vs Block Size ---
    csv4 = os.path.join(data_dir, "block_efficiency.csv")
    if os.path.exists(csv4):
        df4 = pd.read_csv(csv4)
        fig, ax = plt.subplots(figsize=(9, 5.5), dpi=150)

        bars = ax.bar([str(x) for x in df4['block_size']], df4['efficiency_pct'], color='#17becf', edgecolor='black', width=0.5)
        ax.set_xlabel('Размер блока диска (байт)', fontweight='bold')
        ax.set_ylabel('Полезная емкость блока (%)', fontweight='bold')
        ax.set_ylim(85, 102)
        ax.set_title('Тест 4: Эффективность хранения (Payload Efficiency) vs Размер блока\n(Оверхед хранения указателя next_block и заголовка 8 байт)', fontsize=12, fontweight='bold', pad=12)

        for bar in bars:
            yval = bar.get_height()
            ax.text(bar.get_x() + bar.get_width() / 2.0, yval + 0.4, f"{yval:.2f}%", ha='center', va='bottom', fontweight='bold')

        plt.tight_layout()
        out4 = os.path.join(charts_dir, "block_efficiency.png")
        plt.savefig(out4)
        plt.close()
        print(f"Generated chart: {out4}")

    print("=== All benchmark charts successfully generated ===")

if __name__ == '__main__':
    run_benchmarks()
