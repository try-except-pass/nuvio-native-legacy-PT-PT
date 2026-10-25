#!/usr/bin/env python3
"""Gera src/ajweb_pagina.h a partir de tools/ajweb.html (a pagina servida por
src/ajweb.c). Rode depois de mexer no HTML:  python3 tools/ajweb-gerar.py"""
import pathlib
raiz = pathlib.Path(__file__).resolve().parent.parent
html = (raiz / "tools" / "ajweb.html").read_text(encoding="utf-8")
linhas = []
for l in html.split("\n"):
    l = l.replace("\\", "\\\\").replace('"', '\\"')
    linhas.append('  "' + l + '\\n"')
(raiz / "src" / "ajweb_pagina.h").write_text(
    "// GERADO por tools/ajweb-gerar.py a partir de tools/ajweb.html. Nao edite.\n"
    "static const char AJWEB_PAGINA[] =\n" + "\n".join(linhas) + ";\n", encoding="utf-8")
print("src/ajweb_pagina.h:", len(html), "bytes")
