#!/usr/bin/env python3
import os
import glob
import re

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

BASE_DIR = "results"


def load_qos(scenario: str):
    path = os.path.join(BASE_DIR, f"scenario_{scenario}")
    df = {}

    # vazao: s{scenario}_QoS_vazao.txt dentro da pasta do cenário
    fname_thr = os.path.join(path, f"s{scenario}_QoS_vazao.txt")
    if os.path.exists(fname_thr):
        df["vazao"] = pd.read_csv(fname_thr, sep=r"\s+", header=None, engine="python")

    # jitter, perda, delay: formato "Flow X: valor s"
    for metric in ["jitter", "perda", "delay"]:
        fname = os.path.join(path, f"s{scenario}_QoS_{metric}.txt")
        if os.path.exists(fname):
            vals = []
            with open(fname) as f:
                for line in f:
                    m = re.search(r":\s*([0-9.eE+-]+)", line)
                    if m:
                        vals.append(float(m.group(1)))
            df[metric] = np.array(vals)

    return df


def plot_qos():
    scenarios = ["0", "1"]
    thr, jitter, loss, delay = [], [], [], []

    for s in scenarios:
        qos = load_qos(s)

        # vazão média (segunda coluna numérica)
        if "vazao" in qos:
            thr.append(float(qos["vazao"][1].mean()))
        else:
            thr.append(0.0)

        # jitter médio
        if "jitter" in qos and qos["jitter"].size > 0:
            jitter.append(float(qos["jitter"].mean()))
        else:
            jitter.append(0.0)

        # perda média
        if "perda" in qos and qos["perda"].size > 0:
            loss.append(float(qos["perda"].mean()))
        else:
            loss.append(0.0)

        # delay médio
        if "delay" in qos and qos["delay"].size > 0:
            delay.append(float(qos["delay"].mean()))
        else:
            delay.append(0.0)

    # Throughput
    plt.figure(figsize=(8, 5), dpi=150)
    bars = plt.bar(scenarios, thr)
    plt.xticks(fontsize=11)
    plt.yticks(fontsize=11)
    plt.xlabel("Cenário", fontsize=12)
    plt.ylabel("Vazão média (Mbps)", fontsize=12)
    for bar in bars:
        x = bar.get_x() + bar.get_width() / 2
        y = bar.get_height()
        plt.text(x, y, f"{y:.2f}", ha="center", va="bottom", fontsize=10)
    plt.title("Vazão média por cenário", fontsize=14)
    plt.grid(axis="y", linestyle="--", alpha=0.4)
    plt.tight_layout()
    plt.savefig(os.path.join(BASE_DIR, "qos_throughput.png"), dpi=300)
    plt.close()

    # Jitter
    plt.figure(figsize=(8, 5), dpi=150)
    bars = plt.bar(scenarios, jitter)
    plt.xticks(fontsize=11)
    plt.yticks(fontsize=11)
    plt.xlabel("Cenário", fontsize=12)
    plt.ylabel("Jitter médio (s)", fontsize=12)
    for bar in bars:
        x = bar.get_x() + bar.get_width() / 2
        y = bar.get_height()
        plt.text(x, y, f"{y:.6f}", ha="center", va="bottom", fontsize=10)
    plt.title("Jitter médio por cenário", fontsize=14)
    plt.grid(axis="y", linestyle="--", alpha=0.4)
    plt.tight_layout()
    plt.savefig(os.path.join(BASE_DIR, "qos_jitter.png"), dpi=300)
    plt.close()

    # Perda
    plt.figure(figsize=(8, 5), dpi=150)
    bars = plt.bar(scenarios, loss)
    plt.xticks(fontsize=11)
    plt.yticks(fontsize=11)
    plt.xlabel("Cenário", fontsize=12)
    plt.ylabel("Perda (%)", fontsize=12)
    for bar in bars:
        x = bar.get_x() + bar.get_width() / 2
        y = bar.get_height()
        plt.text(x, y, f"{y:.2f}", ha="center", va="bottom", fontsize=10)
    plt.title("Perda de pacotes por cenário", fontsize=14)
    plt.grid(axis="y", linestyle="--", alpha=0.4)
    plt.tight_layout()
    plt.savefig(os.path.join(BASE_DIR, "qos_loss.png"), dpi=300)
    plt.close()

    # Delay
    plt.figure(figsize=(8, 5), dpi=150)
    bars = plt.bar(scenarios, delay)
    plt.xticks(fontsize=11)
    plt.yticks(fontsize=11)
    plt.xlabel("Cenário", fontsize=12)
    plt.ylabel("Delay médio (s)", fontsize=12)
    for bar in bars:
        x = bar.get_x() + bar.get_width() / 2
        y = bar.get_height()
        plt.text(x, y, f"{y:.4f}", ha="center", va="bottom", fontsize=10)
    plt.title("Delay médio por cenário", fontsize=14)
    plt.grid(axis="y", linestyle="--", alpha=0.4)
    plt.tight_layout()
    plt.savefig(os.path.join(BASE_DIR, "qos_delay.png"), dpi=300)
    plt.close()


def plot_psnr_mos():
    psnr_rows = []

    for scen in [0, 1]:
        base_dir = f"results/scenario_{scen}"
        for fname in glob.glob(f"{base_dir}/psnr_*.txt"):
            # psnr_harbour_ue0_s0.txt
            base = fname.split("/")[-1].replace(".txt", "")
            parts = base.split("_")   # ['psnr','harbour','ue0','s0']
            if len(parts) < 4:
                continue
            video = parts[1]
            ue    = int(parts[2].replace("ue", ""))
            scen_f = int(parts[3].replace("s", ""))

            df = pd.read_csv(fname, sep=r"\s+", header=None)
            mean_psnr = df[0].mean()

            psnr_rows.append(
                dict(
                    scenario=scen_f,
                    video=video,
                    ue=ue,
                    mean_psnr=mean_psnr,
                )
            )

    if not psnr_rows:
        print("Nenhum psnr_*.txt encontrado.")
        return

    df = pd.DataFrame(psnr_rows)

    for scen in sorted(df["scenario"].unique()):
        d = df[df["scenario"] == scen]
        pivot = d.pivot_table(index="ue", values="mean_psnr", aggfunc="mean")
        plt.figure(figsize=(6, 4), dpi=150)
        ax = pivot.plot(kind="bar", legend=False)
        plt.ylabel("PSNR médio (dB)")
        plt.xlabel("UE")
        plt.title(f"PSNR médio por UE - cenário {scen}")
        plt.grid(axis="y", linestyle="--", alpha=0.4)

        # valores no topo
        for p in ax.patches:
            x = p.get_x() + p.get_width() / 2
            y = p.get_height()
            plt.text(x, y, f"{y:.2f}", ha="center", va="bottom", fontsize=9)

        plt.tight_layout()
        plt.savefig(f"results/psnr_s{scen}.png", dpi=300)
        plt.close()

def plot_largura_por_ue():
    scenarios = ["0", "1"]
    for s in scenarios:
        path = os.path.join(BASE_DIR, f"scenario_{s}")
        files = sorted(glob.glob(os.path.join(path, f"s{s}_QoS_drops_largura_ue*.txt")))
        if not files:
            continue

        ue_ids = []
        thr = []

        for f in files:
            # extrai índice do UE a partir do nome: ..._ueX.txt
            m = re.search(r"ue(\d+)\.txt$", f)
            if not m:
                continue
            ue = int(m.group(1))

            with open(f) as fh:
                line = fh.readline().strip()
                # linha tipo: "UE X throughput total: Y Mbps"
                m2 = re.search(r"([\d.]+)\s*Mbps", line)
                if not m2:
                    continue
                val = float(m2.group(1))
                ue_ids.append(ue)
                thr.append(val)

        if not ue_ids:
            continue

        plt.figure()
        plt.bar(ue_ids, thr)
        plt.xlabel("UE")
        plt.ylabel("Throughput total (Mbps)")
        plt.title(f"Throughput total por UE - cenário {s}")
        plt.xticks(ue_ids)
        plt.savefig(os.path.join(path, f"largura_ue_s{s}.png"), dpi=300)



if __name__ == "__main__":
    plot_qos()
    plot_psnr_mos()
    plot_largura_por_ue()

"""import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
import re
import numpy as np

BASE_DIR = "results"

def load_qos(scenario):
    path = os.path.join(BASE_DIR, f"scenario_{scenario}")
    df = {}

    # vazao continua como antes (assumindo 2 colunas numéricas)
    fname_thr = os.path.join(path, f"s{scenario}_QoS_vazao.txt")
    if os.path.exists(fname_thr):
        df["vazao"] = pd.read_csv(fname_thr, sep=r"\s+", header=None, engine="python")

    # jitter e perda: formato "Flow X: valor s"
    for metric in ["jitter", "perda"]:
        fname = os.path.join(path, f"s{scenario}_QoS_{metric}.txt")
        if os.path.exists(fname):
            vals = []
            with open(fname) as f:
                for line in f:
                    m = re.search(r":\s*([0-9.eE+-]+)", line)
                    if m:
                        vals.append(float(m.group(1)))
            df[metric] = np.array(vals)

    return df

def plot_qos():
    scenarios = ["0", "1"]
    thr, jitter, loss = [], [], []

    for s in scenarios:
        qos = load_qos(s)
        # vazão média
        if "vazao" in qos:
            thr.append(qos["vazao"][1].mean())
        else:
            thr.append(0.0)

        # jitter médio
        if "jitter" in qos and qos["jitter"].size > 0:
            jitter.append(float(qos["jitter"].mean()))
        else:
            jitter.append(0.0)

        # perda média
        if "perda" in qos and qos["perda"].size > 0:
            loss.append(float(qos["perda"].mean()))
        else:
            loss.append(0.0)
        delay_file = os.path.join(BASE_DIR, f"s{s}_QoS_delay.txt")
        if os.path.exists(delay_file):
            d_delay = pd.read_csv(delay_file, sep=r"\s+", header=None)
            qos["delay"] = d_delay[3].astype(float)  # mesma posição do jitter/perda


    # Throughput
    plt.figure(figsize=(8, 5), dpi=150)
    bars = plt.bar(scenarios, thr)
    plt.xticks(fontsize=11)
    plt.yticks(fontsize=11)
    plt.xlabel("Cenário", fontsize=12)
    plt.ylabel("Vazão média (Mbps)", fontsize=12)
    for bar in bars:
        x = bar.get_x() + bar.get_width() / 2
        y = bar.get_height()
        plt.text(x, y, f"{y:.6f}", ha="center", va="bottom", fontsize=10)
    plt.title("Vazão média por cenário", fontsize=14)
    plt.grid(axis="y", linestyle="--", alpha=0.4)
    plt.tight_layout()
    plt.savefig(os.path.join(BASE_DIR, "qos_throughput.png"), dpi=300)

    # Jitter
    plt.figure(figsize=(8, 5), dpi=150)
    bars = plt.bar(scenarios, jitter)
    plt.xticks(fontsize=11)
    plt.yticks(fontsize=11)
    plt.xlabel("Cenário", fontsize=12)
    plt.ylabel("Jitter médio (ms)", fontsize=12)
    for bar in bars:
        x = bar.get_x() + bar.get_width() / 2
        y = bar.get_height()
        plt.text(x, y, f"{y:.4f}", ha="center", va="bottom", fontsize=10)
    plt.title("Jitter médio por cenário", fontsize=14)
    plt.grid(axis="y", linestyle="--", alpha=0.4)
    plt.tight_layout()
    plt.savefig(os.path.join(BASE_DIR, "qos_jitter.png"), dpi=300)

    # Perda
    plt.figure(figsize=(8, 5), dpi=150)
    bars = plt.bar(scenarios, loss)
    plt.xticks(fontsize=11)
    plt.yticks(fontsize=11)
    plt.xlabel("Cenário", fontsize=12)
    plt.ylabel("Perda (%)", fontsize=12)
    for bar in bars:
        x = bar.get_x() + bar.get_width() / 2
        y = bar.get_height()
        plt.text(x, y, f"{y:.2f}", ha="center", va="bottom", fontsize=10)
    plt.title("Perda de pacotes por cenário", fontsize=14)
    plt.grid(axis="y", linestyle="--", alpha=0.4)
    plt.tight_layout()
    plt.savefig(os.path.join(BASE_DIR, "qos_loss.png"), dpi=300)



def plot_psnr_mos():
    psnr_rows = []

    for scen in [0, 1]:
        base_dir = f"results/scenario_{scen}"
        for fname in glob.glob(f"{base_dir}/psnr_*.txt"):
            # psnr_harbour_ue0_s0.txt
            base = fname.split("/")[-1].replace(".txt", "")
            parts = base.split("_")   # ['psnr','harbour','ue0','s0']
            if len(parts) < 4:
                continue
            video = parts[1]
            ue    = int(parts[2].replace("ue", ""))
            scen_f = int(parts[3].replace("s", ""))

            # uma coluna numérica, sem header
            df = pd.read_csv(fname, sep=r"\s+", header=None)
            mean_psnr = df[0].mean()

            psnr_rows.append(
                dict(
                    scenario=scen_f,
                    video=video,
                    ue=ue,
                    mean_psnr=mean_psnr,
                )
            )

    if not psnr_rows:
        print("Nenhum psnr_*.txt encontrado.")
        return

    df = pd.DataFrame(psnr_rows)

    # exemplo: PSNR médio por UE (média entre harbour/crew)
    for scen in sorted(df["scenario"].unique()):
        d = df[df["scenario"] == scen]
        pivot = d.pivot_table(index="ue", values="mean_psnr", aggfunc="mean")
        plt.figure(figsize=(8, 5), dpi=150)
        pivot.plot(kind="bar", legend=False)
        bars = plt.gca().patches
        for bar in bars:
            x = bar.get_x() + bar.get_width()/2
            y = bar.get_height()
            plt.text(x, y, f"{y:.2f}", ha="center", va="bottom", fontsize=9)
        plt.ylabel("PSNR médio (dB)")
        plt.xlabel("UE")
        plt.title(f"PSNR médio por UE - cenário {scen}")
        plt.grid(axis="y", linestyle="--", alpha=0.4)
        plt.tight_layout()
        plt.savefig(f"results/psnr_s{scen}.png")
        plt.close()

if __name__ == "__main__":
    plot_qos()
    plot_psnr_mos()
"""