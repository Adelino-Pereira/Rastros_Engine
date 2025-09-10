#include "GameController.hpp"
#include <iostream>
#include <random>


GameController::GameController(const std::string& mode, int rows, int cols)
    : ai_player(true, max_depth),
      ai_player_2(false, max_depth),
      mode(mode),
      rounds(0),
      board(rows, cols) // initialize directly in the member initializer list
{
    if (mode == "ai_first") {
        ai_player = AI(true, max_depth);
    } else if (mode == "human_first") {
        ai_player_2 = AI(false, max_depth);
    } else if (mode == "ai_vs_ai") {
        ai_player = AI(true, max_depth);
        ai_player_2 = AI(false, max_depth);
    }
}


GameController::GameController(const std::string& mode, int rows, int cols, const Board& board, int rounds)
    : ai_player(true, max_depth),
      ai_player_2(false, max_depth),
      mode(mode),
      rounds(rounds),
      board(board)
{
    if (mode == "ai_first") {
        ai_player = AI(true, max_depth);
    } else if (mode == "human_first") {
        ai_player_2 = AI(false, max_depth);
    } else if (mode == "ai_vs_ai") {
        ai_player = AI(true, max_depth);
        ai_player_2 = AI(false, max_depth);
    }
}



void GameController::run() {
    while (!board.is_terminal()) {
        board.print();

        if (mode == "human_vs_human") {
            play_human_turn();
        } else if (mode == "human_first") {
            if (board.current_player) play_human_turn();
            else play_ai_turn();
        } else if (mode == "ai_first") {
            if (board.current_player) play_ai_turn();
            else play_human_turn();
        } else if (mode == "ai_vs_ai") {
            play_ai_vs_ai_turn();
        }

        board.switch_player();
        rounds++;
    }

    board.print();
    handle_terminal_state();
}

void GameController::play_ai_turn() {
    std::cout << "\n🤖 Turno do Jogador " << (board.current_player ? "1" : "2") << " (IA)...\n";

    std::pair<int, int> move;

    // if (rounds == 0 && board.current_player) {
    //     std::random_device rd;
    //     std::mt19937 gen(rd());
    //     std::uniform_int_distribution<> dis(0, static_cast<int>(initial_moves.size() - 1));
    //     move = initial_moves[dis(gen)];
    // } else {
        auto& ai = board.current_player ? ai_player : ai_player_2;
        int depth = std::min(start_depth + rounds / 5, max_depth);
        depth = (depth % 2 == 0) ? depth - 1 : depth;
        depth = std::max(depth, start_depth);
        move = ai.choose_move(board,depth,rounds);
        board.make_move(move);
    //}

    // auto valid = board.get_valid_moves();
    // bool found = std::find(valid.begin(), valid.end(), move) != valid.end();

    // if (!found) {
    //     std::cout << "❌ AI attempted invalid move: (" << move.first << "," << move.second << ")" << std::endl;
    // } else {
    //     std::cout << "✅ AI move applied: (" << move.first << "," << move.second << ")" << std::endl;
    //     board.make_move(move);
    // }



    //board.make_move(move);
}

void GameController::play_human_turn() {
    std::cout << "\n🧑 Turno do Jogador (Humano)...\n";
    auto moves = board.get_valid_moves();
    if (moves.empty()) {
        std::cout << "Sem jogadas válidas disponíveis.\n";
        return;
    }
    for (size_t i = 0; i < moves.size(); ++i) {
        std::cout << i << ": (" << moves[i].first << ", " << moves[i].second << ")\n";
    }
    int choice;
    std::cout << "Escolhe uma jogada: ";
    std::cin >> choice;
    if (choice >= 0 && static_cast<size_t>(choice) < moves.size()) {
        board.make_move(moves[choice]);
    }
}

void GameController::play_ai_vs_ai_turn() {
    std::cout << "\nTurno do ";
    std::cout << (board.current_player ? "Jogador 1 (IA)" : "Jogador 2 (IA)") << "...\n";
    auto& ai = board.current_player ? ai_player : ai_player_2;
    int depth = std::min(start_depth + rounds / 5, max_depth);
    auto move = ai.choose_move(board, depth,rounds);

    board.make_move(move);
}

void GameController::handle_terminal_state() {
    if (board.marker == std::make_pair(6, 0)) {
        std::cout << "🎉 Vitória do Jogador 1!\n";
    } else if (board.marker == std::make_pair(0, 6)) {
        std::cout << "🎉 Vitória do Jogador 2!\n";
    } else {
        std::cout << "⚠️ Sem jogadas válidas. Fim de jogo!\n";
    }
}

void GameController::run_ai_turn() {
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

std::vector<int> GameController::get_flat_grid() {
    std::vector<int> flat;
    for (const auto& row : board.grid) {
        flat.insert(flat.end(), row.begin(), row.end());
    }
    return flat;
}


std::pair<int, int> GameController::get_marker() {
    return board.marker;
}

std::vector<std::pair<int, int>> GameController::get_valid_moves() {
    return board.get_valid_moves();
}


