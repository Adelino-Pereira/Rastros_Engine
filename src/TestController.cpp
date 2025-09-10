#include "TestController.hpp"
#include <iostream>
#include <random>
#include <vector>
#include <utility>
#include <stdexcept>
#include "Heuristic1.hpp"
#include "Heuristic2.hpp"
#include "AI.hpp"

TestController::TestController(const std::string& mode, int rows, int cols,std::pair<int, int>  first_move, int debug)
    : ai_player(true, max_depth,heuristic1,debug),
      ai_player_2(false, max_depth, heuristic2,debug),
      mode(mode),
      rounds(0),
      board(rows, cols), // initialize directly in the member initializer list
      first_move(first_move)

{
        ai_player.reset_ordering_stats();
        ai_player_2.reset_ordering_stats();
        
        ai_player = AI(true, max_depth,heuristic1,debug);
        ai_player_2 = AI(false, max_depth,heuristic2,debug);

         // board.set_current_player_from_int(2);
}

TestController::TestController(const std::string& mode, int rows, int cols, int debug)
    : ai_player(true, max_depth,heuristic1,debug),
      ai_player_2(false, max_depth, heuristic2,debug),
      mode(mode),
      rounds(0),
      board(rows, cols) // initialize directly in the member initializer list
{
        ai_player.reset_ordering_stats();
        ai_player_2.reset_ordering_stats();

        ai_player = AI(true, max_depth,heuristic1,debug);
        ai_player_2 = AI(false, max_depth,heuristic2,debug);

         // board.set_current_player_from_int(2);
}

void TestController::configure_quiescence(bool max_on, bool min_on,
                                          int max_plies, int swing_delta, int low_mob) {
    ai_player.set_quiescence(max_on, max_plies, swing_delta, low_mob);
    ai_player_2.set_quiescence(min_on, max_plies, swing_delta, low_mob);
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

    while (!board.is_terminal()) {
        //board.print();
        if (mode == 1){play_ai_vs_ai_turn_mode1();}
        if (mode == 2){play_ai_vs_ai_turn_mode2();}

        board.switch_player();
        rounds++;
    }

    //board.print();
    std::cout << "Rounds : (" << rounds-1 << ")\n";
    std::cout << "=== Ordering Quality (P1/MAX AI) ===\n";
    ai_player.print_ordering_stats();
    std::cout << "=== Ordering Quality (P2/MIN AI) ===\n";
    ai_player_2.print_ordering_stats();

    ai_player.clear_tt();
    ai_player_2.clear_tt();
    return handle_terminal_state();
    

}

void TestController::play_ai_turn() {
    std::cout << "\n🤖 Turno do Jogador " << (board.current_player ? "1" : "2") << " (IA)...\n";

    std::pair<int, int> move;

    auto& ai = board.current_player ? ai_player : ai_player_2;
    int depth = std::min(start_depth + rounds / 5, max_depth);
    move = ai.choose_move(board,depth,rounds);
    board.make_move(move);

    //board.make_move(move);
}




void TestController::play_ai_vs_ai_turn_mode1() {
    //std::cout << "\nTurno do ";
    //std::cout << (board.current_player ? "Jogador 1 (IA)" : "Jogador 2 (IA)") << "...\n";
    auto& ai = board.current_player ? ai_player : ai_player_2;
    auto player = board.current_player == 1 ? "MAX" : "MIN";
    //if (board.current_player == 1){start_depth=1;}
    int depth = std::min(start_depth + rounds / 5, max_depth);
    depth = (depth % 2 == 0) ? depth - 1 : depth;
    //depth = (depth % 2 != 0) ? depth + 1 : depth;
    depth = std::max(depth, start_depth); 
    //std::cout << "depth: " << depth << "\n";
    auto marker = board.get_marker();
    if(rounds==0){
        //auto move = std::pair<int, int>{3, 4};
        auto move = first_move;
        //auto move = get_random_move(get_valid_moves());
        // std::cout<<"bc: "<<board.cols-2<<"\n";
        // std::cout<<"fm: "<< (move.first <=1) <<"sm: "<< (move.second >= board.cols-2) <<"\n";
        // while (move.first <=1 && move.second >= board.cols-2){
        //     move = get_random_move(get_valid_moves());
        // };
        std::cout << "["<<player<<"]First AI controled move : (" << move.first << ", " << move.second << ")\n";
        board.make_move(move);
    }else{
        auto move = ai.choose_move(board, depth,rounds);
        //std::cout <<"|"<< std::string(depth, '-')<<"["<<player<<"]move : (" << move.first << ", " << move.second << ")\n";
        board.make_move(move);
    }
}


void TestController::play_ai_vs_ai_turn_mode2() {
    //std::cout << "\nTurno do ";
    //std::cout << (board.current_player ? "Jogador 1 (IA)" : "Jogador 2 (IA)") << "...\n";
    auto& ai = board.current_player ? ai_player : ai_player_2;
    auto player = board.current_player == 1 ? "MAX" : "MIN";
    //if (board.current_player == 1){start_depth=1;}
    int depth = std::min(start_depth + rounds / 5, max_depth);
    depth = (depth % 2 == 0) ? depth - 1 : depth;
    //depth = (depth % 2 != 0) ? depth + 1 : depth;
    depth = std::max(depth, start_depth); 
    //std::cout << "depth: " << depth << "\n";
    auto marker = board.get_marker();
    // int a = 2;
    if(rounds==10000){
        // auto move = std::pair<int, int>{1, 4};
        //auto move = first_move;
        auto move = get_random_move(get_valid_moves());
        // std::cout<<"bc: "<<board.cols-2<<"\n";
        // std::cout<<"fm: "<< (move.first <=1) <<"sm: "<< (move.second >= board.cols-2) <<"\n";
        while (move.first <=1 && move.second >= board.cols-2){
           move = get_random_move(get_valid_moves());
        };
        std::cout << "["<<player<<"]First AI controled move : (" << move.first << ", " << move.second << ")\n";
        board.make_move(move);
    }else{
        auto move = ai.choose_move(board, depth,rounds);
        //std::cout <<"|"<< std::string(depth, '-')<<"["<<player<<"]move : (" << move.first << ", " << move.second << ")\n";
        board.make_move(move);
    }
}

bool TestController::handle_terminal_state() {
std::cout << "[Visited] lookups=" << AI::vs_lookups
          << " hits=" << AI::vs_hits
          << " inserts=" << AI::vs_inserts
          << " hit_rate=" << (100.0 * AI::vs_hits / std::max<uint64_t>(1, AI::vs_lookups))
          << "%\n";

    if (board.marker == std::make_pair(board.rows-1, 0)) {
        std::cout << "Vitória do Jogador 1!\n\n";
        return winner = true;
    } else if (board.marker == std::make_pair(0, board.cols-1)) {
        std::cout << "Vitória do Jogador 2!\n\n";
        return winner = false;
    } else {
        std::cout << "Sem jogadas válidas. Fim de jogo!\n\n";
        if(board.current_player == true){
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
    if ((mode == "ai_first" && board.current_player) ||
        (mode == "human_first" && !board.current_player) ||
        (mode == "ai_vs_ai")) {
        play_ai_turn();
        board.switch_player();
    } else {
        std::cout << "[Warning] Attempted AI turn when it's not AI's turn.\n";
    }
}


/////

std::vector<int> TestController::get_flat_grid() {
    std::vector<int> flat;
    for (const auto& row : board.grid) {
        flat.insert(flat.end(), row.begin(), row.end());
    }
    return flat;
}


std::pair<int, int> TestController::get_marker() {
    return board.marker;
}

std::vector<std::pair<int, int>> TestController::get_valid_moves() {
    return board.get_valid_moves();
}


#include <vector>
#include <utility>
#include <random>
#include <stdexcept>

std::pair<int, int> TestController:: get_random_move(const std::vector<std::pair<int, int>>& moves) {
    if (moves.empty()) {
        throw std::runtime_error("No valid moves available.");
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<> dist(0, moves.size() - 1);

    return moves[dist(g)];
}


