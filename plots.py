import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

SRC = Path(sys.argv[1] if len(sys.argv) > 1 else "out.txt")
DST = Path(sys.argv[2] if len(sys.argv) > 2 else "figures")
DST.mkdir(exist_ok=True)
TEXT = SRC.read_text(encoding="utf-8")

plt.rcParams.update({
    "font.family": "serif",
    "font.serif": ["Times New Roman", "Liberation Serif", "DejaVu Serif"],
    "font.size": 12,
    "axes.grid": True,
    "grid.color": "#dddddd",
    "savefig.dpi": 200,
})


def table(start, end=None):
    part = TEXT.split(start, 1)[1]
    if end:
        part = part.split(end, 1)[0]
    rows = [line for line in part.splitlines() if "|" in line]
    return [[cell.split() for cell in row.split("|")] for row in rows[1:]]


def plot(x, lines, xlabel, ylabel, name, logx=False, marks=()):
    fig, ax = plt.subplots(figsize=(7, 4))
    for y, label, style in lines:
        ax.plot(x, y, style, color="black", label=label, markersize=5)
    for xv, text in marks:
        ax.axvline(xv, color="gray", linestyle="--")
        ax.text(xv, ax.get_ylim()[1], " " + text, va="top")
    if logx:
        ax.set_xscale("log")
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    if any(label for _, label, _ in lines):
        ax.legend()
    fig.tight_layout()
    fig.savefig(DST / name)
    plt.close(fig)
    print("збережено", DST / name)


def bars(model, calc, ylabel, name):
    x = range(1, len(model) + 1)
    fig, ax = plt.subplots(figsize=(7, 4))
    ax.bar([i - 0.2 for i in x], model, 0.4, color="dimgray", label="модель")
    ax.bar([i + 0.2 for i in x], calc, 0.4, color="silver", label="розрахунок")
    ax.set_xticks(list(x))
    ax.set_xlabel("Номер прогону верифікації")
    ax.set_ylabel(ylabel)
    ax.set_ylim(0, 1.15)
    ax.legend(ncol=2, loc="upper center")
    fig.tight_layout()
    fig.savefig(DST / name)
    plt.close(fig)
    print("збережено", DST / name)


ver = table("3. Верифікація", "4. Перехідний")
bars([float(r[1][0]) for r in ver], [float(r[2][0]) for r in ver],
     "Завантаження процесора Uпр", "Рисунок_3.2.png")
bars([float(r[1][1]) for r in ver], [float(r[2][1]) for r in ver],
     "Завантаження вихідної лінії Uлін 1", "Рисунок_3.3.png")


tr = table("4. Перехідний", "5. Кількість")
t = [float(r[0][0]) for r in tr]
styles = ["-", "--", "-.", ":", "-x"]
for name, col, ylabel, mark in (("Рисунок_4.1.png", 1, "Середній час передачі, мс", 5000),
                                ("Рисунок_4.2.png", 2, "Ймовірність відмови", 14000)):
    lines = [([float(r[col][s]) for r in tr], f"прогін {s + 1}", styles[s]) for s in range(5)]
    plot(t, lines, "Модельний час t, мс", ylabel, name, marks=[(mark, f"{mark} мс")])


nm = [r for r in table("6. Вплив", "Дисперсійний") if r[0][0] != "inf"]
n = [int(r[0][0]) for r in nm]
xl = "Гранична кількість повідомлень Nmax"
plot(n, [([float(r[1][0]) for r in nm], "", "-o")], xl, "Ймовірність відмови", "Рисунок_4.3.png")
plot(n, [([float(r[1][3]) for r in nm], "", "-o")], xl, "Середній час передачі, мс", "Рисунок_4.4.png")
plot(n, [([float(r[1][1]) for r in nm], "процесор", "-o"),
         ([float(r[1][2]) for r in nm], "вихідна лінія 1", "--s")], xl, "Завантаження", "Рисунок_4.5.png")


nf = table("7. Без контролю", "8. Зменшення")
plot([float(r[0][0]) for r in nf], [([float(r[1][0]) for r in nf], "", "-o")],
     "Час моделювання Tmod, мс", "Середній час передачі, мс", "Рисунок_4.6.png", logx=True)


im = table("8. Зменшення")
lim = [r for r in im if r[0][1] == "3"]
inf = [r for r in im if r[0][1] == "inf"]
xl = "Середній час передачі по лінії, мс"
plot([float(r[0][0]) for r in lim], [([float(r[1][0]) for r in lim], "", "-o")],
     xl, "Ймовірність відмови", "Рисунок_5.1.png")
plot([float(r[0][0]) for r in lim], [([float(r[1][3]) for r in lim], "Nmax = 3", "-o"),
                                     ([float(r[1][3]) for r in inf], "Nmax = ∞", "--s")],
     xl, "Середній час передачі, мс", "Рисунок_5.2.png")
