#!/usr/bin/env python3
import argparse
import csv
import json
import re
from pathlib import Path
'''
Converte um registo de jogo no formato csv, descarregado no final de
uma partida da aplicação web, numa lista com coordenadas adequadas para
carregar na aplicação CLI.

'''


MOVE_RE = re.compile(r'^\s*([A-Za-z])\s*(\d+)\s*$')
SIZE_RE = re.compile(r'(\d+)\s*x\s*(\d+)', re.IGNORECASE)

def parse_size_from_name(name: str):
    """
    Extrai "AlturaxLargura do nome do ficheiro csv"
    Devolve (height, width) como ints.
    Escolhe a última ocurrência se houver mais que uma
    """
    matches = SIZE_RE.findall(name)
    if not matches:
        raise ValueError(
            "O tamanho do tabuleiro deve estar definido algures no nome do ficheiro. "
            "Examplo: ..._7x7_....csv"
        )
    h, w = matches[-1]
    return int(h), int(w)

def coord_to_indices(token: str, height: int, width: int):
    """
    Convert coordenadas tipo xadrez (ex:e4, colRow) em coordenadas
    adequadas ao tabuleiro CLI (row,col)
    """
    m = MOVE_RE.match(token)
    if not m:
        raise ValueError(f"Invalid move format: {token!r}")
    letter, num_str = m.groups()
    col = ord(letter.lower()) - ord('a')  # 0-based left→right
    row_from_bottom_1 = int(num_str)     # 1-based bottom→up

    if not (0 <= col < width):
        raise ValueError(f"Column out of bounds for board width {width}: {token!r}")

    if not (1 <= row_from_bottom_1 <= height):
        raise ValueError(f"Row out of bounds for board height {height}: {token!r}")

    # Converte bottom->up para top->down, 0-index
    row = height - row_from_bottom_1
    return (row, col)

def read_moves_from_csv(csv_path: Path, height: int, width: int):
    """
    Lê ficheiro  CSV com as colunas : Ronda nº, Jogador 1, Jogador 2
    devolve uma lista  (row, col) das jogadas por ordem de jogada:
    J1 da ronda 1, J2 da ronda 1, J1 da ronda 2, J2 da ronda 2, ...
    Ignora células vazias.
    """
    moves = []
    with csv_path.open('r', encoding='utf-8-sig', newline='') as f:
        reader = csv.reader(f)
        # Skip header
        try:
            header = next(reader)
        except StopIteration:
            return moves

        for row in reader:
            # Defensive: allow rows with extra/less columns
            j1 = row[1].strip() if len(row) > 1 and row[1] else ''
            j2 = row[2].strip() if len(row) > 2 and row[2] else ''
            if j1:
                moves.append(coord_to_indices(j1, height, width))
            if j2:
                moves.append(coord_to_indices(j2, height, width))
    return moves

def main():
    ap = argparse.ArgumentParser(
        description="Converte um registo de jogo CSV para uma lista (row,col) de jogadas."
    )
    ap.add_argument("csv_path", help="Caminho para o ficheiro CSV")
    ap.add_argument(
        "-o", "--output",
        help=("Opcional: Caminho de saída.se for um diretório grava "
              "um ficheiro .txt com o mesmo nome do csv nesse directório. "
              "Se nenhum caminho for dado grava no mesmo local do ficheiro de entrada")
    )
    ap.add_argument(
        "--format",
        choices=["list", "json", "lines"],
        default="lines",
        help=("formato da saída: "
              "'list' -> estilo Python [(r,c), ...] (default); "
              "'json' -> JSON array [[r,c], ...]; "
              "'lines' -> uma coordenada 'r,c' por linha.")
    )
    args = ap.parse_args()

    csv_path = Path(args.csv_path)
    if not csv_path.exists():
        raise SystemExit(f"Não foram encontrados ficheiros CSV: {csv_path}")

    height, width = parse_size_from_name(csv_path.name)

    moves = read_moves_from_csv(csv_path, height, width)

    # Decidir caminho de saída
    if args.output:
        out_path = Path(args.output)
        if out_path.is_dir() or str(args.output).endswith(("/", "\\")):
            out_file = out_path / (csv_path.stem + ".txt")
        else:
            out_file = out_path
    else:
        out_file = csv_path.with_suffix(".txt")

    # Trasformar de acordo com o formato escolhido
    if args.format == "list":
        # lista de tuplos estilo Python
        content = "[" + ", ".join(f"({r},{c})" for r, c in moves) + "]\n"
    elif args.format == "json":
        #lista de listas
        content = json.dumps(moves) + "\n"
    else:  # "lines"
        #uma jogada por linha (CLI format)
        content = "\n".join(f"{r},{c}" for r, c in moves) + ("\n" if moves else "")

    out_file.parent.mkdir(parents=True, exist_ok=True)
    out_file.write_text(content, encoding="utf-8")
    print(f"Wrote {len(moves)} moves to: {out_file}")

if __name__ == "__main__":
    main()
