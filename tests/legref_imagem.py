#!/usr/bin/env python3
"""MKVs sinteticos de legenda de IMAGEM para tests/legref.c: PGS (alternando MOSTRA/LIMPA), PGS com blocos zlib, PGS
com escorregoes de fase (dois MOSTRA ou dois LIMPA seguidos) e VOBSUB (o Cues traz a CueDuration).

  python3 tests/legref_imagem.py DIR      # grava pgs.mkv pgs_zl.mkv pgs_slip.mkv vob.mkv vob_zl.mkv e imagem.txt (os tempos)
"""
import os, struct, sys, zlib

N = 200            # falas
PASSO = 4.0        # s entre inicios
DUR = 2.0          # s de cada fala


def tam(n):        # tamanho EBML de 8 bytes (sempre valido, o parser aceita)
    return b"\x01" + n.to_bytes(7, "big")


def el(i, corpo):
    return bytes.fromhex(i) + tam(len(corpo)) + corpo


def uint(i, v):
    return el(i, v.to_bytes(max(1, (v.bit_length() + 7) // 8), "big"))


def pcs(objetos):  # segmento PCS: tipo 0x16, tamanho 11, o byte 10 do corpo = numero de objetos
    corpo = bytes(10) + bytes([objetos])
    return b"\x16" + struct.pack(">H", len(corpo)) + corpo


def faixa(codec, zl):
    # o que o mkvmerge antigo grava: 62 40 83 50 34 80 (ContentEncoding > ContentCompression vazio = zlib)
    enc = bytes.fromhex("6D80") + b"\x86" + bytes.fromhex("624083503480") if zl else b""
    return el("AE", uint("D7", 1) + uint("73C5", 1) + uint("83", 17) + el("86", codec.encode()) + el("22B59C", b"eng") + enc)


def montar(arq, codec, zl, eventos, cue_dur):
    """eventos: lista de (tempo_s, payload) na ordem dos pontos do Cues."""
    por_cluster = 8   # o relativo do bloco e int16 em ms: 8 eventos de ate 4 s cabem
    clusters, cues = [], []
    pos = 0
    corpos = []
    for k in range(0, len(eventos), por_cluster):
        grupo = eventos[k:k + por_cluster]
        base = int(grupo[0][0] * 1000)
        dados = uint("E7", base)
        blocos = []
        for t, pl in grupo:
            if zl: pl = zlib.compress(pl)
            rel_ms = int(t * 1000) - base
            bloco = el("A3", b"\x81" + struct.pack(">h", rel_ms) + b"\x80" + pl)
            blocos.append((t, len(dados), bloco))
            dados += bloco
        corpos.append((base, blocos, dados))
    # layout: EBML, Segment{ SeekHead, Info, Tracks, Clusters..., Cues }
    ebml = el("1A45DFA3", el("4282", b"matroska"))
    info = el("1549A966", uint("2AD7B1", 1000000) + el("4489", struct.pack(">d", (eventos[-1][0] + 5) * 1000)))
    tracks = el("1654AE6B", faixa(codec, zl))
    seek_tam = len(el("114D9B74", b"".join(el("4DBB", el("53AB", bytes.fromhex(i)) + el("53AC", b"\x00" * 4)) for i in ("1549A966", "1654AE6B", "1C53BB6B"))))
    o = seek_tam
    pos_info = o; o += len(info)
    pos_tracks = o; o += len(tracks)
    cl_bytes = b""
    for base, blocos, dados in corpos:
        cl = el("1F43B675", dados)
        for t, rel, _ in blocos:
            cue = uint("B3", int(t * 1000)) + el("B7", uint("F7", 1) + uint("F1", o) + uint("F0", rel) + (uint("B2", int(DUR * 1000)) if cue_dur else b""))
            cues.append(el("BB", cue))
        cl_bytes += cl; o += len(cl)
    pos_cues = o
    cues_el = el("1C53BB6B", b"".join(cues))
    seek = el("114D9B74", b"".join(el("4DBB", el("53AB", bytes.fromhex(i)) + el("53AC", p.to_bytes(4, "big")))
                                  for i, p in (("1549A966", pos_info), ("1654AE6B", pos_tracks), ("1C53BB6B", pos_cues))))
    assert len(seek) == seek_tam
    with open(arq, "wb") as f:
        f.write(ebml + el("18538067", seek + info + tracks + cl_bytes + cues_el))


def pgs_eventos(slips):
    ev, certos = [], []
    for k in range(N):
        t = 10 + k * PASSO
        certos.append((t, t + DUR))
        ev.append((t, pcs(1)))
        ev.append((t + DUR, pcs(0)))
        if k in slips.get("limpa", ()): ev.append((t + DUR + 0.5, pcs(0)))   # LIMPA em dobro: vira a fase
        if k in slips.get("mostra", ()): ev.append((t - 0.5, pcs(1)))   # MOSTRA em dobro (antes do certo)
    ev.sort(key=lambda e: e[0])
    return ev, certos


def main(d):
    os.makedirs(d, exist_ok=True)
    ev, certos = pgs_eventos({})
    montar(f"{d}/pgs.mkv", "S_HDMV/PGS", False, ev, False)
    montar(f"{d}/pgs_zl.mkv", "S_HDMV/PGS", True, ev, False)
    ev, _ = pgs_eventos({"limpa": {30, 90, 150}, "mostra": {60, 120}})
    montar(f"{d}/pgs_slip.mkv", "S_HDMV/PGS", True, ev, False)
    montar(f"{d}/pgs_slip_pl.mkv", "S_HDMV/PGS", False, ev, False)   # os escorregoes sozinhos, sem zlib
    vob = [(10 + k * PASSO, bytes(64)) for k in range(N)]
    montar(f"{d}/vob.mkv", "S_VOBSUB", False, vob, True)
    montar(f"{d}/vob_zl.mkv", "S_VOBSUB", True, vob, True)
    with open(f"{d}/imagem.txt", "w") as f:
        f.write("%d\n" % N)


if __name__ == "__main__":
    main(sys.argv[1])
