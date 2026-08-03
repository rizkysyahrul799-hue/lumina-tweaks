#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <string>
std::string exec_cmd(const std::string& cmd);
int get_sf_index_by_fps(float target_fps);
bool set_refresh_rate(float target_fps);
#endif // DISPLAY_HPP