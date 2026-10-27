"""Resumo do teste de legendas PT-PT (tests/legendas_amostra.c).

Uso: python3 tests/legendas_amostra.py <dir_saida> <amostra.json>
Para cada titulo com legenda PT escolhida e referencia inglesa do mesmo release, alinha os inicios dos
blocos: procura o deslocamento (e a velocidade, para releases de 23,976/24/25 fps) que mais casa blocos
dentro de 0,4 s. Classes:
  em_sincronia      velocidade 1, |deslocamento| <= 0,3 s
  desvio_constante  velocidade 1, deslocamento maior (o AutoSync corrige)
  velocidade        so casa com outra velocidade (release de outro fps)
  inconclusivo      poucos blocos casam (segmentacao muito diferente) — sem veredito
"""
import csv, json, sys
from bisect import bisect_left
from collections import Counter
from pathlib import Path

D = Path(sys.argv[1])
amostra = {a["imdb"]: a for a in json.load(open(sys.argv[2], encoding="utf-8"))}
TOL = 0.4
VELOCIDADES = [1.0, 25 / 23.976, 23.976 / 25, 24 / 23.976, 23.976 / 24, 25 / 24, 24 / 25]

def inicios(p):
    return [float(l.split()[0]) for l in p.read_text().splitlines() if l.strip()]

def casados(pt, en, off, vel):
    n = 0
    for t in pt:
        x = t * vel + off
        i = bisect_left(en, x)
        if (i < len(en) and abs(en[i] - x) <= TOL) or (i > 0 and abs(en[i - 1] - x) <= TOL):
            n += 1
    return n

def alinhar(pt, en):
    melhor = (0, 0.0, 1.0)
    for vel in VELOCIDADES:
        # grosso: passo de 0,5 s em +-120 s; fino: 0,05 s em volta do melhor
        cand = max(((casados(pt, en, o / 2, vel), o / 2) for o in range(-240, 241)), key=lambda x: x[0])
        fino = max(((casados(pt, en, cand[1] + d / 20, vel), cand[1] + d / 20) for d in range(-10, 11)), key=lambda x: x[0])
        if fino[0] > melhor[0]:
            melhor = (fino[0], fino[1], vel)
    return melhor

linhas = list(csv.DictReader(open(D / "resumo.tsv", encoding="utf-8"), delimiter="\t"))
classes = Counter()
detalhes = []
sem_pt = sem_ref = falha_dl = 0
for l in linhas:
    a = amostra.get(l["imdb"], {})
    nome = f"{a.get('titulo','?')} ({a.get('ano','')}, {a.get('tmdb_lang','')})"
    if l["estado"] != "ok":
        classes["addon_sem_resposta"] += 1; detalhes.append((l["imdb"], nome, "addon_sem_resposta", "")); continue
    if int(l["n_pt"]) == 0:
        sem_pt += 1; classes["sem_pt_pt"] += 1
        detalhes.append((l["imdb"], nome, "sem_pt_pt", f"pt-br disponiveis={l['n_ptbr']}")); continue
    if int(l["cues_pt"]) <= 0:
        falha_dl += 1; classes["falha_download_ou_leitura"] += 1
        detalhes.append((l["imdb"], nome, "falha_download_ou_leitura", l["escolhida"])); continue
    if int(l["cues_en"]) <= 0 or not (D / f"{l['imdb']}.en.txt").exists():
        sem_ref += 1; classes["sem_referencia_en"] += 1
        detalhes.append((l["imdb"], nome, "sem_referencia_en", l["escolhida"])); continue
    pt = sorted(inicios(D / f"{l['imdb']}.pt.txt")); en = sorted(inicios(D / f"{l['imdb']}.en.txt"))
    n, off, vel = alinhar(pt, en)
    frac = n / max(len(pt), 1)
    if frac < 0.35:
        c = "inconclusivo"
    elif abs(vel - 1.0) > 1e-6:
        c = "velocidade"
    elif abs(off) <= 0.3:
        c = "em_sincronia"
    else:
        c = "desvio_constante"
    classes[c] += 1
    mesmo = int(l["afinidade"])
    detalhes.append((l["imdb"], nome, c, f"casados={frac:.0%} desloc={off:+.2f}s vel={vel:.4f} afinidade_release={mesmo}"))

N = len(linhas)
print(f"Titulos: {N}")
for c, n in classes.most_common():
    print(f"  {c:28} {n:4}  ({100*n/N:.0f}%)")
com_pt = N - sem_pt - classes["addon_sem_resposta"]
print(f"\nCom PT-PT disponivel: {com_pt}/{N}")
print("PT-BR disponivel nos que nao tem PT-PT:",
      sum(1 for l in linhas if l["estado"] == "ok" and int(l["n_pt"]) == 0 and int(l["n_ptbr"]) > 0))
print("\nDetalhe:")
for d in sorted(detalhes, key=lambda x: x[2]):
    print("  " + " | ".join(d))
