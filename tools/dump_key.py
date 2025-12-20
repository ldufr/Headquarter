import argparse
import utils
import sys
from scanner import FileScanner

def reverse_endian(data):
    return data[::-1]

def print_bytes(bytes):
    str = '\\x' + '\\x'.join('%02X' % b for b in bytes)
    print(str)

def follow_call(scanner, call_rva):
    op, call_param = scanner.read(call_rva, '<Bi')
    if op != 0xE8 and op != 0xE9:
        raise RuntimeError(f"Unsupported opcode '0x{op:02X} ({op})'")
    return call_rva + call_param + 5

def get_build_number(scanner):
    addr = scanner.find(b'\x68\x00\x00\x00\x10\x6A\x00\x6A\x00\x6A\x00\x50\xFF', -0x2A)
    addr = follow_call(scanner, addr)
    build_number, = scanner.read(addr + 1, '<I')
    return build_number

def get_keys_from_scanner(scanner):
    addr = scanner.find(b'\x8B\x45\x08\xC7\x00\x88\x00\x00\x00\xB8', +0xA)
    keys, = scanner.read(addr)

    if type(scanner) is FileScanner:
        # It would be nice to not do that explicitly, but overall, this is a reloc,
        # so when reading from a file, we need to remove the `ImageBase` to get an
        # RVA based on 0.
        keys -= scanner.parsed.OPTIONAL_HEADER.ImageBase

    pr, = scanner.read(keys + 4, '4s')
    pm, = scanner.read(keys + 8, '64s')
    pk, = scanner.read(keys + 72, '64s')

    pr = int.from_bytes(pr, byteorder='little')
    pm = int.from_bytes(pm, byteorder='little')
    pk = int.from_bytes(pk, byteorder='little')

    return (pr, pm, pk)

def write_keys_in_file(path, pr, pm, pk):
    content = f'root = {pr}\nserver_public = {pk}\nprime = {pm}'
    open(path, 'w').write(content)

def main(args):
    if args.pid:
        import process
        proc = process.Process(args.pid)
        scanner = process.ProcessScanner(proc)
    elif args.file:
        scanner = FileScanner(args.file)
    elif args.proc:
        import process
        proc = process.Process.from_name(args.proc)
        scanner = process.ProcessScanner(proc)

    pr, pm, pk = get_keys_from_scanner(scanner)

    if args.output:
        output = args.output
    else:
        build = get_build_number(scanner)
        output = utils.get_path('data', f'gw_{build}.pub.txt')

    content = f'root = {pr}\nserver_public = {pk}\nprime = {pm}'
    if output == '-':
        print(content)
    else:
        print(f'Writting to "{output}"')
        open(output, 'w').write(content)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--pid", type=int, required=False,
        help="Process id of the target Guild Wars instance.")
    parser.add_argument("--proc", type=str, default='Gw.exe',
        help="Process name of the target Guild Wars instance.")
    parser.add_argument("--file", type=str, required=False,
        help="Path to the file on disk.")
    parser.add_argument("--output", "-o", type=str, required=False,
        help="Path where to write the output keys.")
    args = parser.parse_args()

    main(args)
