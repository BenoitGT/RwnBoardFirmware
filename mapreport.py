#!/usr/bin/env python3
"""
mapreport.py : rapport Markdown (avec graphes Mermaid) a partir d'un build XC8 pour PIC16.

  mapreport.py mem.map [--hex firmware.hex] [-o rapport.md] [--stack-limit 8]

  - le .map donne : occupation flash / RAM, taille de chaque fonction, variables globales
  - le .hex donne : les instructions CALL / GOTO reelles -> graphe d'appels et profondeur de
    la pile materielle (le .map XC8 v3 ne contient pas de graphe d'appels)
    Sans --hex, le script cherche <nom>.hex a cote du .sym indique dans le .map.

Seulement la bibliotheque standard de Python. Cible : PIC16 "mid-range" (instructions 14 bits,
CALL / GOTO sur 11 bits : un 16F628A a 2 K mots n'a pas besoin de PCLATH).
"""
import argparse
import bisect
import math
import os
import re
import sys


# ----------------------------------------------------------------------------- lecture du .map
def hx(s):
    return int(s.rstrip('hH'), 16)


def parse_map(path):
    text = open(path, encoding='utf-8', errors='replace').read().splitlines()
    m = {'ranges': {}, 'psects': {}, 'classes': {}, 'unused': {}, 'symbols': [], 'modules': [],
          'sym_path': None, 'device': ''}

    # --- ligne de commande de l'editeur de liens : tailles des classes
    cmd = ' '.join(text[:30])
    mm = re.search(r'-Q(\w+)', cmd)
    if mm:
        m['device'] = mm.group(1)
    mm = re.search(r'-h\+(\S+\.sym)', cmd)
    if mm:
        m['sym_path'] = mm.group(1)
    for cls, spec in re.findall(r'-A(\w+)=([0-9A-Fa-fh,\-x/]+)', cmd):
        total = 0
        rng = []
        for lo, hi in re.findall(r'([0-9A-Fa-f]+)h-([0-9A-Fa-f]+)h', spec):
            lo, hi = int(lo, 16), int(hi, 16)
            rng.append((lo, hi))
            total += hi - lo + 1
        m['ranges'][cls] = rng

    section = None
    cur_class = None
    cur_unused = None
    for ln in text:
        s = ln.strip()
        if s.startswith('TOTAL') and 'Name' in s:
            section = 'total'
            continue
        if s.startswith('SEGMENTS'):
            section = 'segments'
            continue
        if s.startswith('UNUSED ADDRESS RANGES'):
            section = 'unused'
            continue
        if s == 'Symbol Table':
            section = 'symbols'
            continue
        if s.startswith('MODULE INFORMATION'):
            section = 'modules'
            module = None
            continue

        if section == 'total':
            mc = re.match(r'CLASS\s+(\w+)', s)
            if mc:
                cur_class = mc.group(1)
                m['classes'].setdefault(cur_class, [])
                continue
            me = re.match(r'(\w+)\s+([0-9A-F]+)\s+([0-9A-F]+)\s+([0-9A-F]+)\s+(\d+)$', s)
            if me and cur_class:
                name, link, load, length, _ = me.groups()
                ent = {'name': name, 'link': int(link, 16), 'load': int(load, 16), 'len': int(length, 16),
                       'bits': name.startswith('bit')}
                m['classes'][cur_class].append(ent)
                m['psects'][name] = ent
        elif section == 'unused':
            if not s or s.startswith('Name'):
                continue
            mr = re.match(r'(?:(\w+)\s+)?([0-9A-F]+)-([0-9A-F]+)\s+', s + ' ')
            if mr:
                if mr.group(1):
                    cur_unused = mr.group(1)
                lo, hi = int(mr.group(2), 16), int(mr.group(3), 16)
                m['unused'].setdefault(cur_unused, []).append((lo, hi))
        elif section == 'symbols':
            parts = s.split()
            if len(parts) == 3:
                try:
                    m['symbols'].append((parts[0], parts[1], int(parts[2], 16)))
                except ValueError:
                    pass
        elif section == 'modules':
            if not s or s.startswith('Module'):
                continue
            if 'estimated size' in s:
                continue
            if not ln.startswith((' ', '\t')):
                module = s
                continue
            parts = s.split()
            if module and len(parts) >= 5 and parts[1] in ('CODE',):
                try:
                    m['modules'].append({'module': module, 'name': parts[0], 'addr': int(parts[2], 16),
                                          'size': int(parts[4])})
                except ValueError:
                    pass
    return m


def short(module):
    return 'shared (runtime)' if module == 'shared' else os.path.basename(module)


# ----------------------------------------------------------------------------- lecture du .hex
def read_hex_words(path):
    data = {}
    ext = 0
    for ln in open(path):
        ln = ln.strip()
        if not ln.startswith(':'):
            continue
        n = int(ln[1:3], 16)
        addr = int(ln[3:7], 16)
        typ = int(ln[7:9], 16)
        if typ == 4:
            ext = int(ln[9:13], 16) << 16
        elif typ == 0:
            for i in range(n):
                data[ext + addr + i] = int(ln[9 + 2 * i:11 + 2 * i], 16)
    words = {}
    for a in sorted(data):
        if a % 2 == 0 and a + 1 in data:
            words[a // 2] = data[a] | (data[a + 1] << 8)
    return words


# ----------------------------------------------------------------------------- graphe d'appels
def build_functions(m):
    funcs = []
    for e in m['modules']:
        funcs.append({'name': e['name'], 'start': e['addr'], 'size': e['size'], 'module': short(e['module'])})
    covered = [(f['start'], f['start'] + f['size']) for f in funcs]
    for ent in m['classes'].get('CODE', []):
        lo, hi = ent['link'], ent['link'] + ent['len']
        if ent['len'] and not any(a <= lo and hi <= b for a, b in covered):
            nm = {'intentry': 'interrupt_entry', 'reset_vec': 'reset_vec', 'end_init': 'end_init',
                  'cinit': 'cinit', 'clrtext': 'clear_ram'}.get(ent['name'], ent['name'])
            funcs.append({'name': nm, 'start': lo, 'size': ent['len'], 'module': 'startup'})
    funcs.sort(key=lambda f: f['start'])
    return funcs


def analyse_calls(funcs, words):
    starts = [f['start'] for f in funcs]

    def owner(addr):
        i = bisect.bisect_right(starts, addr) - 1
        if i >= 0 and addr < funcs[i]['start'] + funcs[i]['size']:
            return funcs[i]
        return None

    edges = []      # (src, dst, kind)
    warnings = []
    for f in funcs:
        for a in range(f['start'], f['start'] + f['size']):
            w = words.get(a)
            if w is None:
                continue
            if (w & 0x3800) == 0x2000:                      # CALL k
                kind = 'call'
            elif (w & 0x3800) == 0x2800:                    # GOTO k
                kind = 'goto'
            else:
                if w in (0x0082,) or (w & 0x3F80) == 0x0780 and (w & 0x7F) == 0x02:   # movwf PCL / addwf PCL,f
                    warnings.append(f"saut calcule (PCL) dans {f['name']} a 0x{a:03X} : table ou pointeur de fonction, a analyser a la main")
                continue
            tgt = w & 0x7FF
            dst = owner(tgt)
            if dst is None:
                warnings.append(f"{kind} {f['name']} -> 0x{tgt:03X} : cible hors des fonctions connues")
                continue
            if kind == 'goto' and dst is f:
                continue                                   # saut interne
            if dst['module'] == 'startup' and f['module'] != 'startup':
                continue                                   # fin de main : saut vers la sortie (exit), pas un vrai appel
            if tgt != dst['start']:
                warnings.append(f"{kind} {f['name']} -> {dst['name']}+{tgt - dst['start']} (entree au milieu de la fonction)")
            edges.append((f['name'], dst['name'], kind))
    seen = set()
    uniq = []
    for e in edges:
        if e not in seen:
            seen.add(e)
            uniq.append(e)
    return uniq, warnings


def depths(funcs, edges):
    out = {}
    for s, d, k in edges:
        out.setdefault(s, []).append((d, k))
    memo = {}
    cycles = []

    def depth(n, stack):
        if n in memo:
            return memo[n][0]
        best, via = 0, None
        for d, k in out.get(n, []):
            if d in stack:
                cycles.append(' -> '.join(stack + [d]))
                continue
            v = depth(d, stack + [d]) + (1 if k == 'call' else 0)
            if v > best or via is None:
                best, via = v, d
        memo[n] = (best, via)
        return best

    for f in funcs:
        depth(f['name'], [f['name']])
    return memo, cycles, out


def deepest_path(memo, out, root):
    path = [root]
    n = root
    guard = 0
    while memo.get(n, (0, None))[1] and guard < 64:
        n = memo[n][1]
        path.append(n)
        guard += 1
    return path


def reach(out, root):
    seen = {root}
    todo = [root]
    while todo:
        n = todo.pop()
        for d, _ in out.get(n, []):
            if d not in seen:
                seen.add(d)
                todo.append(d)
    return seen


# ----------------------------------------------------------------------------- rapport
def bank_of(addr, ranges):
    for cls in ('COMMON', 'BANK0', 'BANK1', 'BANK2', 'BANK3'):
        for lo, hi in ranges.get(cls, []):
            if lo <= addr <= hi:
                return cls
    return None


def md_table(head, rows, align=None):
    out = ['| ' + ' | '.join(head) + ' |', '|' + '|'.join(('---:' if (align and align[i] == 'r') else '---') for i in range(len(head))) + '|']
    for r in rows:
        out.append('| ' + ' | '.join(str(c) for c in r) + ' |')
    return '\n'.join(out)


def nid(name):
    return re.sub(r'\W', '_', name)


def report(m, words, limit, mapname, hexname):
    L = []
    L.append(f'# Rapport firmware : {m["device"]}')
    L.append('')
    L.append(f'Source : `{mapname}`' + (f' + `{hexname}`' if hexname else ' (pas de .hex : pas de graphe d\'appels)'))
    L.append('')

    # ---- 1. occupation
    code_total = sum(hi - lo + 1 for lo, hi in m['ranges'].get('CODE', []))
    code_used = sum(e['len'] for e in m['classes'].get('CODE', []))
    L.append('## 1. Occupation')
    L.append('')
    rows = [['Flash (mots de 14 bits)', code_used, code_total, code_total - code_used, f'{100 * code_used / code_total:.1f} %']]
    banks = {}
    for cls in ('COMMON', 'BANK0', 'BANK1', 'BANK2', 'BANK3'):
        if cls in m['ranges']:
            banks[cls] = [sum(hi - lo + 1 for lo, hi in m['ranges'][cls]), 0]
    for cls in ('COMMON', 'BANK0', 'BANK1', 'BANK2', 'BANK3', 'ABS1'):
        for e in m['classes'].get(cls, []):
            ln = math.ceil(e['len'] / 8) if e['bits'] else e['len']
            start = e['load'] if e['bits'] else e['link']
            b = bank_of(start, m['ranges']) or cls
            if b in banks:
                banks[b][1] += ln
    tot_ram = tot_used = 0
    for cls, (cap, used) in banks.items():
        rows.append([f'RAM {cls}', used, cap, cap - used, f'{100 * used / cap:.1f} %'])
        tot_ram += cap
        tot_used += used
    rows.append(['**RAM totale (octets)**', tot_used, tot_ram, tot_ram - tot_used, f'{100 * tot_used / tot_ram:.1f} %'])
    L.append(md_table(['Zone', 'Utilise', 'Capacite', 'Libre', 'Taux'], rows, 'lrrrr'))
    L.append('')
    for cls, (cap, used) in banks.items():
        if used >= cap:
            L.append(f'> **Attention** : {cls} est pleine ({used}/{cap} octets).')
    L.append('')

    # ---- 2. flash par module
    L.append('## 2. Flash par module et par fonction')
    L.append('')
    bymod = {}
    for e in m['modules']:
        bymod.setdefault(short(e['module']), []).append(e)
    rows = []
    for mod, ents in sorted(bymod.items(), key=lambda kv: -sum(x['size'] for x in kv[1])):
        rows.append([mod, len(ents), sum(x['size'] for x in ents)])
    listed = sum(r[2] for r in rows)
    rows.append(['startup (reset, interruption, init, effacement RAM)', '', code_used - listed])
    L.append(md_table(['Module', 'Fonctions', 'Mots'], rows, 'lrr'))
    L.append('')
    L.append('```mermaid')
    L.append('pie showData title Flash par module (mots)')
    for r in rows:
        if r[2] > 0:
            L.append(f'    "{r[0].split(" (")[0]}" : {r[2]}')
    L.append('```')
    L.append('')
    L.append('Les 12 plus grosses fonctions :')
    L.append('')
    allf = sorted(m['modules'], key=lambda e: -e['size'])[:12]
    L.append(md_table(['Fonction', 'Module', 'Adresse', 'Mots'], [[e['name'], short(e['module']), f'0x{e["addr"]:03X}', e['size']] for e in allf], 'llrr'))
    L.append('')
    free = m['unused'].get('CODE', [])
    big = [(lo, hi) for lo, hi in free if hi - lo > 8]
    if big:
        L.append('Flash libre : ' + ', '.join(f'0x{lo:03X}-0x{hi:03X} ({hi - lo + 1} mots)' for lo, hi in big))
        L.append('')

    # ---- 3. RAM
    L.append('## 3. RAM : variables globales')
    L.append('')
    var_psects = {n: e for n, e in m['psects'].items()
                  if not n.startswith('cstack') and not n.startswith('text') and n in
                  {x['name'] for c in ('COMMON', 'BANK0', 'BANK1', 'BANK2', 'BANK3') for x in m['classes'].get(c, [])}}
    rows = []
    bitrows = []
    for pn, pe in var_psects.items():
        if pe['bits']:
            for name, sp, addr in m['symbols']:
                if sp == pn and not name.startswith('__'):
                    bitrows.append([name.lstrip('_'), f'0x{addr // 8:03X}', addr % 8])
            continue
        syms = sorted([(a, n) for n, sp, a in m['symbols'] if sp == pn and not n.startswith('__')])
        end = pe['link'] + pe['len']
        for i, (a, n) in enumerate(syms):
            nxt = syms[i + 1][0] if i + 1 < len(syms) else end
            rows.append([n.lstrip('_'), pn, f'0x{a:03X}', min(nxt, end) - a])
    rows.sort(key=lambda r: -r[3])
    L.append(md_table(['Variable', 'Zone', 'Adresse', 'Octets'], rows, 'llrr'))
    L.append('')
    L.append('Taille deduite de l\'ecart entre symboles consecutifs (un remplissage eventuel est compte dans la variable qui precede).')
    L.append('')
    if bitrows:
        L.append('Variables d\'un bit : ' + ', '.join(f'`{n}` ({a} bit {b})' for n, a, b in bitrows))
        L.append('')
    cs = [(n, e) for n, e in m['psects'].items() if n.startswith('cstack')]
    if cs:
        L.append('Variables locales (pile compilee, partagees entre fonctions qui ne s\'appellent pas) : ' +
                 ', '.join(f'{n} = {e["len"]} octets' for n, e in cs) + '.')
        L.append('')

    # ---- 4. appels et pile
    L.append('## 4. Graphe d\'appels et pile materielle')
    L.append('')
    if not words:
        L.append('Pas de fichier .hex : lance le script avec `--hex <firmware>.hex`.')
        return '\n'.join(L) + '\n'

    funcs = build_functions(m)
    edges, warns = analyse_calls(funcs, words)
    memo, cycles, out = depths(funcs, edges)
    names = {f['name'] for f in funcs}
    isr_entry = next((f['name'] for f in funcs if f['start'] == 4), None)
    main = '_main' if '_main' in names else None
    d_main = memo[main][0] if main else 0
    d_isr = (1 + memo[isr_entry][0]) if isr_entry else 0
    worst = d_main + d_isr
    start_root = next((f['name'] for f in funcs if f['start'] == 0), None)

    rows = []
    if main:
        rows.append(['main (boucle principale)', d_main, ' -> '.join(x.lstrip('_') for x in deepest_path(memo, out, main))])
    if isr_entry:
        rows.append(['interruption (1 niveau pris par l\'interruption + appels)', d_isr, 'interruption -> ' + ' -> '.join(x.lstrip('_') for x in deepest_path(memo, out, isr_entry)[1:])] if len(deepest_path(memo, out, isr_entry)) > 1 else ['interruption', d_isr, ''])
    if start_root:
        rows.append(['demarrage (avant la boucle, interruptions coupees)', memo[start_root][0], ' -> '.join(x.lstrip('_') for x in deepest_path(memo, out, start_root))])
    L.append(md_table(['Contexte', 'Niveaux de pile', 'Chemin le plus profond'], rows, 'lrl'))
    L.append('')
    status = 'OK' if worst < limit else ('LIMITE ATTEINTE' if worst == limit else 'DEPASSEMENT')
    L.append(f'**Pire cas = main ({d_main}) + interruption ({d_isr}) = {worst} / {limit} niveaux : {status}** '
             f'(l\'interruption peut survenir au point le plus profond de main).')
    L.append('')
    if cycles:
        L.append('> Recursion detectee (profondeur non bornee) : ' + '; '.join(sorted(set(cycles))))
        L.append('')

    # graphe mermaid
    in_main = reach(out, main) if main else set()
    in_isr = reach(out, isr_entry) if isr_entry else set()
    used = in_main | in_isr
    fmap = {f['name']: f for f in funcs}
    L.append('```mermaid')
    L.append('flowchart LR')
    for n in sorted(used, key=lambda x: fmap[x]['start']):
        f = fmap[n]
        L.append(f'    {nid(n)}["{n.lstrip("_")}<br/>0x{f["start"]:03X} · {f["size"]} mots"]')
    for s, d, k in edges:
        if s in used and d in used:
            L.append(f'    {nid(s)} {"-->" if k == "call" else "-.->|goto|"} {nid(d)}')
    L.append('    classDef isr fill:#fde2e2,stroke:#c0392b')
    L.append('    classDef main fill:#e2edfd,stroke:#2e5fa3')
    L.append('    classDef both fill:#f3e8fd,stroke:#7a3fa3')
    for n in used:
        cls = 'both' if (n in in_main and n in in_isr) else ('main' if n in in_main else 'isr')
        L.append(f'    class {nid(n)} {cls}')
    L.append('```')
    L.append('')
    L.append('Bleu : appele depuis `main`. Rouge : appele depuis l\'interruption. Violet : les deux. Trait pointille : `goto` (saut de fin de fonction, ne consomme pas de pile).')
    L.append('')

    rows = []
    for n in sorted(used, key=lambda x: -memo[x][0]):
        callees = sorted({d.lstrip('_') for s, d, k in edges if s == n})
        rows.append([n.lstrip('_'), memo[n][0], ', '.join(callees) or '-'])
    L.append(md_table(['Fonction', 'Niveaux de pile sous elle', 'Appelle'], rows, 'lrl'))
    L.append('')
    if warns:
        L.append('### Points a verifier')
        L.append('')
        for w in sorted(set(warns)):
            L.append(f'- {w}')
        L.append('')
    L.append('Methode : les instructions `CALL` et `GOTO` du .hex sont rattachees aux fonctions par leurs adresses (.map). '
             'Un `GOTO` vers une autre fonction n\'ajoute pas de niveau. Les appels par pointeur ou par table (PCL) ne sont pas suivis.')
    return '\n'.join(L) + '\n'


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('--hex')
    ap.add_argument('-o', '--output')
    ap.add_argument('--stack-limit', type=int, default=8)
    a = ap.parse_args()

    m = parse_map(a.map)
    hexpath = a.hex
    if not hexpath and m['sym_path']:
        cand = os.path.splitext(m['sym_path'])[0] + '.hex'
        if os.path.exists(cand):
            hexpath = cand
    words = read_hex_words(hexpath) if hexpath and os.path.exists(hexpath) else {}
    txt = report(m, words, a.stack_limit, os.path.basename(a.map), os.path.basename(hexpath) if words else None)
    if a.output:
        open(a.output, 'w', encoding='utf-8').write(txt)
    else:
        sys.stdout.write(txt)


if __name__ == '__main__':
    main()
