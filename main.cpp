#include <iostream>
#include <string>
#include <vector>

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
                                       const std::vector<WindowConfig>& configs,
                                       const std::string& vert_src,
                                       const std::string& frag_src)
{
    std::vector<int> indices;
    for (const auto& cfg : configs)
    {
        int idx = ctx.addWindow(cfg.title, cfg.width, cfg.height, vert_src, frag_src);
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
    // old PlayerCharacter loop
    // for (int i = 1; i + 2 < argc; i += 3)
    // {
    //     PlayerCharacter player(argv[i], std::atof(argv[i + 1]), std::atoi(argv[i + 2]));
    //     player.ds_info();
    //     std::cout << "---\n";
    // }

#if defined(DEBUG)
    std::cout << "=== scanning shaders ===\n";
#endif

    Filesystem fs(".glsl");
#if defined(DEBUG)
    fs.ds_info();
#endif

    if (fs.count() < 2)
    {
        std::cout << "need at least 2 shader files (vertex + fragment).\n";
        return 0;
    }

    std::string vert_src, frag_src;
    for (std::size_t i = 0; i < fs.count(); i++)
    {
        auto buf = fs.load(i);
        auto path = buf.path();
        if (path.find("vert") != std::string::npos)
            vert_src = buf.toString();
        else if (path.find("frag") != std::string::npos)
            frag_src = buf.toString();
#if defined(DEBUG)
        buf.ds_info();
        std::cout << "--- content ---\n" << buf.toString() << "\n";
#endif
    }

    if (vert_src.empty() || frag_src.empty())
    {
        std::cerr << "could not find vertex or fragment shader.\n";
        return 1;
    }

    auto configs = parseArgs(argc, argv);
    if (configs.empty())
    {
        std::cerr << "no window configurations.\n";
        return 1;
    }

    WindowMultiContext ctx;
    auto indices = createWindows(ctx, configs, vert_src, frag_src);

#if defined(DEBUG)
    int con_idx = ctx.addConsole("Console - Debug", 600, 400);
    if (con_idx >= 0)
        std::cout << "created: debug console (600x400)\n";
#endif

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
