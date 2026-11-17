import sys
from pathlib import Path
from scipy import stats
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


im = table("8. Зменшення", "9. Розподіл")
lim = [r for r in im if r[0][1] == "3"]
inf = [r for r in im if r[0][1] == "inf"]
xl = "Середній час передачі по лінії, мс"
plot([float(r[0][0]) for r in lim], [([float(r[1][0]) for r in lim], "", "-o")],
     xl, "Ймовірність відмови", "Рисунок_5.1.png")
plot([float(r[0][0]) for r in lim], [([float(r[1][3]) for r in lim], "Nmax = 3", "-o"),
                                     ([float(r[1][3]) for r in inf], "Nmax = ∞", "--s")],
     xl, "Середній час передачі, мс", "Рисунок_5.2.png")


rows = table("9. Розподіл")
a = [float(r[0][0]) for r in rows]
b = [float(r[0][1]) for r in rows]
f = [int(r[1][0]) for r in rows]
n = sum(f)
mid = [(x + y) / 2 for x, y in zip(a, b)]
m = sum(x * k for x, k in zip(mid, f)) / n
s = (sum((x - m) ** 2 * k for x, k in zip(mid, f)) / (n - 1)) ** 0.5

k_erl = round((m / s) ** 2)
theta = m / k_erl
cdf = lambda t: stats.gamma.cdf(t, k_erl, scale=theta)

edges = a + [b[-1]]
F = [cdf(t) for t in edges]
F[0], F[-1] = 0.0, 1.0
exp = [n * (F[i + 1] - F[i]) for i in range(len(f))]

obs_g, exp_g, int_g = [], [], []
o, e, lo = 0, 0.0, a[0]
for i in range(len(f)):
    o, e = o + f[i], e + exp[i]
    if e >= 5:
        obs_g.append(o)
        exp_g.append(e)
        int_g.append((lo, b[i]))
        o, e, lo = 0, 0.0, b[i]
obs_g[-1] += o
exp_g[-1] += e
int_g[-1] = (int_g[-1][0], b[-1])

chi2 = sum((o - e) ** 2 / e for o, e in zip(obs_g, exp_g))
df = len(obs_g) - 1 - 2
crit = stats.chi2.ppf(0.95, df)

print(f"\nn = {n}, середнє = {m:.2f} мс, СКВ = {s:.2f} мс, закон Ерланга: k = {k_erl}, θ = {theta:.3f} мс")
print("інтервал, мс | спостережувана | теоретична")
for (x, y), o, e in zip(int_g, obs_g, exp_g):
    print(f"{x:.0f}–{y:.0f} | {o} | {e:.1f}")
print(f"χ² = {chi2:.2f}, ступенів свободи = {df}, χ²кр(0,05) = {crit:.2f} ->",
      "гіпотезу прийнято" if chi2 < crit else "гіпотезу відхилено")

fig, ax = plt.subplots(figsize=(7, 4))
ax.bar(mid, [k / n / (y - x) for k, x, y in zip(f, a, b)], width=[y - x for x, y in zip(a, b)],
       color="silver", edgecolor="black", label="модель")
t = [a[0] + i * 0.1 for i in range(int((b[-1] - a[0]) / 0.1) + 1)]
ax.plot(t, stats.gamma.pdf(t, k_erl, scale=theta), "k-", label=f"закон Ерланга (k = {k_erl})")
ax.set_xlabel("Час передачі повідомлення, мс")
ax.set_ylabel("Щільність ймовірності")
ax.legend()
fig.tight_layout()
fig.savefig(DST / "Рисунок_4.7.png")
plt.close(fig)
print("збережено", DST / "Рисунок_4.7.png")
