#ifndef HEURISTIC2_HPP
#define HEURISTIC2_HPP

#include "Board.hpp"
#include "Heuristic1.hpp" // reutiliza enum HeuristicCombo

int heuristic2_combo(const Board& board, bool is_max, HeuristicCombo combo);
int heuristic2(const Board& board, bool is_max); // default (mantém retorno atual: C)

#endif
