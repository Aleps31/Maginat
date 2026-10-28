import itertools
import math
import os
import time

IT = os.environ.get("LANG", "").startswith("it")
FONT = "DejaVu Sans"
SEMI = ["♠", "♥", "♦", "♣"]
ETI = {11: "J", 12: "Q", 13: "K", 14: "A"}
FICHE_COL = {5: "#c0392b", 10: "#2471a3", 25: "#1e8449", 50: "#7d3c98", 100: "#222222", 500: "#b7950b"}
TAGLI = [500, 100, 50, 25, 10, 5]


def t(it, en):
    return it if IT else en


def _rgb(h):
    return int(h[1:3], 16), int(h[3:5], 16), int(h[5:7], 16)


def mix(a, b, f):
    x, y = _rgb(a), _rgb(b)
    return "#%02x%02x%02x" % tuple(int(x[i] + (y[i] - x[i]) * f) for i in range(3))


def rr(c, x1, y1, x2, y2, r=10, **kw):
    p = [x1 + r, y1, x1 + r, y1, x2 - r, y1, x2 - r, y1, x2, y1, x2, y1 + r, x2, y1 + r, x2, y2 - r, x2, y2 - r,
         x2, y2, x2 - r, y2, x2 - r, y2, x1 + r, y2, x1 + r, y2, x1, y2, x1, y2 - r, x1, y2 - r, x1, y1 + r,
         x1, y1 + r, x1, y1]
    return c.create_polygon(p, smooth=True, **kw)


def tavolo(c, w, h):
    if hasattr(c, "sfondo"):
        c.sfondo()
        return
    base, luce = "#0a3f28", "#1b8052"
    c.create_rectangle(0, 0, w, h, fill=base, outline="")
    n = 26
    for i in range(n):
        f = i / (n - 1)
        rx = w * 0.66 * (1 - f * 0.88)
        ry = h * 0.62 * (1 - f * 0.88)
        c.create_oval(w / 2 - rx, h / 2 - ry, w / 2 + rx, h / 2 + ry, fill=mix(base, luce, f), outline="")


def targa(c, x, y, testo, fill="#e8f5ec", anchor="nw", size=13, bold=True):
    w = len(testo) * size * 0.78 + 20
    h = size * 2.0
    x1 = x if anchor in ("nw", "w", "sw") else (x - w if anchor in ("ne", "e", "se") else x - w / 2)
    y1 = y if anchor in ("nw", "n", "ne") else (y - h if anchor in ("sw", "s", "se") else y - h / 2)
    rr(c, x1, y1, x1 + w, y1 + h, 8, fill="#06281a", outline="#2d7a4d")
    c.create_text(x1 + w / 2, y1 + h / 2, text=testo, fill=fill, font=(FONT, size, "bold" if bold else "normal"))


PIP = {
    2: [(1, 0), (1, 1)], 3: [(1, 0), (1, .5), (1, 1)],
    4: [(0, 0), (2, 0), (0, 1), (2, 1)], 5: [(0, 0), (2, 0), (0, 1), (2, 1), (1, .5)],
    6: [(0, 0), (2, 0), (0, .5), (2, .5), (0, 1), (2, 1)],
    7: [(0, 0), (2, 0), (0, .5), (2, .5), (0, 1), (2, 1), (1, .25)],
    8: [(0, 0), (2, 0), (0, .5), (2, .5), (0, 1), (2, 1), (1, .25), (1, .75)],
    9: [(0, 0), (2, 0), (0, 1 / 3), (2, 1 / 3), (0, 2 / 3), (2, 2 / 3), (0, 1), (2, 1), (1, .5)],
    10: [(0, 0), (2, 0), (0, 1 / 3), (2, 1 / 3), (0, 2 / 3), (2, 2 / 3), (0, 1), (2, 1), (1, 1 / 6), (1, 5 / 6)],
}


def corona(c, cx, yc, w, h, tipo):
    oro, bordo = "#e0a82e", "#8a6414"
    if tipo == 11:
        c.create_polygon(cx - .13 * w, yc + .05 * h, cx - .10 * w, yc - .03 * h, cx + .10 * w, yc - .06 * h, cx + .13 * w, yc + .05 * h,
                         fill="#2e8b57", outline="#14502e", smooth=False)
        c.create_line(cx + .08 * w, yc - .05 * h, cx + .20 * w, yc - .14 * h, fill="#c0392b", width=2)
        return
    pts = [cx - .13 * w, yc + .05 * h, cx - .13 * w, yc - .05 * h, cx - .065 * w, yc, cx, yc - .07 * h,
           cx + .065 * w, yc, cx + .13 * w, yc - .05 * h, cx + .13 * w, yc + .05 * h]
    c.create_polygon(pts, fill=oro, outline=bordo)
    if tipo == 12:
        for dx in (-.065, 0, .065):
            c.create_oval(cx + dx * w - 2, yc - .02 * h, cx + dx * w + 2, yc + .02 * h, fill="#c0392b", outline="")


def carta(c, x, y, k, w=72, h=104, coperta=False, evid=False):
    f = w / 72.0
    rr(c, x + 5, y + 8, x + w + 5, y + h + 8, 10, fill="#073d25", outline="")
    rr(c, x + 3, y + 5, x + w + 3, y + h + 5, 9, fill="#052a1a", outline="")
    if coperta:
        rr(c, x, y, x + w, y + h, 9, fill="#fdfdf8", outline="#3a3a3a", width=2)
        rr(c, x + 5, y + 5, x + w - 5, y + h - 5, 6, fill="#1d3c6e", outline="#1d3c6e")
        x0, x1, y0, y1 = x + 5, x + w - 5, y + 5, y + h - 5
        pas = 14 * f
        b = y0 - x1
        while b <= y1 - x0:
            a1, a2 = max(x0, y0 - b), min(x1, y1 - b)
            if a1 < a2:
                c.create_line(a1, a1 + b, a2, a2 + b, fill="#2f5aa0")
            b += pas
        b = y0 + x0
        while b <= y1 + x1:
            a1, a2 = max(x0, b - y1), min(x1, b - y0)
            if a1 < a2:
                c.create_line(a1, b - a1, a2, b - a2, fill="#2f5aa0")
            b += pas
        rr(c, x + 5, y + 5, x + w - 5, y + h - 5, 6, fill="", outline="#9bbcf0", width=1)
        c.create_oval(x + w / 2 - 17 * f, y + h / 2 - 17 * f, x + w / 2 + 17 * f, y + h / 2 + 17 * f, fill="#07142a", outline="#ffffff", width=2)
        c.create_text(x + w / 2, y + h / 2, text="M", fill="#ffffff", font=(FONT, int(17 * f), "bold"))
        return
    r, s = k
    col = "#cf2e22" if s in "♥♦" else "#161616"
    if evid:
        rr(c, x - 4, y - 4, x + w + 4, y + h + 4, 12, fill="#ffd54a", outline="")
    rr(c, x, y, x + w, y + h, 9, fill="#fdfdf8", outline="#ffd54a" if evid else "#3a3a3a", width=3 if evid else 2)
    c.create_line(x + 10 * f, y + 3, x + w - 10 * f, y + 3, fill="#ffffff", width=1)
    c.create_line(x + 10 * f, y + h - 3, x + w - 10 * f, y + h - 3, fill="#e6e4d6", width=1)
    e = ETI.get(r, str(r))
    c.create_text(x + 7 * f, y + 4 * f, text=e, fill=col, anchor="nw", font=(FONT, int(13 * f), "bold"))
    c.create_text(x + 8 * f, y + 21 * f, text=s, fill=col, anchor="nw", font=(FONT, int(11 * f)))
    c.create_text(x + w - 7 * f, y + h - 4 * f, text=e, fill=col, anchor="se", font=(FONT, int(13 * f), "bold"), angle=0)
    c.create_text(x + w - 8 * f, y + h - 21 * f, text=s, fill=col, anchor="se", font=(FONT, int(11 * f)))
    cx = x + w / 2
    if r in PIP:
        x0, x1 = x + w * 0.38, x + w * 0.62
        y0, y1 = y + h * 0.25, y + h * 0.75
        for ci, rf in PIP[r]:
            px = x0 + (x1 - x0) * ci / 2.0
            py = y0 + (y1 - y0) * rf
            c.create_text(px, py, text=s, fill=col, font=(FONT, max(7, int(12 * f))), angle=180 if rf > 0.5 else 0)
    elif r == 14:
        c.create_text(cx, y + h / 2 + 2 * f, text=s, fill=col, font=(FONT, int(38 * f)))
    else:
        rr(c, x + 15 * f, y + 17 * f, x + w - 15 * f, y + h - 17 * f, 6, fill="#f6efd8", outline=col, width=1)
        rr(c, x + 19 * f, y + 21 * f, x + w - 19 * f, y + h - 21 * f, 4, fill="", outline="#c9a227", width=1)
        corona(c, cx, y + h * 0.31, w, h, r)
        c.create_text(cx, y + h * 0.51, text=e, fill=col, font=(FONT, int(24 * f), "bold"))
        c.create_text(cx, y + h * 0.71, text=s, fill=col, font=(FONT, int(17 * f)))


def fiche(c, x, y, v, r=20):
    col = FICHE_COL.get(v, "#555555")
    c.create_oval(x - r + 2, y - r + 4, x + r + 2, y + r + 4, fill="#052a1a", outline="")
    c.create_oval(x - r, y - r, x + r, y + r, fill=mix(col, "#000000", 0.25), outline="#1a1a1a", width=1)
    for i in range(8):
        c.create_arc(x - r, y - r, x + r, y + r, start=i * 45 - 11, extent=22, style="pieslice", fill="#fdfdf8", outline="")
    c.create_oval(x - r * 0.80, y - r * 0.80, x + r * 0.80, y + r * 0.80, fill=col, outline="#fdfdf8", width=1)
    c.create_oval(x - r * 0.58, y - r * 0.58, x + r * 0.58, y + r * 0.58, fill=mix(col, "#ffffff", 0.16), outline=mix(col, "#000000", 0.3))
    c.create_arc(x - r * 0.9, y - r * 0.9, x + r * 0.9, y + r * 0.9, start=95, extent=60, style="arc", outline=mix(col, "#ffffff", 0.55), width=2)
    c.create_text(x, y, text=str(v), fill="#ffffff" if v != 500 else "#000000", font=(FONT, max(7, int(r * 0.48)), "bold"))


def dealer(c, cx, cy, s=1.0, umore="neutro"):
    pelle, ombra = "#e9c29c", "#b88a60"
    k = lambda v: v * s
    c.create_polygon(cx - k(64), cy + k(78), cx - k(60), cy + k(42), cx - k(34), cy + k(29), cx + k(34), cy + k(29),
                     cx + k(60), cy + k(42), cx + k(64), cy + k(78), smooth=True, fill="#1a1b24", outline="#05060a", width=2)
    c.create_rectangle(cx - k(9), cy + k(18), cx + k(9), cy + k(34), fill=ombra, outline="")
    c.create_polygon(cx - k(17), cy + k(29), cx + k(17), cy + k(29), cx, cy + k(66), fill="#f6f6f2", outline="#c8c8c0")
    c.create_line(cx - k(34), cy + k(32), cx - k(8), cy + k(70), fill="#3a3c4c", width=max(1, k(2)))
    c.create_line(cx + k(34), cy + k(32), cx + k(8), cy + k(70), fill="#3a3c4c", width=max(1, k(2)))
    c.create_polygon(cx, cy + k(33), cx - k(15), cy + k(25), cx - k(15), cy + k(41), fill="#b8201a", outline="#6e0e0a")
    c.create_polygon(cx, cy + k(33), cx + k(15), cy + k(25), cx + k(15), cy + k(41), fill="#b8201a", outline="#6e0e0a")
    c.create_oval(cx - k(4), cy + k(29), cx + k(4), cy + k(37), fill="#d92b24", outline="#6e0e0a")
    c.create_oval(cx - k(46), cy + k(46), cx - k(30), cy + k(62), fill="#07142a", outline="#ffffff", width=1)
    c.create_text(cx - k(38), cy + k(54), text="M", fill="#ffffff", font=(FONT, max(6, int(8 * s)), "bold"))
    c.create_oval(cx - k(27), cy - k(6), cx - k(21), cy + k(8), fill=pelle, outline=ombra)
    c.create_oval(cx + k(21), cy - k(6), cx + k(27), cy + k(8), fill=pelle, outline=ombra)
    c.create_oval(cx - k(24), cy - k(32), cx + k(24), cy + k(26), fill=pelle, outline=ombra, width=1)
    c.create_arc(cx - k(25), cy - k(34), cx + k(25), cy + k(22), start=25, extent=130, style="chord", fill="#2a1d14", outline="#2a1d14")
    c.create_polygon(cx - k(26), cy - k(19), cx + k(26), cy - k(19), cx + k(36), cy - k(9), cx - k(36), cy - k(9),
                     fill="#2fb36c", outline="#14683a", width=1)
    c.create_line(cx - k(25), cy - k(20), cx + k(25), cy - k(20), fill="#8fe3b4", width=1)
    for dx in (-9, 9):
        c.create_line(cx + k(dx) - k(5), cy - k(4), cx + k(dx) + k(5), cy - k(4), fill="#2a1d14", width=max(1, k(1.5)))
        c.create_oval(cx + k(dx) - k(4), cy, cx + k(dx) + k(4), cy + k(7), fill="#ffffff", outline="#555555")
        c.create_oval(cx + k(dx) - k(2), cy + k(2), cx + k(dx) + k(2), cy + k(6), fill="#1a1a1a", outline="")
    c.create_line(cx, cy + k(6), cx - k(2), cy + k(12), cx + k(1), cy + k(13), fill=ombra, width=max(1, k(1.3)))
    if umore == "sorriso":
        c.create_arc(cx - k(10), cy + k(8), cx + k(10), cy + k(22), start=200, extent=140, style="arc", outline="#7a2a22", width=max(1, k(2)))
    elif umore == "triste":
        c.create_arc(cx - k(9), cy + k(16), cx + k(9), cy + k(28), start=20, extent=140, style="arc", outline="#7a2a22", width=max(1, k(2)))
    elif umore == "sorpresa":
        c.create_oval(cx - k(4), cy + k(15), cx + k(4), cy + k(23), fill="#5a1a14", outline="#7a2a22")
    else:
        c.create_line(cx - k(7), cy + k(18), cx + k(7), cy + k(18), fill="#7a2a22", width=max(1, k(2)))
    rr(c, cx - k(86), cy + k(64), cx + k(86), cy + k(82), 6, fill="#4a2a12", outline="#1a0d05", width=2)
    c.create_line(cx - k(80), cy + k(67), cx + k(80), cy + k(67), fill="#7a4a22", width=1)
    for dx in (-36, 36):
        c.create_oval(cx + k(dx) - k(10), cy + k(56), cx + k(dx) + k(10), cy + k(70), fill=pelle, outline=ombra)


BOT_COL = {"pri": ("#1f8a4c", "#2fae65"), "alt": ("#14532d", "#1f7a45"), "peri": ("#8a2f25", "#b04032")}


def bottone(bt, tipo="alt"):
    bg, att = BOT_COL.get(tipo, BOT_COL["alt"])
    bt.configure(bg=bg, fg="#ffffff", activebackground=att, activeforeground="#ffffff",
                 disabledforeground="#6f8f7d", relief="flat", bd=0, padx=6, pady=4, highlightthickness=0, cursor="hand2",
                 font=(FONT, 9, "bold" if tipo == "pri" else "normal"))


def stato_bottone(bt, tipo, on):
    bt.config(state="normal" if on else "disabled", bg=BOT_COL.get(tipo, BOT_COL["alt"])[0] if on else "#0f3d24")


def casella(cb):
    cb.configure(bg=BASE, fg="#e8f5ec", selectcolor="#14532d", activebackground=BASE, activeforeground="#ffffff",
                 highlightthickness=0, bd=0, font=(FONT, 9))


def pannello(c, x, y, testo, size=13, maxw=700, fill="#ffd54a", prog=None):
    """Targa scura con bordo oro per i messaggi, centrata in x, y."""
    try:
        import tkinter.font as tkf
        tw = tkf.Font(root=c.c, family=FONT, size=size, weight="bold").measure(testo)
    except Exception:
        tw = len(testo) * size * 1.1
    righe = max(1, int(-(-tw // maxw)))
    w = min(maxw, tw) + 36
    h = righe * size * 1.9 + 12
    rr(c, x - w / 2 + 2, y - h / 2 + 3, x + w / 2 + 2, y + h / 2 + 3, 10, fill="#052a1a", outline="")
    rr(c, x - w / 2, y - h / 2, x + w / 2, y + h / 2, 10, fill="#06281a", outline="#c9a227", width=1)
    c.create_text(x, y - (3 if prog is not None else 0), text=testo, fill=fill, font=(FONT, size, "bold"), width=maxw, justify="center")
    if prog is not None:
        c.create_rectangle(x - w / 2 + 14, y + h / 2 - 7, x + w / 2 - 14, y + h / 2 - 4, fill="#0b3322", outline="")
        c.create_rectangle(x - w / 2 + 14, y + h / 2 - 7, x - w / 2 + 14 + (w - 28) * prog, y + h / 2 - 4, fill="#ffd54a", outline="")


def badge(c, x, y, testo, rosso=False):
    c.create_oval(x - 15, y - 14, x + 17, y + 18, fill="#052a1a", outline="")
    c.create_oval(x - 16, y - 16, x + 16, y + 16, fill="#7a1d16" if rosso else "#06281a", outline="#ff8a7a" if rosso else "#ffd54a", width=2)
    c.create_text(x, y, text=testo, fill="#ffffff" if rosso else "#ffd54a", font=(FONT, 14, "bold"))


def cerchio(c, x, y, r, dash=(4, 4)):
    c.create_oval(x - r, y - r, x + r, y + r, outline="#2d7a4d", width=2, dash=dash)


def colora(w):
    w.configure(bg=BASE)


def pila(c, cx, cy, importo, r=16):
    colonne = []
    resto = importo
    for tg in TAGLI:
        n = resto // tg
        if n:
            colonne.append((tg, min(n, 8)))
            resto -= n * tg
    if not colonne:
        return
    colonne = colonne[:5]
    wtot = (len(colonne) - 1) * (2 * r + 4)
    x = cx - wtot / 2
    for tg, n in colonne:
        for i in range(n):
            fiche(c, x, cy - i * max(3, r // 4), tg, r)
        x += 2 * r + 4


def valuta(mano):
    r = sorted((k[0] for k in mano), reverse=True)
    colore = len({k[1] for k in mano}) == 1
    uniche = sorted(set(r), reverse=True)
    scala = len(uniche) == 5 and uniche[0] - uniche[4] == 4
    if set(r) == {14, 2, 3, 4, 5}:
        scala = True
        r = [5, 4, 3, 2, 1]
    cont = {}
    for x in r:
        cont[x] = cont.get(x, 0) + 1
    gruppi = sorted(cont.items(), key=lambda kv: (kv[1], kv[0]), reverse=True)
    ordine = [k for k, v in gruppi]
    forma = [v for k, v in gruppi]
    if scala and colore:
        cat = 8
    elif forma == [4, 1]:
        cat = 7
    elif forma == [3, 2]:
        cat = 6
    elif colore:
        cat = 5
    elif scala:
        cat = 4
    elif forma == [3, 1, 1]:
        cat = 3
    elif forma == [2, 2, 1]:
        cat = 2
    elif forma == [2, 1, 1, 1]:
        cat = 1
    else:
        cat = 0
    return (cat, ordine)


def migliore(carte):
    if len(carte) <= 5:
        return valuta(carte)
    return max(valuta(c) for c in itertools.combinations(carte, 5))


BASE = "#0a3f28"


class Scala:
    def __init__(self, c, W, H):
        self.c, self.W, self.H = c, W, H
        self.s, self.ox, self.oy = 1.0, 0.0, 0.0

    def imposta(self):
        w = max(self.c.winfo_width(), 200)
        h = max(self.c.winfo_height(), 150)
        self.s = min(w / self.W, h / self.H)
        self.ox = (w - self.W * self.s) / 2
        self.oy = (h - self.H * self.s) / 2

    def inv(self, x, y):
        return (x - self.ox) / self.s, (y - self.oy) / self.s

    def _co(self, a):
        return [v * self.s + (self.ox if i % 2 == 0 else self.oy) for i, v in enumerate(a)]

    def _kw(self, kw):
        kw = dict(kw)
        s = self.s
        if "width" in kw:
            kw["width"] = max(1, kw["width"] * s) if isinstance(kw["width"], (int, float)) else kw["width"]
        if "font" in kw:
            f = list(kw["font"])
            f[1] = max(6, int(round(f[1] * s)))
            kw["font"] = tuple(f)
        if "dash" in kw:
            kw["dash"] = tuple(max(1, int(d * s)) for d in kw["dash"])
        return kw

    def _mk(self, nome, a, kw):
        if len(a) == 1 and isinstance(a[0], (list, tuple)):
            a = a[0]
        return getattr(self.c, nome)(*self._co(a), **self._kw(kw))

    def create_rectangle(self, *a, **kw):
        return self._mk("create_rectangle", a, kw)

    def create_oval(self, *a, **kw):
        return self._mk("create_oval", a, kw)

    def create_line(self, *a, **kw):
        return self._mk("create_line", a, kw)

    def create_polygon(self, *a, **kw):
        return self._mk("create_polygon", a, kw)

    def create_text(self, *a, **kw):
        return self._mk("create_text", a, kw)

    def create_arc(self, *a, **kw):
        return self._mk("create_arc", a, kw)

    def sfondo(self):
        c = self.c
        w = max(c.winfo_width(), 200)
        h = max(c.winfo_height(), 150)
        base, luce = BASE, "#1b8052"
        c.create_rectangle(0, 0, w, h, fill=base, outline="")
        n = 26
        for i in range(n):
            f = i / (n - 1)
            rx = w * 0.66 * (1 - f * 0.88)
            ry = h * 0.62 * (1 - f * 0.88)
            c.create_oval(w / 2 - rx, h / 2 - ry, w / 2 + rx, h / 2 + ry, fill=mix(base, luce, f), outline="")
        c.create_text(w / 2, h * 0.60, text="MAGINAT", fill="#12633c", font=(FONT, max(10, int(46 * self.s)), "bold"))
        t = max(8, int(10 * self.s))
        for r in ((0, 0, w, t), (0, h - t, w, h), (0, 0, t, h), (w - t, 0, w, h)):
            c.create_rectangle(*r, fill="#4a2a12", outline="")
        c.create_rectangle(1, 1, w - 1, h - 1, outline="#7a4a22", width=2)
        c.create_rectangle(t * 0.5, t * 0.5, w - t * 0.5, h - t * 0.5, outline="#5f3718", width=1)
        c.create_rectangle(t, t, w - t, h - t, outline="#1a0d05", width=2)
        rr(c, t + 5, t + 5, w - t - 5, h - t - 5, 18, fill="", outline="#c9a227", width=1)

    def delete(self, *a):
        return self.c.delete(*a)


class Anim:
    """Animazioni per i giochi: carte che volano, carte che si girano, fiches che volano."""
    DUR_VOLO = 0.26
    DUR_FLIP = 0.30

    def anim_init(self, root, ridisegna):
        self._an_root = root
        self._an_rid = ridisegna
        self._an_pend = []
        self._an_viste = {}
        self._an_voli = {}
        self._an_flip = {}
        self._an_fiche = []
        self._an_testi = []
        self._an_job = None
        self._an_att = False

    def anim_testo(self, x, y, testo, col):
        self._an_testi.append((time.monotonic(), x, y, testo, col))
        self._an_att = True

    def anim_reset(self):
        self._an_testi = []
        self._an_viste.clear()
        self._an_voli.clear()
        self._an_flip.clear()
        self._an_fiche = []
        self._an_pend = []

    def anim_carta(self, c, key, da, x, y, k, w=72, h=104, coperta=False, evid=False, ritardo=0.0):
        ora = time.monotonic()
        if key not in self._an_viste:
            self._an_voli[key] = (ora + ritardo, da)
            self._an_viste[key] = coperta
        fl = self._an_voli.get(key)
        if fl is not None:
            u = (ora - fl[0]) / self.DUR_VOLO
            if u >= 1:
                del self._an_voli[key]
            else:
                self._an_att = True
                if u >= 0:
                    self._an_pend.append((u, fl[1], x, y, k, w, h, coperta))
                return
        prec = self._an_viste[key]
        self._an_viste[key] = coperta
        if prec and not coperta and key not in self._an_flip:
            self._an_flip[key] = ora
        fp = self._an_flip.get(key)
        if fp is not None:
            u = (ora - fp) / self.DUR_FLIP
            if u >= 1:
                del self._an_flip[key]
            else:
                self._an_att = True
                ww = max(4, int(w * abs(1 - 2 * u)))
                if ww < 26:
                    rr(c, x + (w - ww) / 2.0, y, x + (w + ww) / 2.0, y + h, min(9, ww / 2.0),
                       fill="#1d3c6e" if u < 0.5 else "#fdfdf8", outline="#3a3a3a", width=2)
                else:
                    carta(c, x + (w - ww) / 2.0, y, k, w=ww, h=h, coperta=(u < 0.5), evid=evid)
                return
        carta(c, x, y, k, w=w, h=h, coperta=coperta, evid=evid)

    def anim_fiche(self, da, a, v, r):
        self._an_fiche.append((time.monotonic(), da, a, v, r))
        self._an_att = True

    def anim_u(self, t0, dur):
        u = (time.monotonic() - t0) / dur
        if u < 1:
            self._an_att = True
        return max(0.0, min(1.0, u))

    def anim_fine(self, c):
        ora = time.monotonic()
        for (u, da, x, y, k, w, h, cop) in self._an_pend:
            e = 1 - (1 - u) ** 3
            s = 0.6 + 0.4 * e
            carta(c, da[0] + (x - da[0]) * e, da[1] + (y - da[1]) * e, k,
                  w=max(10, int(w * s)), h=max(14, int(h * s)), coperta=cop)
        self._an_pend = []
        vive = []
        for f in self._an_fiche:
            u = (ora - f[0]) / 0.22
            if u < 1:
                e = 1 - (1 - u) ** 3
                fiche(c, f[1][0] + (f[2][0] - f[1][0]) * e,
                      f[1][1] + (f[2][1] - f[1][1]) * e - 18 * math.sin(math.pi * u), f[3], f[4])
                vive.append(f)
        self._an_fiche = vive
        if vive:
            self._an_att = True
        vivi = []
        for (t0, x, y, testo, col) in self._an_testi:
            u = (ora - t0) / 1.8
            if u < 1:
                e = 1 - (1 - min(1, u * 1.4)) ** 2
                yy = y - 44 * e
                cc = col if u < 0.6 else mix(col, BASE, (u - 0.6) / 0.4)
                ombra = mix("#052a1a", BASE, 0) if u < 0.6 else mix("#052a1a", BASE, (u - 0.6) / 0.4)
                c.create_text(x + 2, yy + 2, text=testo, fill=ombra, font=(FONT, 28, "bold"))
                c.create_text(x, yy, text=testo, fill=cc, font=(FONT, 28, "bold"))
                vivi.append((t0, x, y, testo, col))
        self._an_testi = vivi
        if vivi:
            self._an_att = True
        if self._an_att and self._an_job is None:
            self._an_job = self._an_root.after(33, self._an_tick)
        self._an_att = False

    def _an_tick(self):
        self._an_job = None
        try:
            self._an_rid()
        except Exception:
            pass
