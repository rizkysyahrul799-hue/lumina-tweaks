#pragma once

#include <string>

class ProfileManager {
public:
    enum class Profile {
        NORMAL,
        PERFORMANCE
    };

    ProfileManager();
    void apply_profile(Profile profile);

private:
    Profile current_profile_;
    void write_node(const std::string& path, const std::string& value);
    void apply_performance();
    void apply_normal();
};
