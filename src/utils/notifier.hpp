#ifndef LUMINA_NOTIFIER_HPP
#define LUMINA_NOTIFIER_HPP

#include <string>
#include "sysfs.hpp"

inline void send_notification(const std::string& title, const std::string& message) {
    std::string cmd = "su 2000 -c \"cmd notification post -S bigtext -t '"
                      + title + "' 'lumina_notif' '" + message + "'\" 2>/dev/null";
    exec_cmd(cmd);
}

#endif
