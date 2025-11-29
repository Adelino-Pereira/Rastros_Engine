#include "Heuristic1.hpp"
#include "HeuristicsUtils.hpp"

const char* heuristic_combo_label(HeuristicCombo combo) {
    switch (combo) {
        case HeuristicCombo::A: return "A";
        case HeuristicCombo::B: return "B";
        case HeuristicCombo::C: return "C";
        case HeuristicCombo::D: return "D";
        case HeuristicCombo::E: return "E";
        case HeuristicCombo::F: return "F";
        case HeuristicCombo::G: return "G";
        case HeuristicCombo::H: return "H";
        case HeuristicCombo::I: return "I";
        case HeuristicCombo::J: return "J";
    }
    return "?";
}

static int compute_combo(const Board& board,
                         bool is_max,
                         HeuristicCombo combo) {
    auto pos = board.get_marker();

    Board::ReachabilityResult reach = board.compute_reachability();

    int h1 = reach.h1;
    int h5 = reach.h5;

    int h8 = 0;
    if (std::abs(h1) == 900 && std::abs(h5) == 900) {
        h8 = (reach.reachable_count % 2 == 0)
             ? (is_max ? 200 : -200)
             : (is_max ? -200 : 200);
    }

    int h9 = available_choices(board,is_max);
    int hDiag = h_diag_block_goal(board);

    int dist = h_distance(board, pos, is_max);
    int o_dist = h_distance(board, pos, !is_max);

    int penalty = 0;
    if ((dist + 2 < std::abs(h1)) && (std::abs(h1) != 900))
        penalty += -(std::abs(h1) - dist);

    if ((o_dist + 2 < std::abs(h5)) && (std::abs(h5) != 900))
        penalty += (std::abs(h5) - o_dist);

    switch (combo) {
        case HeuristicCombo::A: return h1;
        case HeuristicCombo::B: return h5;
        case HeuristicCombo::C: return h1 + h5;
        case HeuristicCombo::D: return h1 + h5 + h8;
        case HeuristicCombo::E: return h1 + h5 + h9;
        case HeuristicCombo::F: return h1 + h5 + h8 + h9;
        case HeuristicCombo::G: return h1 + h5 + h8 + hDiag;
        case HeuristicCombo::H: return h1 + h5 + h9 + hDiag;
        case HeuristicCombo::I: return h1 + h5 + h8 + h9 + hDiag;
        case HeuristicCombo::J: return h1 + h5 + hDiag;
    }
    return h1 + h5 + h8 + hDiag;
}

int heuristic1_combo(const Board& board, bool is_max, HeuristicCombo combo) {
    return compute_combo(board, is_max, combo);
}

int heuristic1(const Board& board, bool is_max) {
    return compute_combo(board, is_max, HeuristicCombo::A);
}
