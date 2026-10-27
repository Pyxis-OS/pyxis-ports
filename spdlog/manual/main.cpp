#include <spdlog/async.h>
#include <spdlog/details/os.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <string>

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: spdlog-manual LOG_DIRECTORY\n");
        return 2;
    }
    try {
        try {
            spdlog::pattern_formatter unsupported("%P");
            std::fprintf(stderr, "process-ID pattern was unexpectedly accepted\n");
            return 1;
        } catch (const spdlog::spdlog_ex &error) {
            std::printf("process-ID rejection: %s\n", error.what());
        }

        auto now = spdlog::details::os::now();
        auto calendar = spdlog::details::os::localtime(spdlog::log_clock::to_time_t(now));
        std::printf("calendar: %04d-%02d-%02d\n", calendar.tm_year + 1900,
                    calendar.tm_mon + 1, calendar.tm_mday);
        std::printf("native root parent: %s\n",
                    spdlog::details::os::dir_name("tmp://example.log").c_str());

        auto plain = spdlog::stdout_logger_mt("plain");
        auto color = spdlog::stdout_color_mt("color");
        plain->info("bundled fmt {}: {:.2f}", FMT_VERSION, 1.25);
        color->info("automatic console color");

        std::string directory(argv[1]);
        auto basic = spdlog::basic_logger_mt("basic", directory + "/basic.log", true);
        basic->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        basic->info("public native FILE log {}", 1);
        basic->flush();

        auto rotating = spdlog::rotating_logger_mt("rotating", directory + "/rotate.log", 64, 2);
        for (int line = 0; line != 6; ++line) {
            rotating->info("public rotation line {}", line);
        }
        rotating->flush();

        spdlog::init_thread_pool(8192, 1);
        auto async = spdlog::basic_logger_mt<spdlog::async_factory>(
            "async", directory + "/async.log", true);
        async->info("public async line {}", 1);
        async->flush();
        async.reset();
        spdlog::shutdown();
        std::puts("logging calls and async shutdown returned; inspect the public log files");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "spdlog-manual: %s\n", error.what());
        spdlog::shutdown();
        return 1;
    }
}
