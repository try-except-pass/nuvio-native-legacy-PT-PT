"""Resumo do teste do idioma original (tests/idiomaorig_amostra.c).

Uso: python3 tests/idiomaorig_amostra.py amostra.json saida.tsv [revisao.json]
  amostra.json: [{imdb, titulo, ano, tmdb_lang, wd_langs, verdade, status}]  (status: acordo/so_tmdb/conflito)
  revisao.json: {imdb: {verdade: "xx"|null, porque}}  para os conflitos (null = ambiguo, fica de fora da nota)
Imprime cobertura e acerto de cada fonte e da decisao final, por idioma, e lista os erros.
"""
import csv, json, sys
from collections import Counter, defaultdict

amostra = {a["imdb"]: a for a in json.load(open(sys.argv[1], encoding="utf-8"))}
revisao = json.load(open(sys.argv[3], encoding="utf-8")) if len(sys.argv) > 3 else {}
linhas = list(csv.DictReader(open(sys.argv[2], encoding="utf-8"), delimiter="\t"))

MESMO = {"nb": "no", "nn": "no", "sh": "sr", "yue": "cn"}   # codigos diferentes para a mesma lingua

def igual(a, b):
    return MESMO.get(a, a) == MESMO.get(b, b)

def verdade(imdb):
    a = amostra[imdb]
    if a["status"] == "conflito":
        return (revisao.get(imdb) or {}).get("verdade"), "revisado"
    return a["verdade"], a["status"]

fontes = ["tmdb", "wd", "falado", "paisTmdb", "cinemeta", "final"]
nomes = {"tmdb": "TMDB find (o app hoje)", "wd": "Wikidata", "falado": "TMDB idioma falado",
         "paisTmdb": "TMDB pais de producao", "cinemeta": "Cinemeta pais", "final": "CADEIA (decisao)"}
tot = Counter(); resp = Counter(); certo = Counter()
porLingua = defaultdict(Counter)
erros = []
ambiguos = 0
for l in linhas:
    v, origem = verdade(l["imdb"])
    if v is None:
        ambiguos += 1
        continue
    for f in fontes:
        tot[f] += 1
        if l[f]:
            resp[f] += 1
            if igual(l[f], v):
                certo[f] += 1
    porLingua[v]["n"] += 1
    if igual(l["final"], v):
        porLingua[v]["ok"] += 1
    else:
        a = amostra[l["imdb"]]
        erros.append((l["imdb"], a["titulo"], a["ano"], v, l["final"] or "-", origem,
                      f"tmdb={l['tmdb'] or '-'} wd={l['wdTodas'] or '-'} falado={l['falado'] or '-'} "
                      f"pais={l['paisTmdb'] or '-'} cine={l['cinemeta'] or '-'}"))

n = len(linhas) - ambiguos
print(f"Titulos: {len(linhas)}  (avaliados {n}, ambiguos fora da nota {ambiguos})")
print(f"{'fonte':28} {'respondeu':>10} {'acertou':>10} {'acerto/resp':>12}")
for f in fontes:
    r = resp[f]; c = certo[f]
    print(f"{nomes[f]:28} {100*r/n:9.1f}% {100*c/n:9.1f}% {100*c/max(r,1):11.1f}%")
ms = defaultdict(list)
for l in linhas:
    for k in ("ms_tmdb", "ms_wd", "ms_det", "ms_cine"):
        ms[k].append(int(l[k]))
print("tempo (ms) mediana/p95/max:", {k: (sorted(v)[len(v)//2], sorted(v)[len(v)*95//100], max(v)) for k, v in ms.items()})
print("\nAcerto da cadeia por idioma (verdade):")
for lang, c in sorted(porLingua.items(), key=lambda x: -x[1]["n"]):
    print(f"  {lang}: {c['ok']}/{c['n']}")
print(f"\nErros da cadeia: {len(erros)}")
for e in erros:
    print("  " + " | ".join(str(x) for x in e))
