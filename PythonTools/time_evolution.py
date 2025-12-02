#!/usr/bin/env python3
import os
import sys
import glob
import re

import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator
import matplotlib.cm as cm

"""
Plota a evolução do tempo por jogada em função da profundidade de pesquisa,
a partir de ficheiros de log gerados pelo Enginev3 em modo de teste.
Cada ficheiro de log deve ter um nome que inclua a profundidade de pesquisa,
exemplo: GvsG_d14.txt para profundidade 14."""



def extract_depth(filepath: str):
    """
    Extracts the search depth from a filename.
    Example: GvsG_d14.txt -> 14
    """
    filename = os.path.basename(filepath)
    # First try pattern ...d14.txt
    m = re.search(r'd(\d+)(?=\.txt$)', filename)
    if m:
        return int(m.group(1))
    # Fallback: last number before .txt
    m = re.search(r'(\d+)(?=\.txt$)', filename)
    return int(m.group(1)) if m else None


def extract_times(filepath: str):
    """
    Extracts the time (in seconds) of each move from the file.
    Considers lines from the first:
        ****[MIN] Best move selected: ...
    up to the line before:
        Rounds :
    For each line in that range, it reads the last "[... s]" and takes the number.
    """
    times = []
    started = False

    with open(filepath, encoding="utf-8") as f:
        for raw_line in f:
            line = raw_line.strip()

            # Wait until we see the first MIN Best move line
            if not started:
                if line.startswith("****[MIN] Best move selected:"):
                    started = True
                else:
                    continue

            # Stop when we get to "Rounds :"
            if line.startswith("Rounds :"):
                break

            # Extract the last [ ... ] and parse the seconds inside
            # e.g. [30.0720 s]
            left = line.rfind('[')
            right = line.rfind(']')
            if left == -1 or right == -1 or right <= left:
                continue

            inside = line[left + 1:right]  # "30.0720 s"
            parts = inside.split()
            if not parts:
                continue

            try:
                t = float(parts[0])  # first part is the number
                times.append(t)
            except ValueError:
                # If it doesn't parse as float, ignore this line
                continue

    return times


def main(folder: str):
    # Get all .txt files in the folder
    pattern = os.path.join(folder, "*.txt")
    files = sorted(glob.glob(pattern))

    if not files:
        print(f"No .txt files found in {folder}")
        return

    depth_series = []

    for filepath in files:
        depth = extract_depth(filepath)
        if depth is None:
            # Skip files without a depth number
            continue

        times = extract_times(filepath)
        if not times:
            # Skip files with no move times found
            continue

        depth_series.append((depth, times))

    if not depth_series:
        print("No move data found in any file.")
        return

    # Sort by depth so the legend is ordered nicely
    depth_series.sort(key=lambda x: x[0])

    # Plot
    plt.figure()

    cmap = cm.get_cmap('tab20', 15) 

    # for depth, times in depth_series:
    #     x = list(range(1, len(times) + 1,))  # move numbers 1..N
    #     plt.plot(x, times, label=f"prof. {depth}")
    #     plt.plot(x[-1], times[-1], marker='s')

    for i, (depth, times) in enumerate(depth_series):
        x = list(range(1, len(times) + 1))

        color = cmap(i)

        line, = plt.plot(x, times, color=color, label=f"prof {depth}")
        plt.plot(x[-1], times[-1], marker='o', color=color)



    plt.xlabel("nº da jogada")
    plt.ylabel("Tempo por jogada (s)")
    #plt.title("Evolution of time per move by search depth")
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend()
    ax = plt.gca()
    ax.xaxis.set_major_locator(MultipleLocator(2))
    ax.yaxis.set_major_locator(MultipleLocator(5))
    plt.tight_layout()

    plt.show()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {os.path.basename(sys.argv[0])} FOLDER_WITH_TXT_FILES")
        sys.exit(1)

    main(sys.argv[1])
