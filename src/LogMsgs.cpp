#include "LogMsgs.hpp"
#include <iostream>
#include <iomanip>

namespace {
    std::ostream* g_log_stream = &std::cout;
}

namespace LogMsgs {
void set_stream(std::ostream& os) { g_log_stream = &os; }
std::ostream& out() { return *g_log_stream; }

namespace AI {
void log_algo_tag(const std::string& tag) {
    out() << tag << "\n\n";
}

void log_first_move(const std::pair<int,int>& mv) {
    out() << "[MAX]First AI controled move : (" << mv.first << ", " << mv.second << ") ";
}

void log_root_moves(const std::pair<int,int>& pos,
                    const std::vector<std::pair<int,int>>& moves,
                    int depth_override) {
    out() << "(" << pos.first << "," << pos.second << ")->";
    for (const auto& m : moves) {
        out() << "(" << m.first << ", " << m.second << "), ";
    }
    out() << "\n";
    out() << "depth: " << depth_override << "\n";
}

void log_immediate_win(bool is_max, const std::pair<int,int>& mv, int val) {
    out() << "****[" << (is_max ? "MAX" : "MIN")
          << "] Immediate win found at root: (" << mv.first
          << "," << mv.second << ") " << val <<" ";
}

void log_root_trying_move(const std::string& player,
                          const std::pair<int,int>& mv,
                          bool is_last) {
    out() << (is_last ? "└── " : "├── ")
          << player << " trying move to: ("
          << mv.first << "," << mv.second << ")\n";
}

void log_root_score(const std::pair<int,int>& mv, int score, bool is_last) {
    out() << (is_last ? "└── " : "├── ")
          << "score for: (" << mv.first << "," << mv.second << ")-> "
          << score << "\n";
}

void log_best_move(const std::string& player,
                   const std::pair<int,int>& mv,
                   int score,
                   int depth_limit,
                   int actual_depth) {
    out() << "****[" << player << "] Best move selected: (" << mv.first
          << "," << mv.second << ") " << score << " [depth: " << depth_limit;
    if (actual_depth < depth_limit) {
        out() << " -> " << actual_depth;
    }
    out() << "] ";
}
} // namespace AI

} // namespace LogMsgs
