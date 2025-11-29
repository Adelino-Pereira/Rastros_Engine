#ifndef LOGMSGS_HPP
#define LOGMSGS_HPP

#include <ostream>
#include <string>
#include <vector>
#include <utility>

namespace LogMsgs {
// Stream configurável (por defeito std::cout)
void set_stream(std::ostream& os);
std::ostream& out();

namespace AI {
// Helpers específicos da AI (formato mantido)
void log_algo_tag(const std::string& tag); // "alg:minimax_opt", etc.
void log_first_move(const std::pair<int,int>& mv);
void log_root_moves(const std::pair<int,int>& pos,
                    const std::vector<std::pair<int,int>>& moves,
                    int depth_override);
void log_immediate_win(bool is_max, const std::pair<int,int>& mv, int val);
void log_root_trying_move(const std::string& player,
                          const std::pair<int,int>& mv,
                          bool is_last);
void log_root_score(const std::pair<int,int>& mv, int score, bool is_last);
void log_best_move(const std::string& player,
                   const std::pair<int,int>& mv,
                   int score,
                   int depth_limit,
                   int actual_depth);
} // namespace AI

} // namespace LogMsgs

#endif
