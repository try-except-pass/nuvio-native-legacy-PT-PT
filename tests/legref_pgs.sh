#!/bin/bash
# Referencia do AutoSync a partir de uma faixa PGS (so tempos, sem texto).
# MKV sintetico com Cues em todos os blocos: mostra/limpa alternando.
set -euo pipefail
cd "$(dirname "$0")/.."
D=$(mktemp -d "${TMPDIR:-/tmp}/nv-legref-pgs.XXXXXX"); trap 'rm -rf "$D"' EXIT
python3 - "$D" <<'PY'
import sys, pathlib
p = pathlib.Path(sys.argv[1])
def el(i, b): return i.to_bytes((i.bit_length() + 7) // 8, 'big') + (0x10000000 | len(b)).to_bytes(4, 'big') + b
def u(i, n): return el(i, n.to_bytes(4, 'big'))
def seg(t, d): return bytes([t]) + len(d).to_bytes(2, 'big') + d
def pcs(n):
    d = (1920).to_bytes(2, 'big') + (1080).to_bytes(2, 'big') + b'\x10\x00\x01' + bytes([0x80, 0, 0, n])
    if n: d += b'\x00\x07\x00\x00' + (100).to_bytes(2, 'big') + (900).to_bytes(2, 'big')
    return seg(0x16, d)
MOSTRA = pcs(1) + seg(0x14, bytes(12)) + seg(0x15, bytes(40)) + seg(0x80, b'')
LIMPA = pcs(0) + seg(0x80, b'')
def build(nome, falas, tira_limpa=None, simples=False, codec=b'S_HDMV/PGS'):
    ev = []
    for k in range(falas):
        ini = 5000 + k * 7000
        ev.append((ini, MOSTRA)); ev.append((ini + 2500 + (k % 5) * 300, LIMPA))
    if tira_limpa is not None: del ev[tira_limpa]
    tr = el(0x1654ae6b, el(0xae, u(0xd7, 6) + u(0x83, 17) + el(0x86, codec) + el(0x22b59c, b'eng')))
    clusters, cues_pts, off = [], [], 0
    # o SeekHead tem tamanho fixo; so o valor de Cues muda
    def seek(pos): return el(0x114d9b74, el(0x4dbb, el(0x53ab, bytes.fromhex('1c53bb6b')) + u(0x53ac, pos)))
    info = el(0x1549a966, u(0x2ad7b1, 1000000))   # TimestampScale = 1 ms
    sh = seek(0); base = len(sh) + len(info) + len(tr)
    for c in range(0, len(ev), 8):
        grupo = ev[c:c + 8]; t0 = grupo[0][0]
        body = u(0xe7, t0); pos = base + off
        for t, dados in grupo:
            cues_pts.append((t, pos, len(body)))
            blk = b'\x86' + (t - t0).to_bytes(2, 'big', signed=True) + b'\x00' + dados
            body += el(0xa3, blk) if simples else el(0xa0, el(0xa1, blk))
        cl = el(0x1f43b675, body); clusters.append(cl); off += len(cl)
    cues = el(0x1c53bb6b, b''.join(el(0xbb, u(0xb3, t) + el(0xb7, u(0xf7, 6) + u(0xf1, cp) + u(0xf0, r))) for t, cp, r in cues_pts))
    sh = seek(base + off)
    p.joinpath(nome).write_bytes(el(0x1a45dfa3, el(0x4282, b'matroska')) + el(0x18538067, sh + info + tr + b''.join(clusters) + cues + el(0xec, b'\0' * 1024)))
build('pgs.mkv', 80)
build('pgs_simple.mkv', 80, simples=True)
build('pgs_buraco.mkv', 80, tira_limpa=71)     # um "limpa" some no meio: a paridade vira
build('pgs_curto.mkv', 10)                     # poucos blocos: nao diz nada
build('texto.mkv', 80, codec=b'S_TEXT/UTF8')   # nao e PGS e nao tem texto
PY
cc -Isrc -O1 -g -Wall -Wextra src/legref.c src/legenda.c src/assrender.c tests/legref_pgs.c -pthread -lm -o "$D/t"
"$D/t" "$D"
