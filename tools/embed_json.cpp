// 构建期把 src/json/*.json 原样嵌进这个头文件（raw string），
// 由 src/app/Data/GameData.h 在编译期用 constexpr JSON 解析器读成数值表；
// 游戏运行时不会读取或解析 JSON。
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char **argv)
{
    if (argc < 4 || (argc - 2) % 2 != 0) {
        std::fprintf(stderr, "usage: embed_json <out.h> <name> <file.json> ...\n");
        return 1;
    }

    std::ofstream out(argv[1], std::ios::binary);
    if (!out) {
        std::fprintf(stderr, "cannot write %s\n", argv[1]);
        return 1;
    }

    out << "#pragma once\n\nnamespace game_data {\n\n";
    for (int i = 2; i + 1 < argc; i += 2) {
        const char *name = argv[i];
        std::ifstream in(argv[i + 1], std::ios::binary);
        if (!in) {
            std::fprintf(stderr, "cannot read %s\n", argv[i + 1]);
            return 1;
        }
        std::ostringstream buffer;
        buffer << in.rdbuf();
        out << "inline constexpr char " << name << "[] = R\"NEONJSON(\n"
            << buffer.str() << "\n)NEONJSON\";\n\n";
    }
    out << "} // namespace game_data\n";
    return 0;
}
