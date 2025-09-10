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
    
    class_<Board>("Board")
        .constructor<>()
        .constructor<int, int>()
        .function("getValidMoves", &Board::get_valid_moves_js)
        .function("makeMove", &Board::make_move)
        .function("isTerminal", &Board::is_terminal)
        .function("switchPlayer", &Board::switch_player)
        .function("currentPlayerIsHuman", &Board::current_player_is_human)
        .function("shortestPathToGoal", &Board::shortest_path_to_goal)
        .function("getGrid", &Board::get_grid)
        .function("getWinner", &Board::get_winner)

        // === NOVO ===
        .function("resetBoard", &Board::reset_board)
        .function("setMarker", &Board::set_marker_pos)
        .function("blockCell", &Board::block_cell)
        .function("setCurrentPlayerInt", &Board::set_current_player_from_int)
        // ============
        
        .property("marker", &Board::marker)
        .property("currentPlayer", &Board::current_player)
        .property("rows", &Board::rows)   
        .property("cols", &Board::cols);

    class_<AI>("AI")
        .constructor<bool, int>()
        .function("chooseMove", static_cast<std::pair<int, int> (AI::*)(Board&, int, int)>(&AI::choose_move));
}
