#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <cstdlib>

#include "Player.hpp"
#include "Weapon.hpp"
#include "GameObject.hpp"
#include "Window.hpp"
#include "FileBuffer.hpp"
#include "Filesystem.hpp"
#include "GLContext.hpp"
#include "WindowMultiContext.hpp"

struct WindowConfig
{
    std::string title;
    int width;
    int height;
};

static std::vector<WindowConfig> parseArgs(int argc, char **argv)
{
    std::vector<WindowConfig> configs;

    if (argc == 1)
    {
        configs.push_back({"OpenGL - Window", 800, 600});
    }
    else if (argc == 2)
    {
        int count = std::atoi(argv[1]);
        if (count < 1) count = 1;
        for (int i = 0; i < count; i++)
            configs.push_back({"OpenGL - Window " + std::to_string(i + 1), 800, 600});
    }
    else if (argc == 3)
    {
        int w = std::atoi(argv[1]);
        int h = std::atoi(argv[2]);
        if (w < 100) w = 800;
        if (h < 100) h = 600;
        configs.push_back({"OpenGL - Window", w, h});
    }
    else
    {
        int count = std::atoi(argv[1]);
        if (count < 1) count = 1;
        for (int i = 0; i < count; i++)
        {
            int w = (2 + i * 2 + 0 < argc) ? std::atoi(argv[2 + i * 2 + 0]) : 800;
            int h = (2 + i * 2 + 1 < argc) ? std::atoi(argv[2 + i * 2 + 1]) : 600;
            if (w < 100) w = 800;
            if (h < 100) h = 600;
            configs.push_back({"OpenGL - Window " + std::to_string(i + 1), w, h});
        }
    }

    return configs;
}

static std::vector<int> createWindows(WindowMultiContext& ctx,
                                       const std::vector<WindowConfig>& configs)
{
    std::vector<int> indices;
    for (const auto& cfg : configs)
    {
        int idx = ctx.addWindow(cfg.title, cfg.width, cfg.height);
        indices.push_back(idx);
        if (idx >= 0)
            std::cout << "created: " << cfg.title
                      << " (" << cfg.width << "x" << cfg.height << ")\n";
    }
    return indices;
}

static void runLoop(WindowMultiContext& ctx, const std::vector<int>& indices)
{
    std::cout << indices.size() << " window(s) open."
              << (ctx.consoleCount() ? " + console" : "")
              << ". press Esc or close to exit.\n";

    while (ctx.pollEvents())
    {
        for (int idx : indices)
            ctx.render(idx);
        ctx.renderConsoles();
    }

    std::cout << "windows closed.\n";
}

int main(int argc, char **argv)
{
    auto configs = parseArgs(argc, argv);
    if (configs.empty())
    {
        std::cerr << "no window configurations.\n";
        return 1;
    }

    WindowMultiContext ctx;

#if defined(DEBUG)
    int con_idx = ctx.addConsole("Console - Debug", 600, 400, "", "JetBrainsMono NF", 11.0f);
#endif

    // Redirect standard output and standard error to the built-in consoles
    ConsoleRedirector cout_redir(ctx, std::cout);
    ConsoleRedirector cerr_redir(ctx, std::cerr);

    char hostname[256] = "unknown";
    gethostname(hostname, sizeof(hostname));
    const char* username = getlogin();
    if (!username) username = getenv("USER");
    // Get the exact bash prompt directly from the user's bash config
    std::string prompt;
    FILE* pipe = popen("bash -i -c 'echo -n \"${PS1@P}\"' 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            prompt += buffer;
        }
        pclose(pipe);
    }

    if (prompt.empty()) {
        prompt = "$ "; // Fallback if bash fails
    }

    std::cout << prompt;
    for (int i = 0; i < argc; i++) std::cout << argv[i] << (i < argc - 1 ? " " : "");
    std::cout << "\n";

#if defined(DEBUG)
    if (con_idx >= 0)
        std::cout << "created: debug console (600x400)\n";
#endif

    // No shaders needed, we use fixed-function pipeline for texture rendering
    auto indices = createWindows(ctx, configs);

    bool all_failed = true;
    for (int idx : indices)
        if (idx >= 0) { all_failed = false; break; }

    if (all_failed)
    {
        std::cerr << "failed to create any windows.\n";
        return 1;
    }

    runLoop(ctx, indices);
    return 0;
}
