#ifndef HEURISTICS_UTILS_HPP
#define HEURISTICS_UTILS_HPP

#include "Board.hpp"

// Combos de heurística (reutilizado por IA e controladores)
enum class HeuristicCombo {
    A, B, C, D, E, F, G, H, I, J,Noise
};

// Etiqueta curta para logging
const char* heuristic_combo_label(HeuristicCombo combo);
// Calcula score combinando métricas do Board segundo o combo pedido
int heuristic_combo_score(const Board& board, bool is_max, HeuristicCombo combo);

int h_trap(const Board& board, bool is_max);
int check_corners(const Board& board, std::pair<int, int> pos, bool is_max);
int quadrant_bonus(const Board& board, std::pair<int, int> pos, bool is_max);
int h_distance(const Board& board, std::pair<int, int> pos, bool is_max);
int parity_heuristic(const Board& board, bool is_max,int path_val, int o_path_val);
int count_unplayables(const Board& board);

int available_choices(const Board& board, bool is_max);

int h_diag_block_goal(const Board& b);

#endif
