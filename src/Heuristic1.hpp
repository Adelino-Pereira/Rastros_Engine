#ifndef HEURISTIC1_HPP
#define HEURISTIC1_HPP

#include "Board.hpp"

enum class HeuristicCombo {
    A, B, C, D, E, F, G, H, I, J
};

// Avalia usando a combinação pedida
int heuristic1_combo(const Board& board, bool is_max, HeuristicCombo combo);
// Atalho default (mantém retorno atual: A)
int heuristic1(const Board& board, bool is_max);
// Etiqueta curta para logging
const char* heuristic_combo_label(HeuristicCombo combo);

#endif
