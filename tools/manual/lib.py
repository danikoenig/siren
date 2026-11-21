"""Gemeinsamer Manual-Generator (ReportLab) fuer alle Plugins von Dani Koenig.

Jedes Plugin liefert ein CONTENT-Dict (siehe content_*.py). Aufruf aus dem Plugin-Repo:
    python3 tools/manual/make_manual.py <version> <screenshot> <ausgabeordner>
"""
import sys
from reportlab.lib.pagesizes import A4
from reportlab.lib import colors
from reportlab.lib.units import mm
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, Table,
                                TableStyle, Image, PageBreak, KeepTogether)
from reportlab.platypus.doctemplate import NextPageTemplate

DARK = colors.HexColor("#17181c")
CARD = colors.HexColor("#f3f3f5")
TEXT = colors.HexColor("#1d1f24")
DIM = colors.HexColor("#6b707a")
LINE = colors.HexColor("#dddde2")
GREY = colors.HexColor("#9a9fa8")

H1 = ParagraphStyle("h1", fontName="Helvetica-Bold", fontSize=17, leading=21, textColor=TEXT, spaceBefore=6, spaceAfter=8, keepWithNext=1)
H2 = ParagraphStyle("h2", fontName="Helvetica-Bold", fontSize=12.5, leading=16, textColor=TEXT, spaceBefore=12, spaceAfter=6, keepWithNext=1)
BODY = ParagraphStyle("body", fontName="Helvetica", fontSize=10, leading=14.4, textColor=TEXT, spaceAfter=6)
LEAD = ParagraphStyle("lead", parent=BODY, fontSize=10.8, leading=15.8, spaceAfter=7)
SMALL = ParagraphStyle("small", parent=BODY, fontSize=9, leading=12.8, textColor=DIM)
CELL = ParagraphStyle("cell", parent=BODY, fontSize=9.2, leading=12.6, spaceAfter=0)
CELLB = ParagraphStyle("cellb", parent=CELL, fontName="Helvetica-Bold")
BULLET = ParagraphStyle("bullet", parent=BODY, leftIndent=12, bulletIndent=2, spaceAfter=3)

LABELS = {
    "en": {"controls": "Controls", "recipes": "Recipes", "tips": "Good to know", "install": "Install", "page": "Page",
           "manual": "Manual"},
    "de": {"controls": "Bedienelemente", "recipes": "Rezepte", "tips": "Gut zu wissen", "install": "Installation", "page": "Seite",
           "manual": "Manual"},
}


def hexc(h):
    return colors.HexColor(h)


def kv_table(rows, fw, col, keyw=38 * mm):
    data = [[Paragraph(k, CELLB), Paragraph(v, CELL)] for k, v in rows]
    tb = Table(data, colWidths=[keyw, fw - keyw])
    tb.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"),
                            ("TOPPADDING", (0, 0), (-1, -1), 4.5), ("BOTTOMPADDING", (0, 0), (-1, -1), 4.5),
                            ("LINEBELOW", (0, 0), (-1, -1), 0.4, LINE),
                            ("LINEBEFORE", (0, 0), (0, -1), 2, col)]))
    return tb


def section_title(title, col, fw):
    tb = Table([[Paragraph('<font color="white"><b>%s</b></font>' % title, CELL)]], colWidths=[fw])
    tb.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, -1), col),
                            ("TOPPADDING", (0, 0), (-1, -1), 4.5), ("BOTTOMPADDING", (0, 0), (-1, -1), 4.5),
                            ("LEFTPADDING", (0, 0), (-1, -1), 8)]))
    return tb


def grid_table(head, rows, fw, col, firstw=None):
    n = len(head)
    firstw = firstw or 34 * mm
    cw = [firstw] + [(fw - firstw) / (n - 1)] * (n - 1)
    data = [[Paragraph('<font color="white"><b>%s</b></font>' % h, CELL) for h in head]]
    for r in rows:
        data.append([Paragraph(c, CELLB if i == 0 else CELL) for i, c in enumerate(r)])
    tb = Table(data, colWidths=cw, repeatRows=1)
    st = [("BACKGROUND", (0, 0), (-1, 0), col), ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
          ("TOPPADDING", (0, 0), (-1, -1), 4), ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
          ("LINEBELOW", (0, 0), (-1, -1), 0.4, LINE)]
    for i in range(2, len(data), 2):
        st.append(("BACKGROUND", (0, i), (-1, i), CARD))
    tb.setStyle(TableStyle(st))
    return tb


def build(C, lang, version, shot, out):
    t = C[lang]
    L = LABELS[lang]
    accent = hexc(C["accent"])
    amber = hexc(C.get("accent2", C["accent"]))
    W, H = A4
    M = 20 * mm
    BAND = 78 * mm       # Kopfband der ersten Seite (grosszuegig)
    SLIM = 14 * mm       # Kopfband der Folgeseiten
    name = C["title"]
    url = "github.com/danikoenig/" + C["repo"]

    def footer(c, d):
        c.setFillColor(DIM)
        c.setFont("Helvetica", 8)
        c.drawString(M, 11 * mm, "%s v%s  \xb7  Dani Koenig" % (C["title"].title() if C["title"].isupper() else C["title"], version))
        c.drawRightString(W - M, 11 * mm, "%s %d" % (L["page"], d.page))

    def first_page(c, d):
        c.saveState()
        c.setFillColor(DARK)
        c.rect(0, H - BAND, W, BAND, stroke=0, fill=1)
        # kleines Logo: vier Balken in der Plugin-Farbe
        bx, base = M, H - 22 * mm
        for i, hh in enumerate((5.0, 9.0, 3.4, 7.0)):
            c.setFillColor(accent if i % 2 == 0 else amber)
            c.roundRect(bx + i * 2.4 * mm, base, 1.6 * mm, hh * mm, 0.5 * mm, stroke=0, fill=1)
        # Titel
        c.setFillColor(accent)
        c.setFont("Helvetica-Bold", 38)
        c.drawString(M, H - 38 * mm, name)
        # kurze Akzentlinie
        c.setFillColor(amber)
        c.rect(M, H - 44.5 * mm, 22 * mm, 1.1 * mm, stroke=0, fill=1)
        # Untertitel und Art
        c.setFillColor(colors.white)
        c.setFont("Helvetica", 13.5)
        c.drawString(M, H - 54 * mm, t["subtitle"])
        c.setFillColor(GREY)
        c.setFont("Helvetica", 9.5)
        c.drawString(M, H - 61.5 * mm, t["kind"])
        # Fusszeile im Band: links Autor, rechts Version und Link, deutlich getrennt
        c.setStrokeColor(colors.HexColor("#2c2d33"))
        c.setLineWidth(0.6)
        c.line(M, H - 67.5 * mm, W - M, H - 67.5 * mm)
        c.setFillColor(GREY)
        c.setFont("Helvetica", 9)
        c.drawString(M, H - 73 * mm, t["by"])
        c.drawRightString(W - M, H - 73 * mm, "v%s   \xb7   %s" % (version, url))
        c.setFillColor(amber)
        c.rect(0, H - BAND, W, 1.2 * mm, stroke=0, fill=1)
        footer(c, d)
        c.restoreState()

    def later_page(c, d):
        c.saveState()
        c.setFillColor(DARK)
        c.rect(0, H - SLIM, W, SLIM, stroke=0, fill=1)
        c.setFillColor(accent)
        c.setFont("Helvetica-Bold", 11)
        c.drawString(M, H - 9 * mm, name)
        c.setFillColor(GREY)
        c.setFont("Helvetica", 8.5)
        c.drawRightString(W - M, H - 9 * mm, "%s v%s" % (L["manual"], version))
        footer(c, d)
        c.restoreState()

    doc = BaseDocTemplate(out, pagesize=A4, leftMargin=M, rightMargin=M, topMargin=22 * mm, bottomMargin=20 * mm,
                          title="%s Manual" % name.title(), author="Dani Koenig")
    fw = W - 2 * M
    doc.addPageTemplates([
        PageTemplate("first", [Frame(M, 20 * mm, fw, H - BAND - 20 * mm - 10 * mm, id="f1", leftPadding=0, rightPadding=0)], onPage=first_page),
        PageTemplate("later", [Frame(M, 20 * mm, fw, H - SLIM - 20 * mm - 8 * mm, id="f2", leftPadding=0, rightPadding=0)], onPage=later_page),
    ])

    s = [NextPageTemplate("later")]
    s.append(Paragraph(t["intro"], LEAD))
    if t.get("note"):
        s.append(Paragraph(t["note"], SMALL))
    s.append(Spacer(1, 6))

    img = Image(shot)
    ratio = img.imageHeight / float(img.imageWidth)
    maxh = 118 * mm
    wdt = fw
    if wdt * ratio > maxh:
        wdt = maxh / ratio
    img.drawWidth, img.drawHeight = wdt, wdt * ratio
    img.hAlign = "CENTER"
    s.append(img)
    s.append(Spacer(1, 8))

    for title, rows in t.get("blocks", []):
        s.append(Paragraph(title, H2))
        s.append(kv_table(rows, fw, accent))
    s.append(Paragraph(t["quick_title"], H2))
    s.append(kv_table(t["quick"], fw, accent))
    if t.get("first"):
        s.append(Spacer(1, 4))
        s.append(Paragraph(t["first"], BODY))

    s.append(Spacer(1, 6))
    s.append(Paragraph(L["controls"], H1))
    for col, title, rows in t["sections"]:
        c = hexc(col)
        s.append(KeepTogether([section_title(title, c, fw), Spacer(1, 3), kv_table(rows, fw, c), Spacer(1, 9)]))

    for ex in t.get("extras", []):
        s.append(Paragraph(ex["title"], H2))
        if ex.get("text"):
            s.append(Paragraph(ex["text"], BODY))
        if ex.get("grid"):
            s.append(grid_table(ex["grid"]["head"], ex["grid"]["rows"], fw, accent, ex["grid"].get("firstw")))
        if ex.get("rows"):
            s.append(kv_table(ex["rows"], fw, accent))

    s.append(Paragraph(L["recipes"], H2))
    s.append(kv_table(t["recipes"], fw, amber))
    s.append(Paragraph(L["tips"], H2))
    for r in t["tips"]:
        s.append(Paragraph(r, BULLET, bulletText="•"))
    s.append(Paragraph(L["install"], H2))
    s.append(Paragraph(t["install"], BODY))
    doc.build(s)


def main(C):
    version, shot, outdir = sys.argv[1], sys.argv[2], sys.argv[3]
    for lang in ("en", "de"):
        build(C, lang, version, shot, "%s/%s-Manual-%s.pdf" % (outdir, C["file"], lang.upper()))
    print("ok")


def install_text(lang, file, repo, what):
    if lang == "en":
        return ("Download <b>%s-Installer.pkg</b> from github.com/danikoenig/%s/releases and double-click it. "
                "macOS blocks it the first time: System Settings &gt; Privacy &amp; Security &gt; <i>Open Anyway</i>. "
                "The installer puts %s into the VST3, Components (Audio Unit) and CLAP folders and also installs the standalone app where there is one. "
                "Restart the DAW and rescan the plugins. To remove everything, use %s-Uninstaller.pkg. "
                "Requirements: Apple Silicon Mac (M1 or newer), macOS 11 or newer." % (file, repo, what, file))
    return ("Lade <b>%s-Installer.pkg</b> von github.com/danikoenig/%s/releases und doppelklicke es. "
            "macOS blockiert es beim ersten Mal: Systemeinstellungen &gt; Datenschutz &amp; Sicherheit &gt; <i>Dennoch öffnen</i>. "
            "Der Installer legt %s in die Ordner VST3, Components (Audio Unit) und CLAP und installiert, wo es sie gibt, auch die Standalone-App. "
            "DAW neu starten und Plugins neu scannen. Entfernen: %s-Uninstaller.pkg. "
            "Voraussetzungen: Mac mit Apple Silicon (M1 oder neuer), macOS 11 oder neuer." % (file, repo, what, file))
