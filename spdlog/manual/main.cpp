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

static int clock_errors()
{
    spdlog::logger logger("clock-errors");
    int handled = 0;
    bool raw_location = false;
    logger.set_error_handler([&](const std::string &message) {
        ++handled;
        if (handled == 1) {
            raw_location = message.find("clock-routing(1)") != std::string::npos;
        }
        std::printf("handler %d: %s\n", handled, message.c_str());
    });
    logger.log(spdlog::source_loc{"clock-routing", 1, "clock_errors"},
               spdlog::level::info, spdlog::string_view_t{"raw message"});
    logger.info("formatted message {}", 1);
    bool direct_threw = false;
    try {
        (void)spdlog::details::os::now();
    } catch (const spdlog::spdlog_ex &error) {
        direct_threw = true;
        std::printf("direct clock failure: %s\n", error.what());
    }
    // This timestamp is caller-authored; this overload needs no current clock.
    logger.log(spdlog::log_clock::time_point{std::chrono::seconds{946684800}},
               spdlog::source_loc{}, spdlog::level::info,
               spdlog::string_view_t{"explicit timestamp"});
    std::printf("missing-clock routing: handled=%d raw-location=%d direct-threw=%d\n",
                handled, raw_location, direct_threw);
    return handled == 2 && raw_location && direct_threw ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: spdlog-manual LOG_DIRECTORY|--clock-errors\n");
        return 2;
    }
    try {
        if (std::string(argv[1]) == "--clock-errors") {
            return clock_errors();
        }
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
