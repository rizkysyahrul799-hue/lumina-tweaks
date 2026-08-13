#include <iostream>
#include <string>
#include <cstdio>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>

class AppWatcher {
public:
    using Callback = std::function<void(const std::string&)>;

    AppWatcher(Callback cb) : callback_(cb), running_(false) {}

    ~AppWatcher() {
        stop();
    }

    void start() {
        running_ = true;
        worker_ = std::thread(&AppWatcher::loop, this);
    }

    void stop() {
        running_ = false;
        if (worker_.joinable()) {
            worker_.join();
        }
    }

private:
    Callback callback_;
    std::atomic<bool> running_;
    std::thread worker_;

    void loop() {
        while (running_) {
            // Menggunakan 'stdbuf -oL' untuk memaksa line-buffering pada pipe
            FILE* fp = popen("stdbuf -oL am monitor 2>/dev/null", "r");
            if (!fp) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }

            char buffer[256];
            while (running_ && fgets(buffer, sizeof(buffer), fp) != nullptr) {
                std::string line(buffer);
                size_t pos = line.find("Activity starting: ");
                if (pos != std::string::npos) {
                    std::string pkg = line.substr(pos + 19);
                    size_t end = pkg.find_first_of(" \r\n");
                    if (end != std::string::npos) {
                        pkg = pkg.substr(0, end);
                    }
                    if (!pkg.empty() && callback_) {
                        callback_(pkg);
                    }
                }
            }
            pclose(fp);
            if (running_) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
};
