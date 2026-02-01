// aPpLegUo
#include "potatolang.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Helper to replace all occurrences of a substring
static void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty()) return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

int main(int argc, char** argv) {
  try {

    // Compilation mode: ./potatolang <script> --out <binary>
    if (argc >= 4 && std::string(argv[2]) == "--out") {
      std::string sourcePath = argv[1];
      std::string outputPath = argv[3];
      std::string script = potatolang::ReadFile(sourcePath);
      
      // Generate a safe temporary filename
      std::string safeName = outputPath;
      for (char& c : safeName) {
          if (c == '/' || c == '\\' || c == '.') c = '_';
      }
      std::string tempFile = "temp_build_" + safeName + ".cpp";
      
      // Write to a temporary file
      {
          std::ofstream out(tempFile);
          out << "// aPpLegUo\n";
          out << "#include \"potatolang.h\"\n"; 
          out << "const char* kEmbeddedScript = R\"POTATO_EMBED(\n" << script << "\n)POTATO_EMBED\";\n";
          out << "int main(int argc, char** argv) {\n";
          out << "  try {\n";
          out << "    std::string input;\n";
          out << "    if (argc >= 2) {\n";
          out << "      if (std::string(argv[1]) == \"-\") {\n";
          out << "        input = potatolang::ReadAll(std::cin);\n";
          out << "      } else {\n";
          out << "        input = potatolang::ReadFile(argv[1]);\n";
          out << "      }\n";
          out << "    }\n";
          out << "    return potatolang::RunScript(kEmbeddedScript, input, std::cout, std::cerr);\n";
          out << "  } catch (const std::exception& e) {\n";
          out << "    std::cerr << e.what() << \"\\n\";\n";
          out << "    return 1;\n";
          out << "  }\n";
          out << "}\n";
      }
      
      // Compile the temporary file
      // Include current directory for potatolang.h
      std::string cmd = "clang++ -std=c++17 -o " + outputPath + " " + tempFile + " -I. $(pkg-config --cflags --libs sdl2)";
      int ret = std::system(cmd.c_str());
      
      // Clean up
      std::system(("rm " + tempFile).c_str());
      
      return ret;
    }

    if (argc >= 2 && std::string(argv[1]) == "--run") {
      if (argc < 3) throw std::runtime_error("Usage: potatolang --run <script.pt> [input.pt]");
      std::string script = potatolang::ReadFile(argv[2]);
      std::string input;
      if (argc >= 4) {
        if (std::string(argv[3]) == "-") {
          input = potatolang::ReadAll(std::cin);
        } else {
          input = potatolang::ReadFile(argv[3]);
        }
      } else {
        input = "";
      }
      return potatolang::RunScript(script, input, std::cout, std::cerr);
    }
    if (argc >= 2) return potatolang::ParseOnly(potatolang::ReadFile(argv[1]), std::cout, std::cerr);
    return potatolang::ParseOnly(potatolang::ReadAll(std::cin), std::cout, std::cerr);
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
