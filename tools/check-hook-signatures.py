#!/usr/bin/env python3
"""Check native call signatures against the game binary.

Covers every HookBase<FuncSig, Address, ...> and every raw function-pointer
typedef cast from a fixed address ((FnType)0x1xxxxxxx). The number of stack
bytes the signature pushes is compared with the `ret N` of the original
function in GlobalAgenda.exe. A mismatch leaves ESP
off after CallOriginal; whether that crashes depends on the compiler's frame
layout, so it may only show up with some compilers.

Usage: tools/check-hook-signatures.py <path/to/GlobalAgenda.exe> [src-dir]
Exit code 1 if any mismatch or unknown type is found.
"""
import os, re, subprocess, sys

SIZES = {
    'int': 4, 'unsigned': 4, 'unsigned int': 4, 'long': 4, 'unsigned long': 4,
    'float': 4, 'bool': 4, 'char': 4, 'unsigned char': 4, 'BYTE': 4, 'short': 4,
    'unsigned short': 4, 'UBOOL': 4, 'INT': 4, 'DWORD': 4, 'FLOAT': 4, 'SOCKET': 4,
    'uint32_t': 4, 'int32_t': 4, 'uint8_t': 4, 'size_t': 4,
    'double': 8, 'long long': 8, 'unsigned long long': 8, '__int64': 8,
    'uint64_t': 8, 'int64_t': 8, 'QWORD': 8,
    'BOOL': 4, 'SIZE_T': 4, 'std::size_t': 4,
    'FName': 8, 'FVector': 12, 'FRotator': 12, 'FString': 12,
    # by-value structs used in hook signatures
    'FBuffHeader': 0x10, 'FTGEQUIP_SLOTS_STRUCT': 0xC8, 'FImpactInfoBytes': 0x60,
}

HOOK_RE = re.compile(r'HookBase<\s*(.+?)\s*,\s*(0x[0-9A-Fa-f]+)\s*,\s*(\w+)\s*>', re.S)
SIG_RE = re.compile(r'^(.*?)\(\s*(__\w+)\s*\*\s*\)\s*\((.*)\)$', re.S)


def split_args(s):
    out, depth, cur = [], 0, ''
    for ch in s:
        if ch in '<(':
            depth += 1
        elif ch in '>)':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur); cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return [a.strip() for a in out if a.strip() not in ('', 'void')]


def arg_size(a):
    a = re.sub(r'/\*.*?\*/', '', a).strip()
    if '*' in a or '&' in a:
        return 4
    t = re.sub(r'\b(const|volatile|struct|class|enum)\b', '', a).strip()
    t = ' '.join(t.split())
    if t in SIZES:
        return SIZES[t]
    # drop a trailing parameter name
    t2 = ' '.join(t.split()[:-1])
    if t2 in SIZES:
        return SIZES[t2]
    return None


def stack_bytes(conv, args):
    sizes = [arg_size(a) for a in args]
    if None in sizes:
        return None, [a for a, s in zip(args, sizes) if s is None]
    if conv == '__cdecl':
        return 0, []  # caller cleans up
    if conv == '__thiscall':
        sizes = sizes[1:]
    elif conv == '__fastcall':
        regs = 0
        while sizes and regs < 2 and sizes[0] <= 4:
            sizes = sizes[1:]; regs += 1
    return sum((s + 3) & ~3 for s in sizes), []


def original_ret(exe, addr, depth=0):
    """ret N of the function at addr; follows direct tail jumps. None if unknown."""
    out = subprocess.run(['objdump', '-d', '-M', 'intel', '--no-show-raw-insn',
                          f'--start-address={addr:#x}', f'--stop-address={addr + 0x4000:#x}', exe],
                         capture_output=True, text=True).stdout
    tail = None
    for line in out.splitlines():
        m = re.search(r'\t(ret|repz ret)\s*(0x[0-9a-f]+)?\s*$', line)
        if m:
            return int(m.group(2), 16) if m.group(2) else 0
        if re.search(r'\tint3\s*$', line):
            break  # end of function without ret: it leaves through a tail jump
        m = re.search(r'\tjmp\s+(\S.*)$', line)
        if m:
            tail = m.group(1).strip()
    if tail and re.fullmatch(r'0x[0-9a-f]+', tail) and depth < 4:
        return original_ret(exe, int(tail, 16), depth + 1)
    return None


TYPEDEF_RES = [
    re.compile(r'typedef\s+([^;()]*?)\(\s*(__\w+)\s*\*\s*(\w+)\s*\)\s*\(([^;]*?)\)\s*;', re.S),
    re.compile(r'using\s+(\w+)\s*=\s*([^;()]*?)\(\s*(__\w+)\s*\*\s*\)\s*\(([^;]*?)\)\s*;', re.S),
]
CAST_RE = re.compile(r'(?:\(\s*(\w+)\s*\)|reinterpret_cast<\s*(\w+)\s*>\()\s*(0x1[0-9A-Fa-f]{7})\b')


# Originals that leave through an indirect tail jump, verified by hand:
# address -> stack bytes popped.
KNOWN_RETS = {
    0x10a12f10: 4,  # invObj device setter, jmp [vtable+0x124]; native caller pushes 1 arg, no cleanup
}


def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def check(exe, label, conv, args, addr):
    """Returns (checked, problem-line or None)."""
    want, unknown = stack_bytes(conv, args)
    if unknown:
        return 0, f'?? {label}: unknown arg size {unknown}'
    got = KNOWN_RETS.get(addr)
    if got is None:
        got = original_ret(exe, addr)
    if got is None:
        return 1, f'?? {label} @ {addr:#x}: ret not found (indirect tail jump?), check by hand'
    if conv == '__cdecl' and got != 0:
        return 1, f'MISMATCH {label} @ {addr:#x}: signature is __cdecl but original does ret {got:#x}'
    if conv != '__cdecl' and got != want:
        return 1, f'MISMATCH {label} @ {addr:#x}: signature pushes {want:#x} bytes, original does ret {got:#x}'
    return 1, None


def check_raw_pointers(exe, path, text):
    """Raw typedef'd function pointers cast from a fixed address, typedef resolved per file."""
    text = strip_comments(text)
    sigs = {}
    for m in TYPEDEF_RES[0].finditer(text):
        sigs[m.group(3)] = (m.group(2), split_args(' '.join(m.group(4).split())))
    for m in TYPEDEF_RES[1].finditer(text):
        sigs[m.group(1)] = (m.group(3), split_args(' '.join(m.group(4).split())))
    seen = set()
    for m in CAST_RE.finditer(text):
        name = m.group(1) or m.group(2)
        addr = int(m.group(3), 16)
        if name not in sigs or (name, addr) in seen:
            continue
        seen.add((name, addr))
        line = text.count('\n', 0, m.start()) + 1
        yield check(exe, f'{path}:{line} {name}', sigs[name][0], sigs[name][1], addr)


def main():
    if len(sys.argv) < 2:
        print(__doc__); return 2
    exe = sys.argv[1]
    src = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), '..', 'src')
    bad = 0; checked = 0
    for root, dirs, files in os.walk(src):
        dirs[:] = [d for d in dirs if d != 'SDK']
        for f in sorted(files):
            if not f.endswith(('.hpp', '.cpp', '.h')):
                continue
            path = os.path.join(root, f)
            text = open(path, encoding='utf-8', errors='replace').read()
            results = []
            for m in HOOK_RE.finditer(text):
                sig, addr, name = m.group(1), int(m.group(2), 16), m.group(3)
                sm = SIG_RE.match(' '.join(sig.split()))
                if not sm:
                    results.append((0, f'?? {name}: cannot parse FuncSig `{sig}`')); continue
                results.append(check(exe, name, sm.group(2), split_args(sm.group(3)), addr))
            results.extend(check_raw_pointers(exe, os.path.relpath(path, os.path.dirname(src)), text))
            for n, problem in results:
                checked += n
                if problem:
                    print(problem); bad += 1
    print(f'{checked} signatures checked, {bad} problems')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
