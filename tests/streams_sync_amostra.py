#!/usr/bin/env python3
# Dataset do item 2 (HANDOFF §4): a escolha de stream de HOJE (so audio, LOGIC §2.2) contra uma regra que mira
# legenda SINCRONIZADA. Sem baixar video nem legenda: so metadados publicos (TMDB, Torrentio, OpenSubtitles v3).
# A sincronia e ESTIMADA pelo nome do release (grupo/fonte) do stream contra o da legenda: e um proxy, nao a prova.
# Uso: python3 tests/streams_sync_amostra.py [saida.tsv]     (chave do TMDB: local.properties)
import collections, concurrent.futures as cf, json, pathlib, random, re, statistics, sys, urllib.request

N_EN, N_FORA, N_ANIME, TOPO = 30, 30, 10, 30   # titulos por grupo; streams olhados, na ordem do addon
FLAGS = {"en": "🇬🇧🇺🇸", "ja": "🇯🇵", "ko": "🇰🇷", "fr": "🇫🇷", "de": "🇩🇪", "es": "🇪🇸🇲🇽", "it": "🇮🇹", "ru": "🇷🇺",
         "zh": "🇨🇳🇹🇼🇭🇰", "cn": "🇭🇰", "pt": "🇵🇹🇧🇷", "hi": "🇮🇳", "sv": "🇸🇪", "da": "🇩🇰", "no": "🇳🇴", "fi": "🇫🇮",
         "tr": "🇹🇷", "pl": "🇵🇱", "th": "🇹🇭", "nl": "🇳🇱", "fa": "🇮🇷", "ar": "🇸🇦", "he": "🇮🇱", "el": "🇬🇷"}
SRC = [("remux", "remux"), ("bluray", "bluray"), ("blu-ray", "bluray"), ("bdrip", "bluray"), ("brrip", "bluray"),
       ("web-dl", "webdl"), ("webdl", "webdl"), ("webrip", "webrip"), ("hdtv", "hdtv"), ("dvdrip", "dvd"),
       ("hdrip", "hdrip")]
RUIDO = {"dl", "rip", "ray", "hd", "sd", "ts", "cam", "mkv", "avi", "mp4", "srt", "eng", "por", "pob"}


def baixar(url, t=25):
    for _ in range(2):
        try:
            return json.load(urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "nuvio-amostra"}), timeout=t))
        except Exception:
            pass
    return None


def chave_tmdb():
    for l in pathlib.Path("local.properties").read_text().splitlines():
        if "tmdb" in l.lower() and "=" in l:
            return l.split("=", 1)[1].strip()
    sys.exit("sem chave do TMDB em local.properties")


def release(nome):
    """(fonte, grupo) de um nome de arquivo/legenda; '' quando nao da para saber."""
    n = re.sub(r"\.(mkv|mp4|avi|srt|ass|sub)$", "", nome.lower().strip())
    fonte = next((v for k, v in SRC if k in n), "")
    g = ""
    for pat in (r"-([a-z0-9]+)$", r"^\[([^\]]+)\]", r"\bby[. ]([a-z0-9]+)$"):
        m = re.search(pat, n)
        if m and m.group(1) not in RUIDO:
            g = m.group(1)
            break
    return fonte, g


def casa(stream, subs):
    """'forte' = mesmo grupo; 'fraca' = so a mesma fonte; '' = nada."""
    fs, gs = release(stream)
    forte = fraca = False
    for s in subs:
        f, g = release(s)
        forte |= bool(gs and g == gs)
        fraca |= bool(fs and f == fs)
    return "forte" if forte else "fraca" if fraca else ""


def declara(stream, lang):
    fl = FLAGS.get(lang, "")
    linhas = stream.get("title", "").split("\n")
    return bool(fl) and any(c in linhas[-1] for c in fl)


def titulo(tm, key, imdb_de_tmdb=None):
    d = baixar(f"https://api.themoviedb.org/3/movie/{tm['id']}?api_key={key}")
    return {"imdb": (d or {}).get("imdb_id") or "", "lang": tm["original_language"], "nome": tm.get("title", "?")}


def amostra(key):
    random.seed(7)
    pool = []
    for p in range(1, 9):
        pool += (baixar(f"https://api.themoviedb.org/3/movie/top_rated?api_key={key}&page={p}") or {}).get("results", [])
    anime = (baixar(f"https://api.themoviedb.org/3/discover/movie?api_key={key}&with_genres=16&with_original_language=ja&sort_by=vote_count.desc") or {}).get("results", [])
    en = [t for t in pool if t["original_language"] == "en"]
    fora = [t for t in pool if t["original_language"] not in ("en", "ja")]
    esc = [("en", t) for t in random.sample(en, N_EN)] + [("fora", t) for t in random.sample(fora, N_FORA)] + \
          [("anime", t) for t in anime[:N_ANIME]]
    with cf.ThreadPoolExecutor(4) as ex:
        res = list(ex.map(lambda gt: (gt[0], titulo(gt[1], key)), esc))
    return [(g, t) for g, t in res if t["imdb"]]


def analisa(g, t):
    st = (baixar(f"https://torrentio.strem.fun/stream/movie/{t['imdb']}.json", 40) or {}).get("streams", [])[:TOPO]
    sub = (baixar(f"https://opensubtitles-v3.strem.io/subtitles/movie/{t['imdb']}.json") or {}).get("subtitles", [])
    por = [s.get("subtitleFileName", "") for s in sub if s.get("lang") == "por"]
    nome = lambda s: s.get("behaviorHints", {}).get("filename") or s.get("title", "").split("\n")[0]
    if not st:
        return None
    ori = [i for i, s in enumerate(st) if declara(s, t["lang"])]
    hoje = ori[0] if ori else 0                       # passada 1 (declara o original) ou passada 2 (o topo)
    cand = ori or list(range(len(st)))                # a regra nova percorre as mesmas, na mesma ordem
    nova = next((i for i in cand if casa(nome(st[i]), por) == "forte"), None)
    return {"grupo": g, "imdb": t["imdb"], "nome": t["nome"][:28], "orig": t["lang"], "streams": len(st),
            "declaram": len(ori), "por": len(por), "hoje": hoje, "hoje_casa": casa(nome(st[hoje]), por) if por else "",
            "nova": "" if nova is None else nova, "nova_casa": "forte" if nova is not None else ""}


def main():
    key = chave_tmdb()
    alvo = amostra(key)
    print(f"{len(alvo)} titulos com IMDb", file=sys.stderr)
    with cf.ThreadPoolExecutor(3) as ex:
        linhas = [r for r in ex.map(lambda gt: analisa(*gt), alvo) if r]
    cols = list(linhas[0])
    out = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path("build/streams_sync_amostra.tsv")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join("\t".join(str(r[c]) for c in cols) for r in [dict(zip(cols, cols))] + linhas))
    for nome, sel in (("TODOS", linhas),) + tuple((g, [r for r in linhas if r["grupo"] == g]) for g in ("en", "fora", "anime")):
        n = len(sel)
        if not n:
            continue
        com_por = [r for r in sel if r["por"]]
        hoje_f = [r for r in com_por if r["hoje_casa"] == "forte"]
        hoje_w = [r for r in com_por if r["hoje_casa"] == "fraca"]
        melhora = [r for r in com_por if r["nova"] != "" and r["hoje_casa"] != "forte"]
        deslocas = [r["nova"] - r["hoje"] for r in melhora]
        print(f"\n[{nome}] titulos={n} | streams/titulo med={statistics.median(r['streams'] for r in sel)} | "
              f"declaram o original: {sum(1 for r in sel if r['declaram'])}/{n}")
        print(f"  com legenda PT-PT: {len(com_por)}/{n} ({100*len(com_por)//n}%)")
        print(f"  escolha de hoje: release casa FORTE {len(hoje_f)}, so fonte {len(hoje_w)}, nada {len(com_por)-len(hoje_f)-len(hoje_w)}  (de {len(com_por)})")
        print(f"  regra nova acha stream com sincronia estimada FORTE onde hoje nao: {len(melhora)}/{len(com_por)}"
              + (f" | posicao: +{statistics.median(deslocas):g} (mediana), +{max(deslocas)} (max)" if deslocas else ""))


main()
