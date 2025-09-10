// ============================================================================
// AI.cpp — Motor de IA para o jogo Rastros(Minimax + Alpha-Beta)
// ----------------------------------------------------------------------------
// 
// - Níveis de heurística (factory + registo dinâmico por nível)
// - Escolha de jogada (choose_move) com ordenação/aleatoriedade controlada
// - Minimax com poda alfa–beta + memoização por StateKey (transposition table)
// - Heurísticas elementares: caminhos mínimos, paridade, armadilhas, cantos,
//   bónus de quadrante, mobilidade e distâncias de Chebyshev.
// - Compatibilidade com testes: RNG determinístico quando RASTROS_TESTS está ativo.
// ============================================================================


#include "AI.hpp"
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <queue>
#include <iostream>
#include <chrono> 
#include <random>
#include <map>
#include <unordered_map>
#include <string>


int AI::s_rounds = 0;
int AI::count_visited = 0;

uint64_t AI::vs_lookups = 0;
uint64_t AI::vs_hits = 0;
uint64_t AI::vs_inserts = 0;


//para debug tree
static inline std::string indent_rails(int depth) {
    std::string p;
    for (int i = 0; i < depth; ++i) p += "│ ";
    return p;
}
static inline std::string branch_prefix(int depth, bool last) {
    return indent_rails(depth) + (last ? "└── " : "├── ");
}





namespace {
    using Key = std::string;

    static std::unordered_map<Key, std::vector<MoveScore>> s_order_cache;
    static std::unordered_map<Key, int>                    s_heuristic_cache;

    // Cria uma chave compacta a partir do tabuleiro, lado (MAX/MIN) e profundidade.
    // A chave codifica: (is_max, depth, marker, board)
    static Key make_cache_key(const Board& b, bool is_max, int depth, int player_search) {
        std::string s;
        s.reserve(static_cast<size_t>(b.rows) * static_cast<size_t>(b.cols) + 32);
        s.push_back(is_max ? '1' : '0');
        s.push_back('#');
        s.append(std::to_string(depth));
        s.push_back('#');
        // NEW: player_search component
        s.append(std::to_string(player_search)); s.push_back('#');
        // marker
        s.append(std::to_string(b.marker.first));
        s.push_back(',');
        s.append(std::to_string(b.marker.second));
        s.push_back('#');
        // grid
        for (int r = 0; r < b.rows; ++r) {
            for (int c = 0; c < b.cols; ++c) {
                s.push_back(b.grid[r][c] ? '1' : '0');
            }
            s.push_back('|');
        }
        return s;
    }
}


StateKey AI::state_key(const Board& board, bool is_max, int player_search) {
    return {board.grid, board.marker, is_max,player_search};
}

// Limpa as caches locais
void AI::clear_ordering_caches() {
    // std::cout <<s_order_cache.size()<<" - "<<s_heuristic_cache.size()<<"\n";
    s_order_cache.clear();
    s_heuristic_cache.clear();
    // std::cout << "caches cleared!\n";
    // std::cout <<s_order_cache.size()<<s_heuristic_cache.size()<<"\n";
}

void AI::clear_tt() {
    // std::cout <<tt.size()<<"\n";
    tt.clear();
    tt.rehash(0);
    // std::cout << "TT cleared!\n";
    // std::cout <<tt.size()<<"\n";
}

// Configuração da política de ordenação de sucessores e parâmetros associados
void AI::set_ordering_policy(OrderingPolicy p) { ordering_policy = p; }
void AI::set_order_noise(double sigma)         { order_noise_sigma = std::max(0.0, sigma); }
void AI::set_shuffle_ties_only(bool enabled)   { shuffle_ties_only = enabled; }

// Conversão de enum para etiqueta (para compor a chave de cache de ordenação)
static char policy_tag(OrderingPolicy p) {
    switch (p) {
        case OrderingPolicy::Deterministic: return 'D';
        case OrderingPolicy::ShuffleAll:    return 'S';
        case OrderingPolicy::NoisyJitter:   return 'N';
    }
    return 'D';
}

void AI::reset_ordering_stats() {
    ord_max_ = OrderingStats{};
    ord_min_ = OrderingStats{};
}


// para debug estatístico da ordenação de sucessores
void AI::print_ordering_stats() const {
    auto printOne = [&](const char* label, const OrderingStats& s){
        std::cout << "[order] " << label << " nodes=" << s.nodes
                  << " cutoffs=" << s.cutoffs;
        if (s.cutoffs) {
            double avg_cut = double(s.cutoff_idx_sum) / double(s.cutoffs);
            double pct_first = 100.0 * double(s.cutoff_first_child) / double(s.cutoffs);
            std::cout << " avgCutIdx=" << avg_cut
                      << " firstChildCut=" << pct_first << "%";
        }
        if (s.no_cutoff_nodes) {
            double avg_best = double(s.best_idx_sum) / double(s.no_cutoff_nodes);
            std::cout << " avgBestIdx(no-cut)=" << avg_best;
        }
        std::cout << "\n";
    };
    printOne("MAX-to-move", ord_max_);
    printOne("MIN-to-move", ord_min_);
}


// ----------------------------------------------------------------------------
// Suporte a testes (RASTROS_TESTS):
// - Fornece um RNG determinístico para resultados reprodutíveis em testes.
// - Produz a mesma sequência independentemente da plataforma.
// ----------------------------------------------------------------------------
#ifdef RASTROS_TESTS
namespace {
  static std::mt19937& test_rng() {
    static std::mt19937 g(123456789u); // pick any constant seed
    return g;
  }
  // (optional) allow tests to reseed between cases
  inline void test_reset_rng(unsigned seed) { test_rng().seed(seed); }
}
#endif


//AI::AI(bool is_max, int max_depth) : is_max(is_max), max_depth(max_depth) {}


// ----------------------------------------------------------------------------
// Construtor principal:
// - 'is_max' indica se a instância joga como MAX (Jogador 1) ou MIN (Jogador 2).
// - 'max_depth' define a profundidade máxima do minimax.
// - Por omissão, liga a 'default_heuristic' via std::function para permitir
//   injeção de heurísticas alternativas (ex.: via fábrica por nível).
// ----------------------------------------------------------------------------
AI::AI(bool is_max, int max_depth)
    : is_max(is_max), max_depth(max_depth) {
    // fallback to default heuristic
    heuristic = [this](const Board& board, bool max) {
        return this->default_heuristic(board, max);
    };
        AI::count_visited = 0;
        AI::vs_lookups = 0;
        AI::vs_hits = 0;
        AI::vs_inserts = 0;

}

// ----------------------------------------------------------------------------
// Construtor com injeção de heurística + nível de debug:
// - Permite ao test runner/fábrica passar uma função heurística concreta.
// - 'debug_level' controla a verbosidade (1..5) dos logs no terminal.
// - Recomeça contadores globais
// ----------------------------------------------------------------------------
AI::AI(bool is_max, int max_depth, std::function<int(const Board&, bool)> heuristic_func,int debug_level)
    : is_max(is_max), max_depth(max_depth), heuristic(heuristic_func) ,debug_level(debug_level){
        AI::count_visited = 0;
        AI::vs_lookups = 0;
        AI::vs_hits = 0;
        AI::vs_inserts = 0;

    }

// Encaminha para a heurística configurada
int AI::total_heuristic(const Board& board, bool is_max) {
    // Encaminha para a função heurística configurada (por defeito a 'default_heuristic').
    // Permite trocar heurísticas por nível/mode sem alterar o resto do motor.
    return heuristic(board, is_max);
}

// Mapa global de níveis → função heurística
std::map<int, std::function<int(const Board&, bool)>> AI::heuristic_levels;

// getter para obter o turno
// criado para experimentar ativação de heurísticas em diferentes fases do jogo
// não está a ser utilizado
int AI::rounds() { return s_rounds; }

// ----------------------------------------------------------------------------
// register_heuristics():
// - Regista as heurísticas disponíveis por nível (1..10).
// - Níveis simples (2..5) calculam medidas básicas; níveis altos delegam
//   para a heurística "full combo" (default_heuristic) por agora.
// ----------------------------------------------------------------------------
void AI::register_heuristics() {
    heuristic_levels[1] = [](const Board& b, bool is_max) {
        //std::cout<<"level: "<<0<<"\n";
        return 0;

    };

    heuristic_levels[2] = [](const Board& b, bool is_max) {
        auto pos = b.get_marker();
        int dist = AI::h_distance(b, pos, is_max);
        dist = is_max ? -dist : dist;
        return dist;

    };

    heuristic_levels[3] = [](const Board& b, bool is_max) {
        auto pos = b.get_marker();
        int dist = AI::h_distance(b, pos, is_max);
        dist = is_max ? -dist : dist;
        return dist;


    };

    heuristic_levels[4] = [](const Board& b, bool is_max) {
        auto pos = b.get_marker();
        int dist = AI::h_distance(b, pos, is_max);
        int o_dist = AI::h_distance(b, pos, !is_max);
        dist = is_max ? -dist : dist;
        o_dist = is_max ? dist : -dist;
        return dist-o_dist;
    };

    heuristic_levels[5] = [](const Board& b, bool is_max) {
        int dist = std::abs(b.shortest_path_to_goal(is_max));

        int moves = (int)b.get_valid_moves().size();
        return is_max ? -dist  : dist;
    };

    heuristic_levels[6] = [](const Board& b, bool is_max) {
 
        return b.get_valid_moves().size();
    };

    heuristic_levels[7] = [](const Board& b, bool is_max) {
            auto reach = b.compute_reachability();

            int Dmax = reach.h1;
            int Dmin = reach.h5;

            return Dmax+Dmin;
    };

    heuristic_levels[8] = [](const Board& b, bool is_max) {
        AI temp(is_max, 10); 
        return temp.total_heuristic(b, is_max); 
    };

    heuristic_levels[9] = [](const Board& b, bool is_max) {
        AI temp(is_max, 10); 
        return temp.total_heuristic(b, is_max);  
    };

    heuristic_levels[10] = [](const Board& b, bool is_max) {
        AI temp(is_max, 10); //profundidade não atua aqui 
        // //std::cout <<"th"<<temp.total_heuristic(b, is_max)<<"\n";
        return temp.total_heuristic(b, is_max); 
    };

}



// ----------------------------------------------------------------------------
// create_with_level():
// - Fábrica que constrói uma AI com a heurística correspondente ao 'level'.
// - Lança exceção se o nível não estiver registado (fail-fast).
// ----------------------------------------------------------------------------
std::unique_ptr<AI> AI::create_with_level(bool is_max, int depth, int level, int debug) {
    //std::cout<<"is_max "<<is_max<<" depth "<< depth<<" level "<<level<<" debug "<<debug<<"\n";
    auto it = heuristic_levels.find(level);
    // std::cout << "depth" << depth << "\n";
    if (it == heuristic_levels.end()) {
        throw std::invalid_argument("Heuristic level not found");
    }
    return std::make_unique<AI>(is_max, depth, it->second, debug);

}


// ----------------------------------------------------------------------------
// choose_move():
// - Ponto de entrada para a decisão de jogada da IA.
// - Opcionalmente filtra/aleatoriza a primeira jogada evitando jogadas,
// que dêem vitória imediata ao adversário (casas adjacentes ao objectivo adversário)
// - No round inicial (rounds == 0) e com MAX a jogar, pode aplicar um filtro
// para evitar casas adjacentes ao objetivo do adversário, escolhendo aleatoriamente
// entre as seguras (mantendo variedade). -> talvez retirar e fazer procura desde inicio
// - Em seguida, ordena os lances (move ordering) para potenciar os cortes alfa–beta
//   e corre minimax com profundidade 'depth_override' ou 'max_depth' por defeito.
// ----------------------------------------------------------------------------
std::pair<int, int> AI::choose_move(Board& board, int depth_override, int rounds) {

     AI::s_rounds = rounds;
    // --- Round 0 optional filter (keep original behavior) ---
    if (rounds == 0) {//&& board.current_player == true) {
        const auto& start = board.marker;
        std::vector<std::pair<int, int>> initial_moves;

        static const int dr[8] = {-1, 1,  0, 0, -1, -1, 1, 1};
        static const int dc[8] = { 0, 0, -1, 1, -1,  1, -1, 1};

        const int goal_r = 0;
        const int goal_c = board.cols - 1;

        for (int i = 0; i < 8; ++i) {
            int r = start.first + dr[i];
            int c = start.second + dc[i];

            if (r >= 0 && r < board.rows && c >= 0 && c < board.cols &&
                board.grid[r][c] == 1) {

                // Evita casas imediatamente adjacentes ao objetivo adversário
                int dist_to_goal = std::max(std::abs(goal_r - r), std::abs(goal_c - c));
                if (dist_to_goal <= 1) continue;

                initial_moves.emplace_back(r, c);
            }
        }

        if (!initial_moves.empty()) {
            #ifdef RASTROS_TESTS
              auto& gen = test_rng();
            #else
              std::random_device rd;
              std::mt19937 gen(rd());
            #endif
            std::uniform_int_distribution<int> dis(0, static_cast<int>(initial_moves.size() - 1));
            auto move = initial_moves[dis(gen)];
            if (debug_level == 1) {
                std::cout << "[MAX]First AI controled move : (" << move.first << ", " << move.second << ")\n";
            }
            return move;
        }
    }

    // Caches por raiz (limpas a cada chamada)
    clear_ordering_caches();


    const int depth_used = (depth_override != -1) ? depth_override : max_depth;
    const auto pos = board.get_marker();
    const char* player = is_max ? "MAX" : "MIN";
    int player_search = is_max ? 1 : 2;

    // Geração ordenada de sucessores na raiz (move ordering)
    const auto& rootSuccessors = ordered_children(board, is_max, /*depth*/0, depth_used, player_search);

    // if (debug_level >= 2) {
    //     std::cout << "|" << "-" << "(" << pos.first << "," << pos.second << ")->";
    //     for (const auto& ms : rootSuccessors) {
    //         std::cout << "(" << ms.move.first << ", " << ms.move.second << "), ";
    //     }
    //     std::cout << "\n";
    //     std::cout << "depth: " << depth_override << "\n";
    // }

    if (debug_level >= 2) {
        std::cout << indent_rails(0)
                  << "(" << pos.first << "," << pos.second << ")->";
        for (const auto& ms : rootSuccessors) {
            std::cout << "(" << ms.move.first << ", " << ms.move.second << "), ";
        }
        std::cout << "\n";
        // std::cout << "depth: " << depth_override << "\n";
    }




    if (rootSuccessors.empty()) return {-1, -1}; // sem jogadas possíveis

    int best_score = is_max ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    std::pair<int, int> best_move = rootSuccessors.front().move; // fallback (primeiro)

    // nº de traços para árvore de debug
    // const int dash = std::max(0, depth_used - 1);

    for (const auto& ms : rootSuccessors) {
        Board tmp = board;
        tmp.make_move(ms.move);

        // Atalho: se o sucessor é terminal e é vitória para quem acabou de jogar, retorna já.
        if (tmp.is_terminal()) {
            int v = adjust_terminal_score(evaluate_terminal(tmp, /*perspetiva do sucessor*/ !is_max),
                                          /*depth*/ 1);
            if ((is_max && v > 0) || (!is_max && v < 0)) {
                if (debug_level == 1) {
                    std::cout << "****[" << (is_max ? "MAX" : "MIN")
                              << "] Immediate win found at root: (" << ms.move.first
                              << "," << ms.move.second << ") " << v << "\n";
                }
                return ms.move;  // jogada vencedora a profundidade 1
            }
            // Caso contrário (terminal mas perde), continua a avaliar os restantes
        }

        // if (debug_level >= 3) {
        //     std::cout << "|\n";
        //     std::cout << "|" << player << " trying move to: (" << ms.move.first << "," << ms.move.second << ")\n";
        // }

        if (debug_level >= 2) {
            bool last_root_child = (&ms == &rootSuccessors.back());
            std::cout << branch_prefix(0, last_root_child)
                      << player << " trying move to: ("
                      << ms.move.first << "," << ms.move.second << ")\n";
        }

        // if (debug_level >= 1) {
        // std::cout << "max_depth: " << max_depth << "\n";
        // }

        int score = minimax(tmp,
                            /*is_max=*/!is_max,
                            /*depth=*/1,
                            std::numeric_limits<int>::min(),
                            std::numeric_limits<int>::max(),
                            depth_used,
                            player_search);

        // if (debug_level >= 2) {
        //     std::cout << "|" << std::string(static_cast<size_t>(dash), '-')
        //               << "score for: (" << ms.move.first << "," << ms.move.second << ")-> "
        //               << score << "\n";
        // }

        if (debug_level >= 2) {
            bool last_root_child = (&ms == &rootSuccessors.back());
            std::cout << branch_prefix(0, last_root_child)
                      // << std::string(static_cast<size_t>(dash), '-') // same visual idea
                      << "score for: (" << ms.move.first << "," << ms.move.second << ")-> "
                      << score << "\n";
        }



        if ((is_max && score > best_score) || (!is_max && score < best_score)) {
            best_score = score;
            best_move = ms.move;
        }
    }

    if (debug_level >= 1) {
        std::cout << "****[" << player << "] Best move selected: (" << best_move.first
                  << "," << best_move.second << ") " << best_score << "\n";
        
    }

    unplayable_cells_count++;
    return best_move;
}


// ----------------------------------------------------------------------------
// minimax(board, is_max, depth, alpha, beta, max_depth):
// - Implementa Minimax cokm cortes Alpha–Beta e memorização de estados visitados.
// - A chave de transposição (StateKey) inclui: grid, marker, is_max, e mapeia para
// uma entrada na tabela de transposição que contém( val,depth, bound)
// - Se terminal: 'evaluate_terminal' devolve ±1000 (ou 0).
// - Se profundidade limite: usa 'total_heuristic'. -> podendo estender
// via quiescence search caso esteja ativa. (não está a ser utilizada)
// - Ordenação de jogadas para melhorar eficácia dos cortes.
// ----------------------------------------------------------------------------
int AI::minimax(Board board, bool is_max, int depth, int alpha, int beta, int max_depth, int player_search) {
    
    // std::cout << "player_search"<<player_search<<"\n";

    // contadores repetidos ->limpar
    eval_successors++;
    vs_lookups++;

    // Profundidade mínima ainda necessária
    const int required = max_depth - depth;

    StateKey key = state_key(board, is_max,player_search); // chave única para estado

    //Consulta à tabela de transposição
    if (auto it = tt.find(key); it != tt.end()) {

        const TTEntry& e = it->second;
        if (e.depth >= required) {// só é válido se a entrada tiver profundidade suficiente
            vs_hits++;
            count_visited++;
            if (debug_level >= 5 ) {
                std::cout << indent_rails(depth)
                          << "[hit] key=" << key
                          << " d_req=" << required
                          << " got=" << e.depth
                          << " val=" << e.value
                          << " bound=" << (e.bound==TTBound::Exact ? "E" : e.bound==TTBound::Lower ? "L" : "U")
                          << "\n";
            }

            // Lógica de utilização do bound
            if (e.bound == TTBound::Exact) return e.value; // valor exato
            if (e.bound == TTBound::Lower && e.value >= beta)  return e.value; // fail-high cutoff
            if (e.bound == TTBound::Upper && e.value <= alpha) return e.value; // fail-low  cutoff
            // Caso contrário, prossegue pesquisa normal
        }else{
            if (debug_level >= 5) {
                std::cout << indent_rails(depth)
                        <<"hit_not_val: "<< key
                        << " d_req=" << required
                        <<"\n";
            }
        }
    }


    // estado terminal: vitória, derrota
    if (board.is_terminal()) {
        int val = evaluate_terminal(board, is_max);

        if (debug_level >= 4) {
            std::cout << indent_rails(depth)
                      << "terminal - (" << key.marker.first << "," << key.marker.second << ")\n";
        }

        TTEntry e{ val, required, TTBound::Exact };
        tt[key] = e; vs_inserts++;

        if (debug_level >= 5) {
            std::cout << indent_rails(depth)
                      << "[save] key=" << key
                      << " d_req=0"
                      << " val=" << val << " (leaf, Exact)\n";
        }

        return val;
    }

    //desligar quiescence para a app, não foi criado ainda binding
    //Pode ser ligada e desligada no TestControler
    const bool use_quiescence = false;
    // estado nó folha: limite de profundidade atingido
    if (depth >= max_depth) {
        if (use_quiescence) {// ativa pesquisa em quiescência (não está a ser utilizada- precisa de afinação)

            if (debug_level >= 2 && depth >= max_depth && use_quiescence) {
                std::cout << "[Q] entering quiescence at depth=" << depth << "\n";
            }
            // estende seletivamente até a posição "acalmar"
            int qval = quiescence(board, is_max, alpha, beta, /*qdepth=*/0, /*base_depth=*/depth);

            //se for utilizada é preciso gravar chave aqui

            return qval;
        } else {
            // Avaliação heurística
            const Key hkey = make_cache_key(board, is_max, max_depth,player_search);
            // std::cout<<"hkey: "<< hkey <<"\n";
            int val;
            if (auto it = s_heuristic_cache.find(hkey); it != s_heuristic_cache.end()){
                val = it->second;
                std::cout<<"cached_value: "<< val <<"\n";

            } 
            else { val = total_heuristic(board, is_max); s_heuristic_cache[hkey] = val; 
                    // std::cout<<"heur_calc: "<< val <<"\n";
            }
            TTEntry e{ val, 0, TTBound::Exact };
            tt[key] = e; vs_inserts++;
            if (debug_level >= 5) {
                std::cout << "[save] key=" << key
                          << " d_req=0"
                          << " val=" << val << " (leaf, Exact)\n";
            }

            
            return val;
        }
    }

    //Expansão de nós internos
    const auto pos = board.get_marker();
    const char* player   = is_max ? "MAX" : "MIN";
    const char* opponent = is_max ? "MIN" : "MAX";

    // ordenação de sucessores por heurística
    const auto& successors = ordered_children(board, is_max, depth, max_depth, player_search);

    // debug- estatísticas de ordenação
    auto& OST = stats_for(is_max); 
    OST.nodes++;

    int child_idx = 0;
    int best_idx = -1;
    bool had_cutoff = false;


    if (debug_level >= 3) {
        std::cout << indent_rails(depth)
                  << "(" << pos.first << "," << pos.second << ")->";
        for (const auto& ms : successors) {
            std::cout << "(" << ms.move.first << ", " << ms.move.second << "), ";
        }
        std::cout << "eval [" << opponent << "] position to [" << player << "]\n";
    }

    gen_successors += successors.size();

    int best = is_max ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();

    for (const auto& ms : successors) {
        Board tmp = board;
        tmp.make_move(ms.move);

        int score = minimax(tmp, !is_max, depth + 1, alpha, beta, max_depth, player_search);
        int adj_score = adjust_terminal_score(score, depth);
        score = adj_score; // corrigir esta atribuição

        if (debug_level >= 2  &&depth <= 1 && debug_level <3 ) {
        std::cout << "RS (" << ms.move.first << "," << ms.move.second 
                  << ") -> " << score << "\n";
        }

        if (debug_level >= 3) {
            bool last_child = (&ms == &successors.back());
            std::cout << branch_prefix(depth, last_child)
                      << "(" << ms.move.first << ", " << ms.move.second << ") "
                      << adj_score<< "\n";
        }

        

        //cortes alfa-beta
        if (is_max) {
            if (score >= beta) {
                prunes++;
                // if (debug_level >= 4) {
                //     std::cout << "|->" << std::string(depth, '-') << "pruned:" << score << "\n";
                // }
                if (debug_level >= 4) {
                    std::cout << indent_rails(depth)
                              << "pruned: " << score << "\n";
                }

                //variáveis de debug
                had_cutoff = true;
                OST.cutoffs++;
                OST.cutoff_idx_sum += child_idx;
                if (child_idx == 0) OST.cutoff_first_child++;

                TTEntry e{ score, required, TTBound::Lower };
                tt[key] = e; 
                vs_inserts++;
                if (debug_level >= 5) {
                    std::cout << "[save] key=" << key
                              << " d_req=" << required
                              << " val=" << score << " (cutoff, Lower)\n";
                }
                return score;
            }
            alpha = std::max(alpha, score);
            best  = std::max(best,  score);
        } else {
            if (score <= alpha) {
                prunes++;
                //variáveis de debug
                had_cutoff = true;
                OST.cutoffs++;
                OST.cutoff_idx_sum += child_idx;
                if (child_idx == 0) OST.cutoff_first_child++;

                if (debug_level >= 4) {
                    std::cout << indent_rails(depth)
                              << "pruned: " << score << "\n";
                }

                TTEntry e{ score, required, TTBound::Upper };
                tt[key] = e; 
                vs_inserts++;
                if (debug_level >= 5) {
                    std::cout << "[save] key=" << key
                              << " d_req=" << required
                              << " val=" << score << " (cutoff, Upper)\n";
                }

                return score;
            }
            beta = std::min(beta, score);
            best = std::min(best, score);
        }
        child_idx++;

    }

    // Caso não haja corte guarda estatísticas de melhor sucessor
    if (!had_cutoff && best_idx >= 0) {
        OST.no_cutoff_nodes++;
        OST.best_idx_sum += best_idx;
    }


    TTEntry e{ best, required, TTBound::Exact };
    tt[key] = e; vs_inserts++;
    if (debug_level >= 5) {
        std::cout << "[save] key=" << key
                  << " d_req=" << required
                  << " val=" << best << " (final, Exact)\n";
    }


    return best;
}


int AI::adjust_terminal_score(int score, int depth) {
    // - Quando detetados estados terminais (±1000), ajusta-se a pontuação
    // pela profundidade, preferindo vitórias mais rápidas e derrotas mais tardias
    if (score == 1000) return score - depth;
    if (score == -1000) return score + depth;
    return score;
}

int AI::evaluate_terminal(const Board& board, bool is_max) {
    // - Converte estado terminal em valor numérico:
    //   1000  -> MAX alcançou o seu objetivo
    //  -1000  -> MIN alcançou o seu objetivo
    //   ±1000 -> Sem jogadas: quem está de turno perde (sinal depende de 'is_max').
    //   0     -> Não-terminal (fallback — em princípio não chega aqui).
    if (board.marker == std::make_pair(board.rows - 1, 0)) return 1000;
    if (board.marker == std::make_pair(0, board.cols - 1)) return -1000;
    if (board.get_valid_moves().empty()) return is_max ? -1000 : 1000;
    return 0;
}



// ---  Penalização de casa adjacente diagonalmente ao objetivo ------------
static int h_diag_block_goal(const Board& b) {
    
    if (b.is_terminal()) return 0; // não aplicar penalização a estados terminais
    
    // obter distâncias da posição atual
    auto reach = b.compute_reachability();

    const bool max_to_move = b.current_player;

    // Não penalizar casa se a próxima jogada for vencedora
    const bool max_can_win_next =  max_to_move && (std::abs(reach.h1) == 1);
    const bool min_can_win_next = !max_to_move && (std::abs(reach.h5) == 1);

    const int R = b.rows, C = b.cols;


    //  - MAX  (R-1, 0) -> diagonal adjacente: (R-2, 1)
    //  - MIN  (0, C-1) -> diagonal adjacente: (1,   C-2)
    const int rMax = R - 2, cMax = 1;
    const int rMin = 1,     cMin = C - 2;

    auto in_bounds = [&](int r, int c){
        return r >= 0 && r < R && c >= 0 && c < C;
    };

    // valor da penalização
    const int P = 40;

    int score = 0;

    // casa junto do obj de MAX bloqueada? penaliza, excepto se ganha na jogada seguinte
    if (in_bounds(rMax, cMax) && b.grid[rMax][cMax] == 0) {
        if (!max_can_win_next) score -= P;
    }

    // casa junto do obj de MIN bloqueada? penaliza, excepto se ganha na jogada seguinte
    if (in_bounds(rMin, cMin) && b.grid[rMin][cMin] == 0) {
        if (!min_can_win_next) score += P;
    }

    return score;
}


//heuristica maxima ativa na app
int AI::default_heuristic(const Board& board, bool is_max) {
    // - Combina sub-heurísticas para uma avaliação equilibrada do estado.
    // - Dmax -> caminho mínimo p/ obj MAX
    // - Dmin  = caminho mínimo p/ obj MIN
    // - Par  = Paridade de casas livres quando objetivos são inalcançáveis
    // - AvCels - numero de casas disponiveis para jogar
    // - BlkDiag - casa diagonal adjacente ao objetivo bloqueada
    int r = AI::rounds();
    auto pos = board.marker;

    // std::cout << "rounds:"<< r<<"\n";

    auto reach = board.compute_reachability();

    int Dmax = reach.h1;
    int Dmin = reach.h5;

    int Par = 0;
    if (std::abs(Dmax) == 900 && std::abs(Dmin) == 900) {
        Par = (reach.reachable_count % 2 == 0)
             ? (is_max ? 200 : -200)
             : (is_max ? -200 : 200);

    }

    int AvCels = available_choices(board,is_max);

    int BlkDiag = h_diag_block_goal(board);

    //std::cout << "AvCels:"<< AvCels<<"\n";

    return Dmax+Dmin+Par+BlkDiag;

}


int AI:: available_choices(const Board& board, bool is_max){
    int choices = board.get_valid_moves().size();
    return is_max ? choices : -choices; 
}


//static int h_distance(const Board& board, std::pair<int, int> pos, bool is_max);

int AI::h_distance(const Board& board,std::pair<int, int> pos, bool is_max) {
    // - Distância de Chebyshev até ao objetivo (máximo das diferenças em r/c).
    auto goal = is_max ? std::make_pair(board.rows - 1, 0) : std::make_pair(0, board.cols - 1);
    return std::max(std::abs(goal.first - pos.first), std::abs(goal.second - pos.second));
}

// ----------------------------------------------------------------------------
// - Gera a lista de sucessores e ordena pela heuristica
// - Aplica um dos tipos de ordenação (Deterministic, ShuffleAll, NoisyJitter)
// para experimentação e criação de variedade em torneios.
// - Usa uma cache local (s_order_cache) indexada pelo estado + tipo para
// evitar recalcular na mesma raiz.
// * ShuffleAll: ignora scores, baralha completamente.
// * NoisyJitter: adiciona ruído gaussiano pequeno aos scores, ordena estável.
// - Garante comparador estável gerando ruído fixo por jogada.
// * Deterministic: ordena de forma determinística pelos scores (MAX desc, MIN asc).
// - Opcionalmente baralha apenas grupos de empates.
// ----------------------------------------------------------------------------

const std::vector<MoveScore>& AI::ordered_children(Board& board, bool is_max, int depth, int /*max_depth*/, int player_search) {
    // A chave de cache inclui a política, para diferentes IAs coexistirem sem conflito
    Key base = make_cache_key(board, is_max, depth, player_search);
    Key key  = base + "|" + policy_tag(ordering_policy);

    if (auto it = s_order_cache.find(key); it != s_order_cache.end()) return it->second;


    // Constrói a lista de sucessores avaliados uma única vez
    std::vector<MoveScore> out;
    auto moves = board.get_valid_moves();
    out.reserve(moves.size());


    for (const auto& mv : moves) {
            
        Board tmp = board;
        tmp.make_move(mv);
        int s = total_heuristic(tmp, is_max); // coloca no cache mais tarde em folhas por s_heuristic_cache
        // std::cout <<"("<< mv.first<<","<<mv.second<<") ->"<<s<<"\n";
        out.push_back({mv, s});
    }

    //Aplicar politica de ordenamento
    if (ordering_policy == OrderingPolicy::ShuffleAll) {
        // Ignora scores, baralha completamente
        #ifdef RASTROS_TESTS
          auto& gen = test_rng();
        #else
          std::random_device rd; std::mt19937 gen(rd());
        #endif
        std::shuffle(out.begin(), out.end(), gen);
    } else if (ordering_policy == OrderingPolicy::NoisyJitter) {
        // Heurística + ruído gaussiano pequeno, depois ordenação estável
        #ifdef RASTROS_TESTS
          auto& gen = test_rng();
        #else
          std::random_device rd; std::mt19937 gen(rd());
        #endif
        std::normal_distribution<double> noise(0.0, order_noise_sigma);

        // Pré-computa ruído por jogada (para comparador estável)
        auto move_key = [](const std::pair<int,int>& m)->uint64_t {
            return (static_cast<uint64_t>(static_cast<uint32_t>(m.first)) << 32)
                 ^ static_cast<uint32_t>(m.second);
        };
        std::unordered_map<uint64_t,double> nmap;
        nmap.reserve(out.size());
        for (auto& ms : out) nmap.emplace(move_key(ms.move), noise(gen));

        auto cmpMax = [&](const MoveScore& a, const MoveScore& b) {
            double ka = static_cast<double>(a.score) + nmap[move_key(a.move)];
            double kb = static_cast<double>(b.score) + nmap[move_key(b.move)];
            if (ka != kb) return ka > kb; // best first
            // stable tie-breaker by position
            if (a.move.first != b.move.first) return a.move.first < b.move.first;
            return a.move.second < b.move.second;
        };
        auto cmpMin = [&](const MoveScore& a, const MoveScore& b) {
            double ka = static_cast<double>(a.score) + nmap[move_key(a.move)];
            double kb = static_cast<double>(b.score) + nmap[move_key(b.move)];
            if (ka != kb) return ka < kb; // worst for MAX first
            if (a.move.first != b.move.first) return a.move.first < b.move.first;
            return a.move.second < b.move.second;
        };

        if (is_max) std::stable_sort(out.begin(), out.end(), cmpMax);
        else        std::stable_sort(out.begin(), out.end(), cmpMin);

        // Opcional: baralha apenas empates de score base - aplica-se a qualquer politica escolhida
        if (shuffle_ties_only) {
            #ifdef RASTROS_TESTS
              auto& gen2 = test_rng();
            #else
              std::random_device rd2; std::mt19937 gen2(rd2());
            #endif
            size_t i = 0;
            while (i < out.size()) {
                size_t j = i + 1;
                while (j < out.size() && out[j].score == out[i].score) ++j;
                if (j - i > 1) std::shuffle(out.begin() + i, out.begin() + j, gen2);
                i = j;
            }
        }

    } else { // Deterministic
        if (is_max) std::sort(out.begin(), out.end(), MoveScoreCmpMax{});
        else        std::sort(out.begin(), out.end(), MoveScoreCmpMin{});

        if (shuffle_ties_only) {
            #ifdef RASTROS_TESTS
              auto& gen = test_rng();
            #else
              std::random_device rd; std::mt19937 gen(rd());
            #endif
            size_t i = 0;
            while (i < out.size()) {
                size_t j = i + 1;
                while (j < out.size() && out[j].score == out[i].score) ++j;
                if (j - i > 1) std::shuffle(out.begin() + i, out.begin() + j, gen);
                i = j;
            }
        }
    }

    // coloca resultado na cache e devolve referência
    auto [ins, _] = s_order_cache.emplace(key, std::move(out));
    return ins->second;
}


//////////////////////////////////////////////////////////////////////////

//quiescence-search -> implementada mas não utilizada, precisa verificação/testes
// ----------------------------------------------------------------------------
// - (q-search) para reduzir efeito de horizonte.
// - Extende apenas em posições "ruidosas" (precisam de ser melhor definidas).
// - Mantém avaliação estática (stand-pat) e aplica cortes alfa–beta.
// - qdepth limita a profundidade de extensão; base_depth é a profundidade
// do nó de onde partiu a chamada (para ajustar scores terminais).
// - só explora um subconjunto de jogadas consideradas ruidosas
// ----------------------------------------------------------------------------

int AI::quiescence(Board board, bool is_max, int alpha, int beta, int qdepth, int base_depth) {
    
    //Teste terminal (pode acontecer no horizonte)
    if (board.is_terminal()) {
        int t = evaluate_terminal(board, is_max);
        return adjust_terminal_score(t, base_depth + qdepth);
    }

    // Stand‑pat: avaliação estática da posição corrente
    // (perspetiva = lado que joga neste nó)
    int stand_pat = total_heuristic(board, is_max);

    // Alfa–beta sobre o stand‑pat
    if (is_max) {
        if (stand_pat >= beta) return stand_pat;
        if (alpha < stand_pat) alpha = stand_pat;
    } else {
        if (stand_pat <= alpha) return stand_pat;
        if (beta > stand_pat) beta = stand_pat;
    }

    // Métrica base para decidir "ruído" (uma só vez)
    auto reach = board.compute_reachability();
    int base_h1 = reach.h1;
    int base_h5 = reach.h5;

    //posição é "sossegada" / limite de profundidade
    if (qdepth >= q_max_plies || is_quiet_position(board, base_h1, base_h5)) {
        return stand_pat;
    }

    // Geração de jogadas "ruidosas" (táticas)
    auto qmoves = gen_quiescence_moves(board, is_max, base_h1, base_h5);
    if (qmoves.empty()) return stand_pat; // nada de relevante: devolve stand‑pat

    //Exploração barata (best‑first leve é aplicado dentro do gerador)
    int best = is_max ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();

    for (const auto& mv : qmoves) {
        Board tmp = board;
        tmp.make_move(mv);

        int score = quiescence(tmp, !is_max, alpha, beta, qdepth + 1, base_depth);
        // Ajuste por profundidade total até aqui
        score = adjust_terminal_score(score, base_depth + qdepth + 1);

        if (is_max) {
            if (score > best) best = score;
            if (best >= beta) return best;     // cutoff
            if (alpha < best) alpha = best;
        } else {
            if (score < best) best = score;
            if (best <= alpha) return best;    // cutoff
            if (beta > best) beta = best;
        }
    }

    return best;
}

// ----------------------------------------------------------------------------
// - Determina se uma posição é "calma" (sem ameaças imediatas).
// - Considera ruidosa se algum objetivo está a ≤1 jogada de distância.
// - Caso ambos os caminhos estejam bloqueados longe (900 = inalcançável),
// assume-se como quieta.
// ----------------------------------------------------------------------------
bool AI::is_quiet_position(const Board& board, int base_h1, int base_h5) {
    // "Calma"se não houver vitórias iminentes nem sinais triviais de
    // bloqueio imediato -  ruidosa se qualquer objetivo estiver a 1.
    if (std::abs(base_h1) <= 1) return false;
    if (std::abs(base_h5) <= 1) return false;

    // Se ambos os caminhos estão bloqueados consider-se  como quieta.
    // (900 é o valor de inalcançável no cálculo de reachability.)
    return true;
}

// ----------------------------------------------------------------------------
// - Gera subconjunto de jogadas "ruidosas" para q-search.
// - Critérios para marcar jogada como ruidosa:
// * near_goal: aproximação imediata a objetivo (≤1 jogada).
// * big_swing: grande diferença na distância até objetivo vs base.
// * low_reply: baixa mobilidade de resposta do adversário (jogada forçante).
// - Opcionalmente ordena estas jogadas de forma barata para potenciar cortes.
// ----------------------------------------------------------------------------
std::vector<std::pair<int,int>> AI::gen_quiescence_moves(const Board& board, bool is_max,
                                                         int base_h1, int base_h5) {

    // Seleciona apenas jogadas com potencial tático imediato
    std::vector<std::pair<int,int>> noisy;
    auto moves = board.get_valid_moves();
    noisy.reserve(moves.size());

    for (const auto& mv : moves) {
        Board tmp = board;
        tmp.make_move(mv);

        // Reachability após a jogada
        auto r2 = tmp.compute_reachability();
        int h1p = r2.h1;
        int h5p = r2.h5;

        bool near_goal = (std::abs(h1p) <= 1) || (std::abs(h5p) <= 1);
        bool big_swing = (std::abs(h1p - base_h1) >= q_swing_delta) ||
                         (std::abs(h5p - base_h5) >= q_swing_delta);

        // Mobilidade de resposta do oponente
        int opp_moves = (int)tmp.get_valid_moves().size();
        bool low_reply = (opp_moves <= q_low_mob);

        if (near_goal || big_swing || low_reply) {
            noisy.push_back(mv);
        }
    }

    // opcional -> precisa de verificação e testes
    //ordenação barata (melhora os cortes sem custo excessivo):
    // prioridade aproximada para lances que melhoram a perspetiva do jogador
    // ao turno e/ou pioram a do adversário.
    std::stable_sort(noisy.begin(), noisy.end(), [&](const auto& a, const auto& b){
        Board ta = board; ta.make_move(a);
        Board tb = board; tb.make_move(b);
        auto ra = ta.compute_reachability();
        auto rb = tb.compute_reachability();
        int da = (is_max ? -ra.h1 : ra.h1) + (is_max ? rb.h5 : -ra.h5); 
        int db = (is_max ? -rb.h1 : rb.h1) + (is_max ? rb.h5 : -rb.h5);
        return da > db; // best-first leve
    });

    return noisy;
}












