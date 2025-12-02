#include <emscripten/bind.h>
#include "Board.hpp"
#include "AI.hpp"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(std_types) {
    value_array<std::pair<int, int>>("PairIntInt")
        .element(&std::pair<int, int>::first)
        .element(&std::pair<int, int>::second);

    register_vector<int>("VectorInt");
    register_vector<std::vector<int>>("VectorVectorInt");
}

EMSCRIPTEN_BINDINGS(rastros_module) {
    //last changes
    function("initHeuristics", &AI::register_heuristics);
    function("createAIWithLevel", &AI::create_with_level);
    //

    enum_<OrderingPolicy>("OrderingPolicy")
    .value("Deterministic", OrderingPolicy::Deterministic)
    .value("ShuffleAll", OrderingPolicy::ShuffleAll)
    .value("NoisyJitter", OrderingPolicy::NoisyJitter);

    
    class_<Board>("Board")
        .constructor<int, int>()
        .function("getFlatGrid", &Board::get_flat_grid)
        .function("getFlatValidMoves", &Board::get_flat_valid_moves)
        .function("getMarker", &Board::get_marker)
        .function("getFlatMarker", &Board::get_marker_flat)
        .function("makeMove", &Board::make_move)
        .function("isTerminal", &Board::is_terminal)
        .function("switchPlayer", &Board::switch_player)
        .function("currentPlayerIsHuman", &Board::current_player_is_human)
        .function("currentPlayerIsMax", &Board::current_player_is_max)
        .function("getRows", &Board::get_rows)
        .function("getCols", &Board::get_cols)
        
        .function("getWinner", &Board::get_winner)

        // === NOVO ===
        .function("resetBoard", &Board::reset_board)
        .function("setMarker", &Board::set_marker_pos)
        .function("blockCell", &Board::block_cell)
        .function("setCurrentPlayerInt", &Board::set_current_player_from_int)
        // ============
        ;

    class_<AI>("AI")
        .constructor<bool, int>()
        .function("chooseMove", static_cast<std::pair<int, int> (AI::*)(Board&, int, int)>(&AI::choose_move))
        .function("setOrderingPolicy", &AI::set_ordering_policy)
        .function("setShuffleTiesOnly", &AI::set_shuffle_ties_only)
        .function("setOrderNoise", &AI::set_order_noise)
        .function("setQuiescence", &AI::set_quiescence)
        .function("clearTT", &AI::clear_tt)
        .function("clearOrderCaches", &AI::clear_order_caches)
        .function("clearSuccessorHeuristicCaches", &AI::clear_s_heuristic_caches)
        .function("setDebugLevel", &AI::set_debug_level)
        .function("getDebugLevel", &AI::get_debug_level);
}
