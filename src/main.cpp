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
#include <functional>
#include <cctype>
#include "Heuristic1.hpp"
#include "Heuristic2.hpp"
#include "LogMsgs.hpp"


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

static std::optional<std::string> get_flag_str(int argc, char* argv[], const std::string& shortf, const std::string& longf) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == shortf || a == longf) {
            if (i + 1 < argc) return std::string(argv[i + 1]);
        } else if (a.rfind(longf + "=", 0) == 0) {
            return a.substr(longf.size() + 1);
        }
    }
    return std::nullopt;
}

static std::vector<std::string> strip_new_flags(int argc, char* argv[]) {
    std::vector<std::string> out;
    for (int i = 0; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-d" || a == "--depth" ||
            a == "-d1" || a == "--depth1" ||
            a == "-d2" || a == "--depth2" ||
            a == "-md" || a == "--max-depth" ||
            a == "-g" || a == "--games" ||
            a == "-r" || a == "--row" ||
            a == "-c" || a == "--col" ||
            a == "-h" || a == "--heur" ||
            a == "-h1" || a == "--heur1" ||
            a == "-h2" || a == "--heur2") {
            // skip this and the next (its value), if present
            ++i;
            continue;
        }
        // also skip --depth=, --max-depth=, etc.
        if (a.rfind("--depth=", 0) == 0 ||
            a.rfind("--depth1=", 0) == 0 ||
            a.rfind("--depth2=", 0) == 0 ||
            a.rfind("--max-depth=", 0) == 0 ||
            a.rfind("--games=", 0) == 0 ||
            a.rfind("--row=", 0) == 0 ||
            a.rfind("--col=", 0) == 0 ||
            a.rfind("--heur=", 0) == 0 ||
            a.rfind("--heur1=", 0) == 0 ||
            a.rfind("--heur2=", 0) == 0) {
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

static HeuristicCombo parse_combo(const std::string& s, HeuristicCombo fallback) {
    if (s.empty()) return fallback;
    char c = static_cast<char>(std::toupper(s.front()));
    switch (c) {
        case 'A': return HeuristicCombo::A;
        case 'B': return HeuristicCombo::B;
        case 'C': return HeuristicCombo::C;
        case 'D': return HeuristicCombo::D;
        case 'E': return HeuristicCombo::E;
        case 'F': return HeuristicCombo::F;
        case 'G': return HeuristicCombo::G;
        case 'H': return HeuristicCombo::H;
        case 'I': return HeuristicCombo::I;
        case 'J': return HeuristicCombo::J;
        default:  return fallback;
    }
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

// Configuração comum de ordenação e parâmetros de ruído
struct OrderingConfig {
    OrderingPolicy pMax = OrderingPolicy::Deterministic;
    OrderingPolicy pMin = OrderingPolicy::Deterministic;
    double sigmaMax = 0.75;
    double sigmaMin = 0.75;
    bool shuffleTiesOnly = false;
};

// Configuração comum de quiescence
struct QuiescenceConfig {
    bool enableMax = false;
    bool enableMin = false;
    int plies = 4;
    int swing = 2;
    int lowmob = 2;
};

static OrderingConfig parse_ordering_config(int cargc, const std::vector<char*>& cargv) {
    OrderingConfig cfg;
    if (cargc >= 4) cfg.pMax = parse_policy(cargv[3]);
    if (cargc >= 5) cfg.pMin = parse_policy(cargv[4]);
    if (cargc >= 6) cfg.sigmaMax = std::stod(cargv[5]);
    if (cargc >= 7) cfg.sigmaMin = std::stod(cargv[6]);
    if (cargc >= 8) cfg.shuffleTiesOnly = parse_bool(cargv[7]);
    return cfg;
}

static void log_ordering(const OrderingConfig& cfg) {
    std::cout << "[Ordering] MAX=" << policy_repr(cfg.pMax, cfg.sigmaMax)
              << ", MIN=" << policy_repr(cfg.pMin, cfg.sigmaMin);
    if (cfg.shuffleTiesOnly) std::cout << ", shuffle_ties_only=1";
    std::cout << "\n";
}

static void apply_ordering(TestController& controller, const OrderingConfig& cfg) {
    controller.configure_ordering(cfg.pMax, cfg.pMin, cfg.sigmaMax, cfg.sigmaMin, cfg.shuffleTiesOnly);
}

static void apply_quiescence(TestController& controller, const QuiescenceConfig& cfg) {
    controller.configure_quiescence(cfg.enableMax, cfg.enableMin, cfg.plies, cfg.swing, cfg.lowmob);
    print_quiescence_header(cfg.enableMax, cfg.enableMin, cfg.plies, cfg.swing, cfg.lowmob);
}

static void apply_depth_overrides(TestController& controller,
                                  const std::optional<int>& depthFlag,
                                  const std::optional<int>& maxDepthFlag,
                                  const std::optional<int>& depthFlag1,
                                  const std::optional<int>& depthFlag2,
                                  const std::optional<int>& maxDepthFlag1,
                                  const std::optional<int>& maxDepthFlag2) {
    int startGlobal = depthFlag.value_or(controller.get_start_depth());
    int maxGlobal   = maxDepthFlag.value_or(controller.get_max_depth());
    controller.set_depth_limits(startGlobal, maxGlobal);

    int start1 = depthFlag1.value_or(startGlobal);
    int start2 = depthFlag2.value_or(startGlobal);
    int max1   = maxDepthFlag1.value_or(maxGlobal);
    int max2   = maxDepthFlag2.value_or(maxGlobal);

    controller.set_depth_limits_p1(start1, max1);
    controller.set_depth_limits_p2(start2, max2);
}

// Executa um jogo de teste com temporização e devolve true se a AI1 venceu
template <typename Factory>
static bool run_ai_game(Factory&& makeController,
                        int runMode,
                        const OrderingConfig& ordCfg,
                        const QuiescenceConfig& qCfg,
                        const std::optional<int>& depthFlag,
                        const std::optional<int>& maxDepthFlag,
                        const std::optional<int>& depthFlag1,
                        const std::optional<int>& depthFlag2,
                        const std::optional<int>& maxDepthFlag1,
                        const std::optional<int>& maxDepthFlag2) {
    auto t1 = std::chrono::high_resolution_clock::now();
    TestController controller = makeController();
    apply_ordering(controller, ordCfg);
    apply_quiescence(controller, qCfg);
    apply_depth_overrides(controller, depthFlag, maxDepthFlag, depthFlag1, depthFlag2, maxDepthFlag1, maxDepthFlag2);

    bool win = controller.run(runMode);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    double elapsed_s = elapsed / 1e9;
    std::cout << "tempo jogo: " << elapsed_s << "\n\n";
    return win;
}



int main(int argc, char* argv[]) {
    int rows = 7, cols = 7; // defaults
    std::string mode;
    std::string main_choice;
    int debug = 0;
    HeuristicCombo combo_p1 = HeuristicCombo::G; // default -> Heuristic1 combo A
    HeuristicCombo combo_p2 = HeuristicCombo::G; // default -> Heuristic2 combo C

    auto depthFlag    = get_flag_int(argc, argv, "-d",  "--depth");
    auto maxDepthFlag = get_flag_int(argc, argv, "-md", "--max-depth");
    auto depthFlag1   = get_flag_int(argc, argv, "-d1", "--depth1");
    auto depthFlag2   = get_flag_int(argc, argv, "-d2", "--depth2");
    auto maxDepthFlag1= get_flag_int(argc, argv, "-md1","--max-depth1");
    auto maxDepthFlag2= get_flag_int(argc, argv, "-md2","--max-depth2");
    auto gamesFlag    = get_flag_int(argc, argv, "-g",  "--games");
    auto rowFlag      = get_flag_int(argc, argv, "-r","--row");
    auto colFlag      = get_flag_int(argc, argv, "-c","--col");
    auto heurFlagBoth = get_flag_str(argc, argv, "-h",  "--heur");
    auto heurFlagP1   = get_flag_str(argc, argv, "-h1", "--heur1");
    auto heurFlagP2   = get_flag_str(argc, argv, "-h2", "--heur2");

    if (depthFlag && !maxDepthFlag) maxDepthFlag = depthFlag;
    if (depthFlag1 && !maxDepthFlag1) maxDepthFlag1 = depthFlag1;
    if (depthFlag2 && !maxDepthFlag2) maxDepthFlag2 = depthFlag2;
    
    if (heurFlagBoth) {
        combo_p1 = combo_p2 = parse_combo(*heurFlagBoth, combo_p1);
    }
    if (heurFlagP1) combo_p1 = parse_combo(*heurFlagP1, combo_p1);
    if (heurFlagP2) combo_p2 = parse_combo(*heurFlagP2, combo_p2);
    LogMsgs::out() << "[heur] P1=" << heuristic_combo_label(combo_p1)
                 << " P2=" << heuristic_combo_label(combo_p2) << "\n";
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

            // impressão passa a ser feita dentro do GameController::run
            std::cout << "Jogo preparado na jogada " << move_count << ".\n";
            if (mode == "ai_first") {
                board.set_current_player_from_int(1); // MAX começa
            } else if (mode == "human_first") {
                board.set_current_player_from_int(2); // MIN começa
            }

            GameController controller(mode, rows, cols, board, move_count);
            controller.run();
        } else {
            std::cout << "Opção inválida.\n";
        }
    } else if (main_choice == "2") {
        std::cout << "Modo teste1\n";
        mode = "ai_vs_ai";
        if (rowFlag && colFlag){
            rows = *rowFlag;
            cols = *colFlag;
        }

        int AI1_victory = 0;
        int AI2_victory = 0;

        Board board(rows, cols);
        auto valid_moves = board.get_valid_moves();

        OrderingConfig ordCfg = parse_ordering_config(cargc, cargv);
        QuiescenceConfig qCfg{};
        log_ordering(ordCfg);

        for (std::pair<int, int> move : valid_moves) {
            bool win = run_ai_game(
                [&]() { return TestController(mode, rows, cols, move, debug, combo_p1, combo_p2); },
                /*runMode=*/1,
                ordCfg,
                qCfg,
                depthFlag,
                maxDepthFlag,
                depthFlag1,
                depthFlag2,
                maxDepthFlag1,
                maxDepthFlag2
            );
            if (win) AI1_victory++;
            else AI2_victory++;
        }

        std::cout << "Vitórias AI 1: " << AI1_victory << "\n";
        std::cout << "Vitórias AI 2: " << AI2_victory << "\n";
    } else {
        std::cout << "Modo teste2\n";
        mode = "ai_vs_ai";
        if (rowFlag && colFlag){
            rows = *rowFlag;
            cols = *colFlag;
        }
        std::cout << " - Board: " << rows << "x" << cols << "\n";
        int AI1_victory = 0;
        int AI2_victory = 0;

        Board board(rows, cols);
        auto t1_t = std::chrono::high_resolution_clock::now();

        OrderingConfig ordCfg = parse_ordering_config(cargc, cargv);
        QuiescenceConfig qCfg{};
        log_ordering(ordCfg);

        int games = gamesFlag.value_or(100); 
        
        for (int i = 1; i <= games; i++) {
            std::cout << " - Jogo: " << i << "\n";
            bool win = run_ai_game(
                [&]() { return TestController(mode, rows, cols, debug, combo_p1, combo_p2); },
                /*runMode=*/2,
                ordCfg,
                qCfg,
                depthFlag,
                maxDepthFlag,
                depthFlag1,
                depthFlag2,
                maxDepthFlag1,
                maxDepthFlag2
            );
            if (win) AI1_victory++;
            else AI2_victory++;
            
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
