#ifndef AI_HPP
#define AI_HPP

#include "Board.hpp"
#include "HeuristicsUtils.hpp"
#include "LogMsgs.hpp"
#include <utility>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <functional>
#include <map>
#include <algorithm> 
#include <cstdint>
#include <chrono>



struct CompactStateKey {
    uint64_t board_hash = 0;
    int marker_r = 0;
    int marker_c = 0;
    bool is_max = false;
    int player_search = 0;
    std::string id() const {
        std::string s;
        s.reserve(48);
        s += (is_max ? 'M' : 'm');
        s += "@P";
        s += std::to_string(player_search);
        s += '@';
        s += std::to_string(marker_r);
        s += ',';
        s += std::to_string(marker_c);
        s += "|#";
        s += std::to_string(board_hash);
        return s;
    }
    bool operator==(const CompactStateKey& o) const {
        return board_hash == o.board_hash &&
               marker_r == o.marker_r &&
               marker_c == o.marker_c &&
               is_max == o.is_max &&
               player_search == o.player_search;
    }
};

struct CompactOrderKey {
    uint64_t board_hash = 0;
    int depth = 0;
    bool is_max = false;
    int player_search = 0;
    uint8_t policy = 0;
    int marker_r = 0;
    int marker_c = 0;
    bool operator==(const CompactOrderKey& o) const {
        return board_hash == o.board_hash &&
               depth == o.depth &&
               is_max == o.is_max &&
               player_search == o.player_search &&
               policy == o.policy &&
               marker_r == o.marker_r &&
               marker_c == o.marker_c;
    }
};

struct CompactHeuristicKey {
    uint64_t board_hash = 0;
    bool is_max = false;
    int depth = 0;
    int player_search = 0;
    int marker_r = 0;
    int marker_c = 0;
    bool operator==(const CompactHeuristicKey& o) const {
        return board_hash == o.board_hash &&
               is_max == o.is_max &&
               depth == o.depth &&
               player_search == o.player_search &&
               marker_r == o.marker_r &&
               marker_c == o.marker_c;
    }
};

namespace std {
    template<> struct hash<CompactStateKey> {
        size_t operator()(const CompactStateKey& k) const {
            size_t h = std::hash<uint64_t>{}(k.board_hash);
            h ^= std::hash<int>{}(k.marker_r) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(k.marker_c) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<bool>{}(k.is_max) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(k.player_search) + 0x9e3779b9 + (h<<6) + (h>>2);
            return h;
        }
    };

    template<> struct hash<CompactOrderKey> {
        size_t operator()(const CompactOrderKey& k) const {
            size_t h = std::hash<uint64_t>{}(k.board_hash);
            h ^= std::hash<int>{}(k.depth) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<bool>{}(k.is_max) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(k.player_search) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<uint8_t>{}(k.policy) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(k.marker_r) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(k.marker_c) + 0x9e3779b9 + (h<<6) + (h>>2);
            return h;
        }
    };

    template<> struct hash<CompactHeuristicKey> {
        size_t operator()(const CompactHeuristicKey& k) const {
            size_t h = std::hash<uint64_t>{}(k.board_hash);
            h ^= std::hash<bool>{}(k.is_max) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(k.depth) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(k.player_search) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(k.marker_r) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(k.marker_c) + 0x9e3779b9 + (h<<6) + (h>>2);
            return h;
        }
    };
}


enum class TTBound : uint8_t { Exact, Lower, Upper };

struct TTEntry {
    int value;      // stored score
    int depth;      // search depth this entry is valid for (plies remaining)
    TTBound bound;  // Exact / Lower(α) / Upper(β)
    // (optional) std::pair<int,int> bestMove;
};


// Diferentes politicas de ordenamento para testes/torneios
enum class OrderingPolicy {
    Deterministic,   // ordem pela heuristica
    ShuffleAll,      // baralha sucessores
    NoisyJitter      // heuristica + pequeno ruído
};


struct MoveScore {
    std::pair<int,int> move;
    int score;
};

struct MoveScoreCmpMax {
    bool operator()(const MoveScore& a, const MoveScore& b) const {
        if (a.score != b.score) return a.score > b.score;  // best first
        // stable tiebreaker: row, then col
        if (a.move.first != b.move.first) return a.move.first < b.move.first;
        return a.move.second < b.move.second;
    }
};
struct MoveScoreCmpMin {
    bool operator()(const MoveScore& a, const MoveScore& b) const {
        if (a.score != b.score) return a.score < b.score;  // worst for MAX first
        if (a.move.first != b.move.first) return a.move.first < b.move.first;
        return a.move.second < b.move.second;
    }
};


// Estatísticas de ordenação de sucessores e cortes
struct OrderingStats {
    uint64_t nodes = 0;              
    uint64_t cutoffs = 0;            
    uint64_t cutoff_first_child = 0; 
    uint64_t cutoff_idx_sum = 0;     
    uint64_t no_cutoff_nodes = 0;    
    uint64_t best_idx_sum = 0;       
};



class AI {
public:
    AI(bool is_max, int max_depth);
    AI(bool is_max, int max_depth, std::function<int(const Board&, bool)> heuristic_func,int debug_level); // usado por test_runner

    // Interface pública de configuração/consulta
    std::pair<int, int> choose_move(Board& board, int depth_override = -1, int rounds=0);
    void set_ordering_policy(OrderingPolicy p);           // escolhe política de ordenação
    void set_order_noise(double sigma);                   // ruído gaussiano para NoisyJitter
    void set_shuffle_ties_only(bool enabled);             // baralhar apenas empates
    void set_quiescence(bool enabled, int max_plies=4, int swing_delta=2, int low_mob=2) {
        use_quiescence = enabled; q_max_plies = max_plies;
        q_swing_delta = swing_delta; q_low_mob = low_mob;
    }
    // Estatísticas (expostas para ferramentas de teste/benchmark)
    void print_ordering_stats() const;
    void reset_ordering_stats();
    void clear_tt();                                     // limpa tTT
    void clear_order_caches();                           // limpa caches de ordenação
    void clear_s_heuristic_caches();                     // limpa caches de heurística
    void set_debug_level(int lvl) { debug_level = lvl; } // define verbosidade

    // Getters simples
    bool is_max_player() const { return is_max; }
    int max_depth_limit() const { return max_depth; }
    int get_debug_level() const { return debug_level; }
    int get_eval_successors() const { return eval_successors; }
    int get_generated_successors() const { return gen_successors; }
    int get_prunes() const { return prunes; }

    // Factories e utilitários estáticos
    static std::unique_ptr<AI> create_with_level(bool is_max, int depth, int level, int debug = 0);
    static void register_heuristics();
    static int rounds();

    // contadores globais de TT usados em testes/diagnóstico
    static uint64_t vs_lookups;   
    static uint64_t vs_hits;      
    static uint64_t vs_inserts;  

    // Wrappers para primitivas de heurística centralizadas em HeuristicsUtils
    // Tornados públicos para permitir uso em testes (ex.: test_ai.cpp)
    static int h_diag_block_goal(const Board& board) { return ::h_diag_block_goal(board); }
    static int available_choices(const Board& board, bool is_max) { return ::available_choices(board, is_max); }
    static int h_distance(const Board& board, std::pair<int, int> pos, bool is_max) { return ::h_distance(board, pos, is_max); }

private:
    static int count_visited;

    static std::map<int, std::function<int(const Board&, bool)>> heuristic_levels;
    bool is_max;
    int max_depth;
    int unplayable_cells_count = 1;

    int eval_successors = 0;
    int gen_successors = 0;
    int prunes = 0;
    int debug_level = 0;

    //minimax + 2 versões para testes comparativos (sem alafa-béta e sem TT)
    int minimax(Board& board, bool is_max, int depth, int alpha, int beta, int max_depth, int player_search);
    int minimax_noTT(Board board, bool is_max, int depth, int alpha, int beta, int max_depth, int player_search);
    int minimax_no_pruning(Board board, bool is_max, int depth, int max_depth, int player_search);

    int total_heuristic(const Board& board, bool is_max);


    int evaluate_terminal(const Board& board, bool is_max);
    int adjust_terminal_score(int score, int depth);

    
    std::function<int(const Board&, bool)> heuristic;
    int default_heuristic(const Board& board, bool is_max);

    static int s_rounds;

  
    // ordenação e cache de sucessores
    const std::vector<MoveScore>& ordered_children(Board& board, bool is_max, int depth,
                                         int max_depth, int player_search);


    // --- Quiescence search
    bool use_quiescence = true;
    int  q_max_plies    = 4;   // profundidade limite (tune 2..6)
    int  q_swing_delta  = 2;   // variação no caminho
    int  q_low_mob      = 2;   // limite the mobilidade do adversário

    int quiescence(Board board, bool is_max, int alpha, int beta, int qdepth, int base_depth);
    std::vector<std::pair<int,int>> gen_quiescence_moves(const Board& board, bool is_max,
                                                         int base_h1, int base_h5);
    bool is_quiet_position(const Board& board, int base_h1, int base_h5);


    OrderingPolicy ordering_policy = OrderingPolicy::Deterministic;
    double         order_noise_sigma = 0.75; // valor por defeito
    bool           shuffle_ties_only = false;


    CompactStateKey compact_state_key(const Board& board, bool is_max, int player_search) const;

     std::unordered_map<CompactStateKey, TTEntry> tt;

    // estatisticas para MAx e MIN
    OrderingStats ord_max_;
    OrderingStats ord_min_;

    // escolher onde guardar sendo MAX ou MIN
    OrderingStats& stats_for(bool is_max_node) {
        return is_max_node ? ord_max_ : ord_min_;
    }
    const OrderingStats& stats_for(bool is_max_node) const {
        return is_max_node ? ord_max_ : ord_min_;
    }

    int last_max_depth_reached = 0;

};

#endif // AI_HPP
