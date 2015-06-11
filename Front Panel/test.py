from math import ceil, floor, pi, sin

from reportlab.lib.pagesizes import landscape, letter
from reportlab.lib.units import inch, mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas


def saw_path(c, bbox):
    xmin = 0.5
    xmax = 2.5

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    curve = c.beginPath()
    x = xmin
    curve.moveTo(sx(x), sy(x % 1 * 2 - 1))
    while x < xmax:
        if x == int(x):
            curve.lineTo(sx(x), sy(+1))
            curve.lineTo(sx(x), sy(-1))
            x = x + 1
        else:
            x = ceil(x)
        if x > xmax:
            x = xmax
            curve.lineTo(sx(x), sy(x % 1 * 2 - 1))
    return curve


def square_path(c, bbox):
    xmin = -0.25
    xmax = 1.75

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    curve = c.beginPath()
    x = xmin
    curve.moveTo(sx(x), sy(+1 if 2 * x % 2 < 1 else -1))
    while x < xmax:
        if 2 * x % 2 == 0:
            curve.lineTo(sx(x), sy(-1))
            curve.lineTo(sx(x), sy(+1))
            x += 1/2
        elif 2 * x % 2 == 1:
            curve.lineTo(sx(x), sy(+1))
            curve.lineTo(sx(x), sy(-1))
            x += 1/2
        else:
            x = ceil(2 * x) / 2
        if x > xmax:
            x = xmax
            curve.lineTo(sx(x), sy(+1 if 2 * x % 2 < 1 else -1))
            
    return curve


def tri_path(c, bbox):
    xmin = 0.25
    xmax = 2.25

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    curve = c.beginPath()
    x = xmin
    curve.moveTo(sx(x), sy((2*x % 1 if 2 * x % 2 < 1 else 1 - 2*x % 1) * 2 - 1))
    while x < xmax:
        if 2 * x % 2 == 0:
            curve.lineTo(sx(x), sy(-1))
            x += 1/2
        elif 2 * x % 2 == 1:
            curve.lineTo(sx(x), sy(+1))
            x += 1/2
        else:
            x = ceil(2 * x) / 2
        if x > xmax:
            x = xmax
            xx = 2 * x
            curve.lineTo(sx(x),
                         sy((xx % 1 if xx % 2 < 1 else 1 - xx % 1) * 2 - 1))
            # curve.lineTo(sx(x), sy(+1 if 2 * x % 2 < 1 else -1))
            
    return curve


def sin_path(c, bbox):
    xmin = 0
    xmax = 2

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    basis = ((0, 0), (0.5251 / pi / 2, 0.5251), (1.005 / pi / 2, 1), (1 / 4, 1))
    segments = range(floor(xmin * 4), ceil(xmax * 4))

    curve = c.beginPath()
    curve.moveTo(sx(xmin), sy(sin(xmin*2*pi)))
    for s in segments:
        q = s % 4
        x0 = s / 4
        if q == 0:
            p1, p2, p3 = basis[1:]
        elif q == 1:
            p1 = (1/4 - basis[2][0], basis[2][1])
            p2 = (1/4 - basis[1][0], basis[1][1])
            p3 = (1/4 - basis[0][0], basis[0][1])
        elif q == 2:
            p1 = (basis[1][0], -basis[1][1])
            p2 = (basis[2][0], -basis[2][1])
            p3 = (basis[3][0], -basis[3][1])
            pass
        else:
            assert q == 3
            p1 = (1/4 - basis[2][0], -basis[2][1])
            p2 = (1/4 - basis[1][0], -basis[1][1])
            p3 = (1/4 - basis[0][0], -basis[0][1])
        curve.curveTo(sx(x0 + p1[0]), sy(p1[1]),
                      sx(x0 + p2[0]), sy(p2[1]),
                      sx(x0 + p3[0]), sy(p3[1]))
    return curve


def rand_noise(x, b):
    def bigrand(n): return bigrand(n - 1) * 48271 % 0x7fffffff if n else b
    def random(n): return bigrand(n) % 23 / 23
    a = random(int(x))
    b = random(int(x) + 1)
    return (a + x%1 * (b - a)) * 2 - 1


def rand_path(c, bbox):
    xmin = 0
    xmax = 6

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    curve = c.beginPath()
    curve.moveTo(sx(xmin), sy(rand_noise(0, 9)))
    for x in range(xmin + 1, xmax + 1):
        curve.lineTo(sx(x), sy(rand_noise(x, 9)))
    return curve


# sample_hold = WaveformLabel('Sample and Hold',
#                             lambda x: rand_noise(int(3 * x), 5))

def snh_path(c, bbox):
    xmin = 0
    xmax = 6

    def sx(x):
        return bbox[0] + (bbox[2] - bbox[0]) * (x - xmin) / (xmax - xmin)
    def sy(y):
        return (bbox[1] + bbox[3] + y * (bbox[3] - bbox[1])) / 2

    curve = c.beginPath()
    curve.moveTo(sx(xmin), sy(rand_noise(0, 5)))
    for x in range(xmin, xmax):
        curve.lineTo(sx(x), sy(rand_noise(x, 5)))
        curve.lineTo(sx(x + 1), sy(rand_noise(x, 5)))
    return curve


pagesize = letter
c = canvas.Canvas('Synth Panel.pdf', pagesize=pagesize)
c.setFillColorRGB(0, 1, 0)
c.setFillColorRGB(1, 0.7, 0.2)
size = (120*mm, 30*mm)
margin = 10*mm

path_funcs = (saw_path, square_path, tri_path, sin_path, rand_path, snh_path)
n = len(path_funcs)
h = n * size[1] + (n - 1) * margin
x = (pagesize[0] - size[0]) / 2
for i, path_func in enumerate(path_funcs):
    y = (pagesize[1] + h)/2 - i * (size[1] + margin) - size[1]
    c.rect(x, y, *size, stroke=0, fill=1)
    bbox = (x, y, x + size[0], y + size[1])
    c.drawPath(path_func(c, bbox))

c.showPage()
c.save()
