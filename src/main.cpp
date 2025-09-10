#include <iostream>
#include "GameController.hpp"
#include "TestController.hpp"
#include "Board.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <chrono> 
#include <optional>
#include <vector>


//input helpers
static std::optional<int> get_flag_int(int argc, char* argv[], const std::string& shortf, const std::string& longf) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == shortf || a == longf) {
            if (i + 1 < argc) return std::stoi(argv[i + 1]);
        } else if (a.rfind(longf + "=", 0) == 0) {
            return std::stoi(a.substr(longf.size() + 1));
        }
    }
    return std::nullopt;
}

static std::vector<std::string> strip_new_flags(int argc, char* argv[]) {
    std::vector<std::string> out;
    for (int i = 0; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-d" || a == "--depth" ||
            a == "-md" || a == "--max-depth" ||
            a == "-g" || a == "--games") {
            // skip this and the next (its value), if present
            ++i;
            continue;
        }
        // also skip --depth=, --max-depth=, etc.
        if (a.rfind("--depth=", 0) == 0 ||
            a.rfind("--max-depth=", 0) == 0 ||
            a.rfind("--games=", 0) == 0) {
            continue;
        }
        out.push_back(a);
    }
    return out;
}




static OrderingPolicy parse_policy(const std::string& s) {
    if (s == "D" || s == "det" || s == "Deterministic") return OrderingPolicy::Deterministic;
    if (s == "S" || s == "shuffle" || s == "ShuffleAll") return OrderingPolicy::ShuffleAll;
    if (s == "N" || s == "noise" || s == "NoisyJitter") return OrderingPolicy::NoisyJitter;
    return OrderingPolicy::Deterministic;
}

static bool parse_bool(const std::string& s) {
    return (s == "1" || s == "true" || s == "True" || s == "yes" || s == "y");
}
//output helpers
static const char* policy_str(OrderingPolicy p) {
    switch (p) {
        case OrderingPolicy::Deterministic: return "Deterministic";
        case OrderingPolicy::ShuffleAll:    return "ShuffleAll";
        case OrderingPolicy::NoisyJitter:   return "NoisyJitter";
    }
    return "Deterministic";
}

static std::string policy_repr(OrderingPolicy p, double sigma) {
  if (p == OrderingPolicy::NoisyJitter) {
    std::ostringstream oss;
    oss << "NoisyJitter (sigma=" << sigma << ")";
    return oss.str();
  }
  return policy_str(p);
}


static void print_quiescence_header(bool qMax, bool qMin,
                                    int plies, int swing, int lowmob) {
    std::cout << "[Quiescence] MAX=" << (qMax ? "on" : "off")
              << (qMax ? ("(plies=" + std::to_string(plies) +
                          ", swing=" + std::to_string(swing) +
                          ", lowMob=" + std::to_string(lowmob) + ")") : "")
              << ", MIN=" << (qMin ? "on" : "off")
              << (qMin ? ("(plies=" + std::to_string(plies) +
                          ", swing=" + std::to_string(swing) +
                          ", lowMob=" + std::to_string(lowmob) + ")") : "")
              << "\n";
}



int main(int argc, char* argv[]) {
    int rows, cols;
    std::string mode;
    std::string main_choice;
    int debug = 0;

    auto depthFlag    = get_flag_int(argc, argv, "-d",  "--depth");
    auto maxDepthFlag = get_flag_int(argc, argv, "-md", "--max-depth");
    auto gamesFlag    = get_flag_int(argc, argv, "-g",  "--games");

    if (depthFlag && !maxDepthFlag) maxDepthFlag = depthFlag;
    //cópia dos args sem as flags novas
    auto cleaned = strip_new_flags(argc, argv);
    //ponteiros para cstrings
    int cargc = static_cast<int>(cleaned.size());
    std::vector<char*> cargv(cargc);
    for (int i = 0; i < cargc; ++i) cargv[i] = const_cast<char*>(cleaned[i].c_str());

    // permitir a sobreposição do CLI fallback para prompts de entrada
    if (cargc > 1) {
        main_choice = cargv[1];
        if (cargc > 2) {
            debug = std::stoi(cargv[2]);
        } else {
            std::cout << "nivel debug: ";
            std::cin >> debug;
        }
    } else {
        std::cout << "1 - modo de jogo:\n";
        std::cout << "2 - modo de teste 1:\n";
        std::cout << "3 - modo de teste 2:\n";
        std::cin >> main_choice;

        std::cout << "nivel debug: ";
        std::cin >> debug;
    }

    if (main_choice == "1") {
        std::cout << "Escolhe um modo de jogo:\n";
        std::cout << "1: Humano vs Humano\n";
        std::cout << "2: Humano (Jogador 1) vs IA (Jogador 2)\n";
        std::cout << "3: Humano (Jogador 2) vs IA (Jogador 1)\n";
        std::cout << "4: IA vs IA\n";
        std::cout << "Opção: ";
        std::string choice;
        std::cin >> choice;

        if (choice == "1") mode = "human_vs_human";
        else if (choice == "2") mode = "human_first";
        else if (choice == "3") mode = "ai_first";
        else if (choice == "4") mode = "ai_vs_ai";
        else {
            std::cout << "Entrada inválida, usando modo padrão: Humano vs IA (Jogador 2)\n";
            mode = "ai_first";
        }

        std::cout << "1 - Novo tabuleiro\n";
        std::cout << "2 - Carregar de CSV\n";
        std::string board_choice;
        std::cin >> board_choice;

        if (board_choice == "1") {
            std::cout << "Escolhe o número de linhas (mínimo 5): ";
            std::cin >> rows;
            std::cout << "Escolhe o número de colunas (mínimo 5): ";
            std::cin >> cols;

            if (rows < 5 || cols < 5) {
                std::cout << "Tamanho inválido. Usando 7x7 por padrão.\n";
                rows = cols = 7;
            }

            GameController controller(mode, rows, cols);
            controller.run();
        } else if (board_choice == "2") {
            std::string path;
            std::cout << "Caminho para CSV: ";
            std::cin >> path;

            std::regex size_pattern(R"((\d+)x(\d+))");
            std::smatch match;
            if (!std::regex_search(path, match, size_pattern)) {
                std::cerr << "Erro: nome do arquivo deve conter formato NxM (ex: 7x7).\n";
                return 1;
            }
            int rows = std::stoi(match[1]);
            int cols = std::stoi(match[2]);

            int target_ply;
            std::cout << "Número da jogada onde o jogo deve começar: ";
            std::cin >> target_ply;

            Board board(rows, cols, false);

            std::ifstream file(path);
            if (!file) {
                std::cerr << "Erro ao abrir o arquivo.\n";
                return 1;
            }

            std::string line;
            std::getline(file, line);

            board.make_move({2, 4});

            int move_count = 0;
            while (move_count < target_ply && std::getline(file, line)) {
                std::stringstream ss(line);
                std::string ply_str, player, move_str, score;

                std::getline(ss, ply_str, ',');
                std::getline(ss, player, ',');
                std::getline(ss, move_str, ',');
                std::getline(ss, score, ',');

                if (move_str.length() < 2) continue;

                int col = move_str[0] - 'a';
                int row = rows - (move_str[1] - '0');

                std::cout << "(" << move_str[0] << move_str[1] << ") ->";
                std::cout << "(" << row << "," << col << ")\n";

                board.make_move({row, col});
                move_count++;
            }

            board.print();
            std::cout << "Jogo preparado na jogada " << move_count << ".\n";
            if (mode == "ai_first") {
                board.current_player = true;
            } else if (mode == "human_first") {
                board.current_player = false;
            }

            GameController controller(mode, rows, cols, board, move_count);
            controller.run();
        } else {
            std::cout << "Opção inválida.\n";
        }
    } else if (main_choice == "2") {
        std::cout << "Modo teste1\n";
        mode = "ai_vs_ai";
        rows = 7;
        cols = 7;

        int AI1_victory = 0;
        int AI2_victory = 0;

        Board board(rows, cols);
        auto valid_moves = board.get_valid_moves();

        // Defaults: MAX deterministic, MIN shuffled, noise 0.75 (ignored for non-noisy), no tie-shuffle
        OrderingPolicy pMax = OrderingPolicy::Deterministic;
        OrderingPolicy pMin = OrderingPolicy::Deterministic;
        double sigmaMax = 0.75, sigmaMin = 0.75;
        bool shuffleTiesOnly = false;

        // CLI: ./rastros 2 <debug> <pMax> <pMin> [sigmaMax] [sigmaMin] [shuffleTiesOnly]
        if (cargc >= 4) pMax = parse_policy(cargv[3]);
        if (cargc >= 5) pMin = parse_policy(cargv[4]);
        if (cargc >= 6) sigmaMax = std::stod(cargv[5]);
        if (cargc >= 7) sigmaMin = std::stod(cargv[6]);
        if (cargc >= 8) shuffleTiesOnly = parse_bool(cargv[7]);




        std::cout << "[Ordering] MAX=" << policy_repr(pMax, sigmaMax)
                  << ", MIN=" << policy_repr(pMin, sigmaMin);
        if (shuffleTiesOnly) std::cout << ", shuffle_ties_only=1";
        std::cout << "\n";


        for (std::pair<int, int> move : valid_moves) {
            TestController controller(mode, rows, cols, move, debug);
            controller.configure_ordering(pMax, pMin, sigmaMax, sigmaMin, shuffleTiesOnly);
            controller.configure_quiescence(false, false, 4, 2, 2);
            print_quiescence_header(/*qMax=*/false, /*qMin=*/false, 4, 2, 2);

            if (depthFlag)    controller.start_depth = *depthFlag;
            if (maxDepthFlag) controller.max_depth   = *maxDepthFlag;

            bool win = controller.run(1);
            if (win) AI1_victory++;
            else AI2_victory++;
        }

        std::cout << "Vitórias AI 1: " << AI1_victory << "\n";
        std::cout << "Vitórias AI 2: " << AI2_victory << "\n";
    } else {
        std::cout << "Modo teste2\n";
        mode = "ai_vs_ai";
        rows = 7;
        cols = 7;
        std::cout << " - Board: " << rows << "x" << cols << "\n";
        int AI1_victory = 0;
        int AI2_victory = 0;

        Board board(rows, cols);
        auto t1_t = std::chrono::high_resolution_clock::now();

        OrderingPolicy pMax = OrderingPolicy::Deterministic;
        OrderingPolicy pMin = OrderingPolicy::Deterministic;
        double sigmaMax = 0.75, sigmaMin = 0.75;
        bool shuffleTiesOnly = false;

        if (cargc >= 4) pMax = parse_policy(cargv[3]);
        if (cargc >= 5) pMin = parse_policy(cargv[4]);
        if (cargc >= 6) sigmaMax = std::stod(cargv[5]);
        if (cargc >= 7) sigmaMin = std::stod(cargv[6]);
        if (cargc >= 8) shuffleTiesOnly = parse_bool(cargv[7]);


        std::cout << "[Ordering] MAX=" << policy_repr(pMax, sigmaMax)
                  << ", MIN=" << policy_repr(pMin, sigmaMin);
        if (shuffleTiesOnly) std::cout << ", shuffle_ties_only=1";
        std::cout << "\n";

        int games = gamesFlag.value_or(100); 
        
        for (int i = 1; i <= games; i++) {
            auto t1_p = std::chrono::high_resolution_clock::now();
            std::cout << " - Jogo: " << i << "\n";
            TestController controller(mode, rows, cols, debug);
            controller.configure_ordering(pMax, pMin, sigmaMax, sigmaMin, shuffleTiesOnly);
            controller.configure_quiescence(false, false, 4, 2, 2);
            print_quiescence_header(/*qMax=*/false, /*qMin=*/false, 4, 2, 2);
            if (depthFlag)    controller.start_depth = *depthFlag;
            if (maxDepthFlag) controller.max_depth   = *maxDepthFlag;

            bool win = controller.run(2);
            if (win) AI1_victory++;
            else AI2_victory++;
            auto t2_p = std::chrono::high_resolution_clock::now();
            auto elapsed_p = std::chrono::duration_cast<std::chrono::nanoseconds>(t2_p - t1_p).count();
            double elapsed_s = elapsed_p / 1e9;
            std::cout << "tempo jogo: " << elapsed_s << "\n\n";
            
            std::cout << "Vitórias AI 1: " << AI1_victory << "\n";
            std::cout << "Vitórias AI 2: " << AI2_victory << "\n";
        }

        auto t2_t = std::chrono::high_resolution_clock::now();
        auto elapsed_t = std::chrono::duration_cast<std::chrono::nanoseconds>(t2_t - t1_t).count();
        double elapsed = elapsed_t / 1e9;
        // std::cout << "Vitórias AI 1: " << AI1_victory << "\n";
        // std::cout << "Vitórias AI 2: " << AI2_victory << "\n";
        std::cout << "tempo total: " << elapsed << "\n";

    }

    return 0;
}
