#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path
from typing import Dict, List, Tuple

RE_BOARD = re.compile(r'Board\s*:\s*(\d+)\s*x\s*(\d+)', re.IGNORECASE)
RE_GAME_START = re.compile(r'-\s*Jogo\s*:\s*(\d+)', re.IGNORECASE)
RE_GAME_END = re.compile(r'Rounds\s*:\s*\(\s*\d+\s*\)', re.IGNORECASE)
RE_TUPLE = re.compile(r'\(\s*(\d+)\s*,\s*(\d+)\s*\)')

def parse_board_size(text: str) -> Tuple[int, int]:
    m = RE_BOARD.search(text)
    if not m:
        raise ValueError("Não foi possível encontrar o tamanho do tabuleiro após 'Board:' (ex: '7x7').")
    h, w = map(int, m.groups())
    return h, w

def slice_games(lines: List[str]) -> Dict[int, Tuple[int, int]]:
    """
    Devolve um dicionário game_number -> (linha_início, linha_fim_exclusiva).
    Cada fatia inclui tudo após '- Jogo: N' até (incluindo) à linha 'Rounds :'.
    """
    idx_by_game = {}
    i = 0
    while i < len(lines):
        m = RE_GAME_START.search(lines[i])
        if m:
            game_no = int(m.group(1))
            start = i + 1
            # find the next 'Rounds :' line
            j = start
            end = None
            while j < len(lines):
                if RE_GAME_END.search(lines[j]):
                    end = j + 1  # include the 'Rounds :' line
                    break
                # stop if a new game starts before we found Rounds (malformed)
                if j > start and RE_GAME_START.search(lines[j]):
                    end = j
                    break
                j += 1
            if end is None:
                end = len(lines)
            idx_by_game[game_no] = (start, end)
            i = end
        else:
            i += 1
    return idx_by_game

def extract_moves_from_slice(lines: List[str], start: int, end: int) -> List[Tuple[int, int]]:
    """
    Entre start..end, cada linha de jogada tem exatamente um tuplo (r,c).
    Recolhe-se o primeiro tuplo encontrado por linha, mantendo a ordem.
    """
    moves: List[Tuple[int, int]] = []
    for i in range(start, end):
        line = lines[i]
        m = RE_TUPLE.search(line)
        if m:
            r, c = int(m.group(1)), int(m.group(2))
            moves.append((r, c))
    return moves

def serialize(moves: List[Tuple[int, int]], fmt: str) -> str:
    if fmt == "lines":
        return "\n".join(f"{r},{c}" for r, c in moves) + ("\n" if moves else "")
    if fmt == "list":
        return "[" + ", ".join(f"({r},{c})" for r, c in moves) + "]\n"
    if fmt == "json":
        return json.dumps(moves) + "\n"
    raise ValueError(f"Unknown format: {fmt}")

def main():
    ap = argparse.ArgumentParser(
        description="Converter logs do Rastros em listas de jogadas por jogo."
    )
    ap.add_argument("input_path", help="Caminho para o ficheiro .txt de log")
    ap.add_argument(
        "-o", "--output",
        help=("Caminho opcional de saída. Se for diretório, os ficheiros são criados lá. "
              "Se for ficheiro e apenas um jogo for extraído, escreve-se exatamente esse ficheiro. "
              "Se omitir, a saída fica ao lado do input.")
    )
    ap.add_argument(
        "--format",
        choices=["lines", "list", "json"],
        default="lines",
        help="Formato de saída (por defeito: lines)."
    )
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--game", type=int, help="Número do jogo a extrair (ex.: 44)")
    g.add_argument("--all", action="store_true", help="Extrair todos os jogos")

    args = ap.parse_args()
    in_path = Path(args.input_path)
    if not in_path.exists():
        raise SystemExit(f"Ficheiro de entrada não encontrado: {in_path}")

    text = in_path.read_text(encoding="utf-8", errors="replace")
    height, width = parse_board_size(text)

    lines = text.splitlines()
    game_slices = slice_games(lines)
    if not game_slices:
        raise SystemExit("Nenhum jogo encontrado (falta bloco '- Jogo: <N>').")

    out_is_dir_hint = False
    out_path = Path(args.output) if args.output else None
    if out_path:
        out_is_dir_hint = out_path.is_dir() or str(out_path).endswith(("/", "\\"))

    written = 0

    def make_name_for_game(n: int) -> Path:
        # <stem>_g<N>_<HxW>.txt (ex.: Ad-An_7_g44_7x7.txt)
        stem = in_path.stem
        fname = f"{stem}_g{n}_{height}x{width}.txt"
        if out_path:
            return (out_path / fname) if (out_is_dir_hint or args.all) else out_path
        else:
            return in_path.with_name(fname)

    if args.game is not None:
        n = args.game
        if n not in game_slices:
            avail = ", ".join(map(str, sorted(game_slices)))
            raise SystemExit(f"Game {n} not found. Available games: {avail}")
        start, end = game_slices[n]
        moves = extract_moves_from_slice(lines, start, end)
        content = serialize(moves, args.format)
        out_file = make_name_for_game(n)
        out_file.parent.mkdir(parents=True, exist_ok=True)
        out_file.write_text(content, encoding="utf-8")
        print(f"Wrote {len(moves)} moves to: {out_file}")
        written += 1
    else:
        # --all
        for n in sorted(game_slices):
            start, end = game_slices[n]
            moves = extract_moves_from_slice(lines, start, end)
            content = serialize(moves, args.format)
            out_file = make_name_for_game(n)
            # Em --all, se o user passou um caminho não-dir, trata-se como diretório usando o parent
            if out_path and not out_is_dir_hint and not args.game:
                out_file = out_path.parent / out_file.name
            out_file.parent.mkdir(parents=True, exist_ok=True)
            out_file.write_text(content, encoding="utf-8")
            print(f"Wrote {len(moves)} moves to: {out_file}")
            written += 1

    if written == 0:
        raise SystemExit("Nothing written (unexpected).")

if __name__ == "__main__":
    main()
