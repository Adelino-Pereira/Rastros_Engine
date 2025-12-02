#!/usr/bin/env python3
import argparse
import csv
import re
from pathlib import Path

"""
Transforma ficheiros log txt em CSVs por jogo com a opção (--summary) para gerar sumário do torneio."""

# --- Regexes / markers ----------------------------------------------------- #

RE_JOGO = re.compile(r'Jogo:\s*(\d+)', re.IGNORECASE)
RE_ROUNDS = re.compile(r'Rounds\s*:\s*\((\d+)\)')
RE_WINNER = re.compile(r'Vit[óo]ria do Jogador\s*(\d+)', re.IGNORECASE)
RE_TIME = re.compile(r'tempo jogo:\s*([0-9.]+)', re.IGNORECASE)

START_MARKER = "[MAX]First AI controled move"
#START_MARKER = "****[MAX] Best move selected:"
TRAP_MARKER = "Sem jogadas válidas. Fim de jogo!"
COORD_RE = re.compile(r'\((\d+),\s*(\d+)\)')


# --- Helpers --------------------------------------------------------------- #

def extract_coord(line):
    """Return 'x,y' from a '(x, y)' pattern in line, or None."""
    m = COORD_RE.search(line)
    if m:
        return f"{m.group(1)},{m.group(2)}"
    return None


def parse_log_file(path):
    """
    Parse a single log txt file and return a list of dicts with:
    game_number, winner, win_type, rounds, time, moves_key

    - win_type: 'O' (objective) or 'T' (trap)
    - time is stored as float for easier summing
    - moves_key is a string representing the full sequence of moves,
      used to detect unique games.
    """
    results = []
    current_game = None
    pending_game_number = None
    auto_game_counter = 0

    with path.open("r", encoding="utf-8", errors="ignore") as f:
        for raw_line in f:
            line = raw_line.rstrip("\n")

            # Track "Jogo: X"
            m_jogo = RE_JOGO.search(line)
            if m_jogo:
                try:
                    pending_game_number = int(m_jogo.group(1))
                except ValueError:
                    pending_game_number = None

            # Start of game
            if START_MARKER in line:
                auto_game_counter += 1
                game_number = pending_game_number or auto_game_counter
                current_game = {
                    "game_number": game_number,
                    "winner": None,
                    "win_type": "O",  # default: objective win
                    "rounds": None,
                    "time": None,
                    "moves": [],
                    "moves_key": None,
                }
                # First move (MAX)
                c = extract_coord(line)
                if c:
                    current_game["moves"].append(c)
                continue

            # If not inside a game yet, skip
            if current_game is None:
                continue

            # Trap detection
            if TRAP_MARKER in line:
                current_game["win_type"] = "T"

            # Moves for uniqueness:
            if "Best move selected" in line or "Immediate win found" in line:
                c = extract_coord(line)
                if c:
                    current_game["moves"].append(c)

            # Rounds
            m_rounds = RE_ROUNDS.search(line)
            if m_rounds:
                try:
                    current_game["rounds"] = int(m_rounds.group(1))
                except ValueError:
                    pass

            # Winner
            m_winner = RE_WINNER.search(line)
            if m_winner:
                try:
                    current_game["winner"] = int(m_winner.group(1))
                except ValueError:
                    pass

            # Time
            m_time = RE_TIME.search(line)
            if m_time:
                try:
                    current_game["time"] = float(m_time.group(1))
                except ValueError:
                    pass

            # Finalize game when we have rounds, winner, and time
            if (
                current_game["rounds"] is not None
                and current_game["winner"] is not None
                and current_game["time"] is not None
            ):
                current_game["moves_key"] = " ".join(current_game["moves"])
                results.append(current_game)
                current_game = None

    # If last game is partially parsed but has a winner, keep it
    if current_game is not None and current_game["winner"] is not None:
        current_game["moves_key"] = " ".join(current_game["moves"])
        results.append(current_game)

    return results


def determine_output_path(input_path, output_arg, multiple_files):
    """
    Decide the *per-game* CSV output path for a given input file.

    - If multiple_files is True, output_arg (if given) must be a directory.
    - If output_arg is None, CSV goes next to the input file with same name.
    - If output_arg is a directory, CSV goes inside it with the same base name.
    - If output_arg is a file path, only allowed when multiple_files is False.
    """
    base_csv_name = input_path.with_suffix(".csv").name

    if output_arg is None:
        # Same directory as the input file
        return input_path.with_suffix(".csv")

    out_path = Path(output_arg)

    if multiple_files:
        # In directory mode, the output must be a directory (or not exist yet)
        if out_path.exists() and not out_path.is_dir():
            raise ValueError("When input is a directory, --output must be a directory (or not exist yet).")
        out_path.mkdir(parents=True, exist_ok=True)
        return out_path / base_csv_name

    # Single input file
    if out_path.exists() and out_path.is_dir():
        # Save inside this directory
        return out_path / base_csv_name

    # Treat as explicit file path
    if not out_path.suffix:
        out_path = out_path.with_suffix(".csv")

    return out_path


def write_csv(path, rows):
    """Write per-game rows (list of dicts) to CSV at path."""
    fieldnames = ["game_number", "winner", "win_type", "rounds", "time"]
    path.parent.mkdir(parents=True, exist_ok=True)

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow({
                "game_number": row.get("game_number"),
                "winner": row.get("winner"),
                "win_type": row.get("win_type"),
                "rounds": row.get("rounds"),
                "time": row.get("time"),
            })


def summarize_games(games):
    """
    Build summary statistics from a list of game dicts.

    Returns a dict with:
    "P1 O", "P1 T", "P2 O", "P2 T",
    "Total Time", "Shortest Timed Game", "Longest Timed Game",
    "Total Number of Rounds", "Smallest Round Number",
    "Greatest Round Number", "Unique Games"
    """
    p1_o = p1_t = p2_o = p2_t = 0
    total_time = 0.0
    time_values = []
    rounds_values = []
    sequences = set()

    for g in games:
        winner = g.get("winner")
        win_type = g.get("win_type")
        time_val = g.get("time")
        rounds_val = g.get("rounds")

        # Count wins
        if winner == 1:
            if win_type == "T":
                p1_t += 1
            else:
                p1_o += 1
        elif winner == 2:
            if win_type == "T":
                p2_t += 1
            else:
                p2_o += 1

        # Time
        if time_val is not None:
            try:
                t = float(time_val)
                total_time += t
                time_values.append(t)
            except ValueError:
                pass

        # Rounds
        if rounds_val is not None:
            try:
                r = int(rounds_val)
                rounds_values.append(r)
            except ValueError:
                pass

        # Unique games by full move sequence
        key = g.get("moves_key")
        if key:
            sequences.add(key)

    shortest_game = min(time_values) if time_values else ""
    longest_game = max(time_values) if time_values else ""
    total_rounds = sum(rounds_values) if rounds_values else 0
    smallest_round = min(rounds_values) if rounds_values else ""
    greatest_round = max(rounds_values) if rounds_values else ""

    return {
        "P1 O": p1_o,
        "P1 T": p1_t,
        "P2 O": p2_o,
        "P2 T": p2_t,
        "Total Time": total_time,
        "Shortest Timed Game": shortest_game,
        "Longest Timed Game": longest_game,
        "Total Number of Rounds": total_rounds,
        "Smallest Round Number": smallest_round,
        "Greatest Round Number": greatest_round,
        "Unique Games": len(sequences),
    }


def write_summary_csv(path, summary_row):
    """
    Write a single-row summary CSV at path with columns:
    P1 O, P1 T, P2 O, P2 T, Total Time,
    Shortest Timed Game, Longest Timed Game,
    Total Number of Rounds, Smallest Round Number,
    Greatest Round Number, Unique Games
    """
    fieldnames = [
        "P1 O",
        "P1 T",
        "P2 O",
        "P2 T",
        "Total Time",
        "Shortest Timed Game",
        "Longest Timed Game",
        "Total Number of Rounds",
        "Smallest Round Number",
        "Greatest Round Number",
        "Unique Games",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerow(summary_row)


# --- CLI ------------------------------------------------------------------- #

def main():
    parser = argparse.ArgumentParser(
        description="Parse Slime Trail MAX/MIN log txt files into per-game CSV and optional summary CSV."
    )
    parser.add_argument(
        "input",
        help="Path to a txt file or a directory containing txt log files."
    )
    parser.add_argument(
        "-o", "--output",
        help=(
            "Optional output path. "
            "If input is a single file: can be a CSV file path or a directory. "
            "If input is a directory: must be a directory where CSVs will be written."
        ),
        default=None,
    )
    parser.add_argument(
        "--summary",
        action="store_true",
        help=(
            "Also write a summarized CSV per txt file with fields: "
            "P1 O, P1 T, P2 O, P2 T, Total Time, "
            "Shortest Timed Game, Longest Timed Game, "
            "Total Number of Rounds, Smallest Round Number, "
            "Greatest Round Number, Unique Games."
        ),
    )

    args = parser.parse_args()
    input_path = Path(args.input)

    if not input_path.exists():
        raise SystemExit(f"Input path does not exist: {input_path}")

    if input_path.is_file():
        # Single file mode
        csv_path = determine_output_path(input_path, args.output, multiple_files=False)
        games = parse_log_file(input_path)
        write_csv(csv_path, games)
        print(f"Wrote {len(games)} games to {csv_path}")

        if args.summary:
            summary = summarize_games(games)
            summary_csv_path = csv_path.with_name(csv_path.stem + "_summary.csv")
            write_summary_csv(summary_csv_path, summary)
            print(f"Wrote summary to {summary_csv_path}")

    elif input_path.is_dir():
        # Directory mode: parse all .txt files
        txt_files = sorted(
            p for p in input_path.iterdir() if p.is_file() and p.suffix.lower() == ".txt"
        )
        if not txt_files:
            print(f"[WARN] No .txt files found in directory {input_path}")
            return

        for txt in txt_files:
            games = parse_log_file(txt)
            csv_path = determine_output_path(txt, args.output, multiple_files=True)
            write_csv(csv_path, games)
            print(f"Wrote {len(games)} games from {txt.name} to {csv_path}")

            if args.summary:
                summary = summarize_games(games)
                summary_csv_path = csv_path.with_name(csv_path.stem + "_summary.csv")
                write_summary_csv(summary_csv_path, summary)
                print(f"Wrote summary for {txt.name} to {summary_csv_path}")

    else:
        raise SystemExit(f"Input path is neither a file nor a directory: {input_path}")


if __name__ == "__main__":
    main()
