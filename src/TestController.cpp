#include "TestController.hpp"
#include <iostream>
#include <vector>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include "Heuristic1.hpp"
#include "Heuristic2.hpp"
#include "AI.hpp"
#include "LogMsgs.hpp"

TestController::TestController(const std::string& mode, int rows, int cols,std::pair<int, int>  first_move, int debug,
                               HeuristicCombo combo_p1, HeuristicCombo combo_p2)
    : ai_player(true, max_depth,
                [combo_p1](const Board& b, bool is_max){ return heuristic1_combo(b, is_max, combo_p1); },
                debug),
      ai_player_2(false, max_depth,
                [combo_p2](const Board& b, bool is_max){ return heuristic2_combo(b, is_max, combo_p2); },
                debug),
      mode(mode),
      rounds(0),
      board(rows, cols), // initialize directly in the member initializer list
      first_move(first_move),
      combo_p1(combo_p1),
      combo_p2(combo_p2) {
        // Construtor usado no modo de teste 1: força um primeiro movimento específico
        // para comparar AIs com a mesma posição inicial controlada - os 8 primeiros
        // movimentos possiveis.
        ai_player.reset_ordering_stats();
        ai_player_2.reset_ordering_stats();
}

TestController::TestController(const std::string& mode, int rows, int cols, int debug,
                               HeuristicCombo combo_p1, HeuristicCombo combo_p2)
    : ai_player(true, max_depth,
                [combo_p1](const Board& b, bool is_max){ return heuristic1_combo(b, is_max, combo_p1); },
                debug),
      ai_player_2(false, max_depth,
                [combo_p2](const Board& b, bool is_max){ return heuristic2_combo(b, is_max, combo_p2); },
                debug),
      mode(mode),
      rounds(0),
      board(rows, cols), // initialize directly in the member initializer list
      combo_p1(combo_p1),
      combo_p2(combo_p2) {
        // Construtor usado nos restantes modos de teste (sem first_move forçado)
        //First Move aleatório, com restrições, em choose_move em AI.cpp.
        ai_player.reset_ordering_stats();
        ai_player_2.reset_ordering_stats();
}

void TestController::configure_quiescence(bool max_on, bool min_on,
                                          int max_plies, int swing_delta, int low_mob) {
    // Ativa/desativa quiescence por lado e parametriza a extensão extra de busca.
    ai_player.set_quiescence(max_on, max_plies, swing_delta, low_mob);
    ai_player_2.set_quiescence(min_on, max_plies, swing_delta, low_mob);
}

void TestController::set_depth_limits_p1(int start, int max) {
    start_depth_p1 = start;
    max_depth_p1 = max;
}

void TestController::set_depth_limits_p2(int start, int max) {
    start_depth_p2 = start;
    max_depth_p2 = max;
}



void TestController::configure_ordering(OrderingPolicy pMax,
                                        OrderingPolicy pMin,
                                        double sigmaMax,
                                        double sigmaMin,
                                        bool shuffleTiesOnly) {
    ai_player.set_ordering_policy(pMax);
    ai_player.set_order_noise(sigmaMax);
    ai_player.set_shuffle_ties_only(shuffleTiesOnly);

    ai_player_2.set_ordering_policy(pMin);
    ai_player_2.set_order_noise(sigmaMin);
    ai_player_2.set_shuffle_ties_only(shuffleTiesOnly);
}


bool TestController::run(int mode) {
    static bool heur_logged_once = false;
    if (!heur_logged_once) {
        LogMsgs::out() << "[heur] P1=" << heuristic_combo_label(combo_p1)
                     << " P2=" << heuristic_combo_label(combo_p2) << "\n";
        heur_logged_once = true;
    }

    // reiniciar contadores e caches para medições consistentes antes de cada bateria de jogos
    AI::vs_lookups = AI::vs_hits = AI::vs_inserts = 0;
    ai_player.clear_tt();
    ai_player.clear_order_caches();
    ai_player.clear_s_heuristic_caches();
    ai_player_2.clear_tt();
    ai_player_2.clear_order_caches();
    ai_player_2.clear_s_heuristic_caches();

    while (!board.is_terminal()) {
        if (mode == 1){play_ai_vs_ai_turn_mode1();}
        else if (mode == 2){play_ai_vs_ai_turn_mode2();}
        else {
            std::cerr << "Modo de teste inválido: " << mode << "\n";
            break;
        }

        board.switch_player();
        rounds++;
    }

    // Estatísticas resumidas no final de cada jogo
    std::cout << "Rounds : (" << rounds-1 << ")\n";
    std::cout << "=== Ordering Quality (P1/MAX AI) ===\n";
    ai_player.print_ordering_stats();
    std::cout << "=== Ordering Quality (P2/MIN AI) ===\n";
    ai_player_2.print_ordering_stats();

    ai_player.clear_tt();
    ai_player.clear_order_caches();
    ai_player.clear_s_heuristic_caches();
    ai_player_2.clear_tt();
    ai_player_2.clear_order_caches();
    ai_player_2.clear_s_heuristic_caches();

    return handle_terminal_state();
    

}

void TestController::set_depth_limits(int start, int max) {
    start_depth = start;
    max_depth = max;
    start_depth_p1 = start;
    max_depth_p1 = max;
    start_depth_p2 = start;
    max_depth_p2 = max;
}

void TestController::play_ai_turn() {
    std::cout << "\n🤖 Turno do Jogador " << (board.current_player_is_max() ? "1" : "2") << " (IA)...\n";

    std::pair<int, int> move;

    auto& ai = board.current_player_is_max() ? ai_player : ai_player_2;
    int depth = compute_depth_for_player(board.current_player_is_max());
    move = ai.choose_move(board, depth, rounds);
    board.make_move(move);

}


void TestController::play_ai_vs_ai_turn_mode1() {
    auto move = select_ai_move(true);
    board.make_move(move);
}


void TestController::play_ai_vs_ai_turn_mode2() {
    auto move = select_ai_move(false);
    board.make_move(move);
}

bool TestController::handle_terminal_state() {
std::cout << "[Visited] lookups=" << AI::vs_lookups
          << " hits=" << AI::vs_hits
          << " inserts=" << AI::vs_inserts
          << " hit_rate=" << (100.0 * AI::vs_hits / std::max<uint64_t>(1, AI::vs_lookups))
          << "%\n"; // relatório da TT

    auto mk = board.get_marker();
    if (mk == std::make_pair(board.get_rows()-1, 0)) {
        std::cout << "Vitória do Jogador 1!\n\n";
        return winner = true;
    } else if (mk == std::make_pair(0, board.get_cols()-1)) {
        std::cout << "Vitória do Jogador 2!\n\n";
        return winner = false;
    } else {
        std::cout << "Sem jogadas válidas. Fim de jogo!\n\n";
        if(board.current_player_is_max() == true){
            std::cout << "Vitória do Jogador 2!\n\n";
            return winner = false;
        }else{
            std::cout << "Vitória do Jogador 1!\n\n";
            return winner = true;
        }
    }

}

void TestController::run_ai_turn() {
    std::cout << "mode ->" << mode <<  std::endl;
    if ((mode == "ai_first" && board.current_player_is_max()) ||
        (mode == "human_first" && !board.current_player_is_max()) ||
        (mode == "ai_vs_ai")) {
        play_ai_turn();
        board.switch_player();
    } else {
        std::cout << "[Warning] Attempted AI turn when it's not AI's turn.\n";
    }
}


std::pair<int, int> TestController::get_marker() {
    return board.get_marker();
}

int TestController::compute_depth() const {
    int depth = std::min(start_depth + rounds / 5, max_depth);
    depth = (depth % 2 == 0) ? depth - 1 : depth; // força profundidade ímpar 
    return std::max(depth, start_depth);
}

int TestController::compute_depth_for_player(bool is_max) const {
    int s = is_max ? start_depth_p1 : start_depth_p2;
    int m = is_max ? max_depth_p1 : max_depth_p2;
    int depth = std::min(s + rounds / 5, m);
    depth = (depth % 2 == 0) ? depth - 1 : depth; // força ímpar
    return std::max(depth, s);
}

std::pair<int, int> TestController::select_ai_move(bool allow_first_override) {
    auto& ai = board.current_player_is_max() ? ai_player : ai_player_2;
    if (allow_first_override && rounds == 0) {
        auto moves = board.get_valid_moves();
        auto it = std::find(moves.begin(), moves.end(), first_move);
        if (it != moves.end()) {
            std::cout << "[" << (board.current_player_is_max() ? "MAX" : "MIN")
                      << "] First AI controled move : (" << first_move.first
                      << ", " << first_move.second << ")\n";
            return first_move;
        }
        if (!moves.empty()) return moves.front(); // fallback seguro se a jogada forçada não for válida
    }
    int depth = compute_depth_for_player(board.current_player_is_max());
    return ai.choose_move(board, depth, rounds);
}
