import math, os, subprocess, sys
import cairo

DARK = (0.09, 0.12, 0.16)
WHITE = (1.0, 1.0, 1.0)
BLUE = (0.29, 0.64, 1.0)
RED = (0.88, 0.20, 0.20)
GREEN = (0.20, 0.72, 0.35)
OL = 2.6
SIZES = (24, 32, 48, 64)


def rrect(c, x, y, w, h, r):
    c.new_sub_path()
    c.arc(x + w - r, y + r, r, -math.pi / 2, 0)
    c.arc(x + w - r, y + h - r, r, 0, math.pi / 2)
    c.arc(x + r, y + h - r, r, math.pi / 2, math.pi)
    c.arc(x + r, y + r, r, math.pi, 1.5 * math.pi)
    c.close_path()


def union(c, paths, fill=DARK, line=WHITE):
    c.set_line_join(cairo.LINE_JOIN_ROUND)
    c.set_line_cap(cairo.LINE_CAP_ROUND)
    c.set_source_rgb(*line)
    c.set_line_width(OL * 2)
    for p in paths:
        p(c)
        c.stroke()
    c.set_source_rgb(*fill)
    for p in paths:
        p(c)
        c.fill()


def poly(pts):
    def f(c):
        c.move_to(*pts[0])
        for q in pts[1:]:
            c.line_to(*q)
        c.close_path()
    return f


def rr(x, y, w, h, r):
    return lambda c: rrect(c, x, y, w, h, r)


def rot_rr(cx, cy, w, h, r, ang):
    def f(c):
        c.save()
        c.translate(cx, cy)
        c.rotate(math.radians(ang))
        rrect(c, -w / 2, -h / 2, w, h, r)
        c.restore()
    return f


def circ(cx, cy, r):
    def f(c):
        c.new_sub_path()
        c.arc(cx, cy, r, 0, 2 * math.pi)
    return f


ARROW = [(9, 5), (9, 37), (16.5, 30.5), (22, 42.5), (28, 40), (22.5, 28.5), (32, 28)]


def d_arrow(c, fr=0):
    union(c, [poly(ARROW)])


def d_pointer(c, fr=0):
    union(c, [rr(14, 4, 9, 26, 4.5), rr(23, 15, 8, 20, 4), rr(31, 17, 7, 18, 3.5),
              rr(38, 21, 6, 15, 3), rr(14, 22, 30, 20, 8), rot_rr(11, 30, 8, 15, 4, -25)])
    c.set_source_rgb(*WHITE)
    c.set_line_width(1.2)
    c.set_line_cap(cairo.LINE_CAP_ROUND)
    for x, y0, y1 in ((23, 16, 27), (31, 18, 27), (38, 22, 29)):
        c.move_to(x, y0)
        c.line_to(x, y1)
        c.stroke()


def d_text(c, fr=0):
    c.set_line_cap(cairo.LINE_CAP_ROUND)
    for col, w in ((WHITE, 2.6 + OL * 2), (DARK, 2.6)):
        c.set_source_rgb(*col)
        c.set_line_width(w)
        c.move_to(24, 9)
        c.line_to(24, 39)
        c.stroke()
        c.move_to(17, 9)
        c.line_to(31, 9)
        c.stroke()
        c.move_to(17, 39)
        c.line_to(31, 39)
        c.stroke()


def spinner(c, cx, cy, r0, r1, wd, n, k, col=DARK):
    c.set_line_cap(cairo.LINE_CAP_ROUND)
    for pass_, color, width in ((0, WHITE, wd + OL * 0.6), (1, col, wd)):
        for i in range(n):
            a = 0.15 + 0.85 * (((i - k) % n) / (n - 1)) if False else 1.0 - 0.85 * (((k - i) % n) / (n - 1))
            ang = 2 * math.pi * i / n - math.pi / 2
            c.set_source_rgba(*color, a)
            c.set_line_width(width)
            c.move_to(cx + r0 * math.cos(ang), cy + r0 * math.sin(ang))
            c.line_to(cx + r1 * math.cos(ang), cy + r1 * math.sin(ang))
            c.stroke()


def d_wait(c, fr=0):
    spinner(c, 24, 24, 10, 19, 3.2, 12, fr)


def d_progress(c, fr=0):
    union(c, [poly(ARROW)])
    c.set_source_rgb(*WHITE)
    c.arc(37, 37, 10.5, 0, 2 * math.pi)
    c.fill()
    spinner(c, 37, 37, 4, 8, 2.4, 8, fr % 8)


def d_cross(c, fr=0):
    c.set_line_cap(cairo.LINE_CAP_ROUND)
    for col, w in ((WHITE, 2.2 + OL * 2), (DARK, 2.2)):
        c.set_source_rgb(*col)
        c.set_line_width(w)
        for a, b, x, y in ((6, 19, 1, 0), (29, 42, 1, 0)):
            c.move_to(a, 24)
            c.line_to(b, 24)
            c.stroke()
        for a, b in ((6, 19), (29, 42)):
            c.move_to(24, a)
            c.line_to(24, b)
            c.stroke()


def head(x, y, ang):
    def f(c):
        c.save()
        c.translate(x, y)
        c.rotate(math.radians(ang))
        c.move_to(0, 0)
        c.line_to(-9, -7)
        c.line_to(-9, 7)
        c.close_path()
        c.restore()
    return f


def bar(x0, y0, x1, y1, w=4):
    def f(c):
        dx, dy = x1 - x0, y1 - y0
        L = math.hypot(dx, dy)
        nx, ny = -dy / L * w / 2, dx / L * w / 2
        c.move_to(x0 + nx, y0 + ny)
        c.line_to(x1 + nx, y1 + ny)
        c.line_to(x1 - nx, y1 - ny)
        c.line_to(x0 - nx, y0 - ny)
        c.close_path()
    return f


def d_ew(c, fr=0, ang=0):
    c.save()
    c.translate(24, 24)
    c.rotate(math.radians(ang))
    c.translate(-24, -24)
    union(c, [bar(12, 24, 36, 24), head(4, 24, 180), head(44, 24, 0)])
    c.restore()


def d_ns(c, fr=0):
    d_ew(c, fr, 90)


def d_nwse(c, fr=0):
    d_ew(c, fr, 45)


def d_nesw(c, fr=0):
    d_ew(c, fr, -45)


def d_move(c, fr=0):
    union(c, [bar(24, 10, 24, 38), bar(10, 24, 38, 24), head(24, 3, -90), head(24, 45, 90),
              head(3, 24, 180), head(45, 24, 0)])


def d_no(c, fr=0):
    c.set_line_cap(cairo.LINE_CAP_ROUND)
    for col, w in ((WHITE, 4.6 + OL * 2), (RED, 4.6)):
        c.set_source_rgb(*col)
        c.set_line_width(w)
        c.arc(24, 24, 15, 0, 2 * math.pi)
        c.stroke()
        c.move_to(13.4, 13.4)
        c.line_to(34.6, 34.6)
        c.stroke()


def d_help(c, fr=0):
    union(c, [poly(ARROW)])
    c.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
    c.set_font_size(24)
    c.move_to(31, 46)
    c.text_path("?")
    c.set_source_rgb(*WHITE)
    c.set_line_width(4)
    c.set_line_join(cairo.LINE_JOIN_ROUND)
    c.stroke_preserve()
    c.set_source_rgb(*BLUE)
    c.fill()


def d_copy(c, fr=0):
    union(c, [poly(ARROW)])
    c.set_source_rgb(*WHITE)
    c.arc(38, 38, 9, 0, 2 * math.pi)
    c.fill()
    c.set_source_rgb(*GREEN)
    c.arc(38, 38, 6.5, 0, 2 * math.pi)
    c.fill()
    c.set_source_rgb(*WHITE)
    c.set_line_width(2.4)
    c.set_line_cap(cairo.LINE_CAP_ROUND)
    c.move_to(38, 34)
    c.line_to(38, 42)
    c.stroke()
    c.move_to(34, 38)
    c.line_to(42, 38)
    c.stroke()


def d_grab(c, fr=0):
    union(c, [rr(11, 9, 7, 24, 3.5), rr(18, 5, 7, 28, 3.5), rr(25, 6, 7, 27, 3.5),
              rr(32, 11, 7, 22, 3.5), rr(11, 21, 28, 21, 8), rot_rr(9, 30, 8, 15, 4, -30)])


def d_grabbing(c, fr=0):
    union(c, [rr(9, 19, 31, 22, 9), circ(15.5, 19, 5), circ(22.5, 17.5, 5), circ(29.5, 17.5, 5),
              circ(35.5, 20, 4.5), rot_rr(13, 33, 8, 14, 4, 40)])


DRAW = {
    "left_ptr": (d_arrow, (9, 5), 1),
    "pointer": (d_pointer, (18, 4), 1),
    "text": (d_text, (24, 24), 1),
    "wait": (d_wait, (24, 24), 12),
    "progress": (d_progress, (9, 5), 12),
    "crosshair": (d_cross, (24, 24), 1),
    "move": (d_move, (24, 24), 1),
    "ew-resize": (d_ew, (24, 24), 1),
    "ns-resize": (d_ns, (24, 24), 1),
    "nwse-resize": (d_nwse, (24, 24), 1),
    "nesw-resize": (d_nesw, (24, 24), 1),
    "not-allowed": (d_no, (24, 24), 1),
    "help": (d_help, (9, 5), 1),
    "copy": (d_copy, (9, 5), 1),
    "grab": (d_grab, (24, 22), 1),
    "grabbing": (d_grabbing, (24, 24), 1),
}

ALIAS = {
    "default": "left_ptr", "arrow": "left_ptr", "top_left_arrow": "left_ptr", "right_ptr": "left_ptr",
    "context-menu": "left_ptr", "alias": "copy", "dnd-copy": "copy", "dnd-link": "copy",
    "hand1": "pointer", "hand2": "pointer",
    "xterm": "text", "ibeam": "text", "vertical-text": "text",
    "watch": "wait", "left_ptr_watch": "progress", "half-busy": "progress",
    "cross": "crosshair", "cell": "crosshair", "tcross": "crosshair", "zoom-in": "crosshair", "zoom-out": "crosshair",
    "fleur": "move", "size_all": "move", "all-scroll": "move", "dnd-move": "move",
    "sb_h_double_arrow": "ew-resize", "size_hor": "ew-resize", "col-resize": "ew-resize", "h_double_arrow": "ew-resize",
    "left_side": "ew-resize", "right_side": "ew-resize", "e-resize": "ew-resize", "w-resize": "ew-resize",
    "sb_v_double_arrow": "ns-resize", "size_ver": "ns-resize", "row-resize": "ns-resize", "v_double_arrow": "ns-resize",
    "top_side": "ns-resize", "bottom_side": "ns-resize", "n-resize": "ns-resize", "s-resize": "ns-resize",
    "size_fdiag": "nwse-resize", "top_left_corner": "nwse-resize", "bottom_right_corner": "nwse-resize",
    "nw-resize": "nwse-resize", "se-resize": "nwse-resize",
    "size_bdiag": "nesw-resize", "top_right_corner": "nesw-resize", "bottom_left_corner": "nesw-resize",
    "ne-resize": "nesw-resize", "sw-resize": "nesw-resize",
    "crossed_circle": "not-allowed", "forbidden": "not-allowed", "no-drop": "not-allowed", "dnd-none": "not-allowed",
    "question_arrow": "help", "whats_this": "help",
    "openhand": "grab", "closedhand": "grabbing", "dnd-no-drop": "not-allowed",
}


def render(fn, fr, size):
    s = cairo.ImageSurface(cairo.FORMAT_ARGB32, size, size)
    c = cairo.Context(s)
    c.scale(size / 48.0, size / 48.0)
    fn(c, fr)
    return s


def main(out):
    cur = os.path.join(out, "cursors")
    tmp = os.path.join(out, "_png")
    os.makedirs(cur, exist_ok=True)
    os.makedirs(tmp, exist_ok=True)
    for nome, (fn, hot, nfr) in DRAW.items():
        righe = []
        for fr in range(nfr):
            for sz in SIZES:
                p = os.path.join(tmp, "%s-%d-%02d.png" % (nome, sz, fr))
                render(fn, fr, sz).write_to_png(p)
                hx = int(round(hot[0] * sz / 48.0))
                hy = int(round(hot[1] * sz / 48.0))
                righe.append("%d %d %d %s %s" % (sz, hx, hy, p, 80 if nfr > 1 else ""))
        cfg = os.path.join(tmp, nome + ".cfg")
        open(cfg, "w").write("\n".join(righe) + "\n")
        subprocess.check_call(["xcursorgen", cfg, os.path.join(cur, nome)])
    for a, t in ALIAS.items():
        p = os.path.join(cur, a)
        if os.path.lexists(p):
            os.remove(p)
        os.symlink(t, p)
    open(os.path.join(out, "index.theme"), "w").write("[Icon Theme]\nName=Maginat\nComment=Cursori Maginat\n")
    print("creati", len(DRAW), "cursori e", len(ALIAS), "alias")


if __name__ == "__main__":
    main(sys.argv[1])
