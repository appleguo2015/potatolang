#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <array>
#include <memory>
#include <unordered_set>
#include <cstdio>
#include <SDL2/SDL.h>

// Simple 5x7 ASCII font (standard ASCII 32-127)
// Each byte represents a column, 5 bytes per char.
static const unsigned char kFont5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // (space)
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x14, 0x08, 0x3E, 0x08, 0x14, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x08, 0x14, 0x22, 0x41, 0x00, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x00, 0x41, 0x22, 0x14, 0x08, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x09, 0x01, // F
    0x3E, 0x41, 0x49, 0x49, 0x7A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x3F, 0x40, 0x38, 0x40, 0x3F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x7F, 0x41, 0x41, 0x00, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // \ (backslash)
    0x00, 0x41, 0x41, 0x7F, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x0C, 0x52, 0x52, 0x52, 0x3E, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x7F, 0x10, 0x28, 0x44, 0x00, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x10, 0x08, 0x08, 0x10, 0x08, // ~
    0x7F, 0x7F, 0x7F, 0x7F, 0x7F, // (DEL/Block)
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: tomato <filename>\n";
        return 1;
    }
    std::string filename = argv[1];

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << "\n";
      return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Tomato Native Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) return 1;

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return 1;

    std::vector<std::string> lines;
    {
        std::ifstream in(filename);
        if (in) {
            std::string line;
            while (std::getline(in, line)) lines.push_back(line);
        }
    }
    if (lines.empty()) lines.push_back("");

    int cursorX = 0;
    int cursorY = 0;
    int scrollY = 0;
    bool quit = false;
    SDL_Event e;
    
    bool showOutput = false;
    std::string outputContent;

    // Keywords for highlighting
    std::unordered_set<std::string> keywords = {
       "let", "print", "if", "else", "while", "fun", "return", "import",
       "true", "false", "nil", "and", "or"
    };

    auto drawText = [&](int x, int y, const std::string& text, int r, int g, int b) {
       SDL_SetRenderDrawColor(renderer, r, g, b, 255);
       int cx = x;
       for (char c : text) {
           unsigned char uc = (unsigned char)c;
           if (uc < 32 || uc > 127) uc = 127;
           int index = (uc - 32) * 5;
           for (int col = 0; col < 5; ++col) {
               unsigned char col_data = kFont5x7[index + col];
               for (int row = 0; row < 7; ++row) {
                   if ((col_data >> row) & 1) {
                       SDL_RenderDrawPoint(renderer, cx + col, y + row);
                   }
               }
           }
           cx += 6;
       }
       return cx;
    };

    SDL_StartTextInput();

    while (!quit) {
      if (SDL_WaitEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
          quit = true;
        } else if (e.type == SDL_TEXTINPUT) {
            std::string text = e.text.text;
            if (cursorY < static_cast<int>(lines.size())) {
                if (cursorX > static_cast<int>(lines[cursorY].size())) cursorX = lines[cursorY].size();
                lines[cursorY].insert(cursorX, text);
                cursorX += text.length();
            }
        } else if (e.type == SDL_KEYDOWN) {
          auto sym = e.key.keysym.sym;
          
          if (sym == SDLK_BACKSPACE) {
              if (cursorX > 0) {
                  lines[cursorY].erase(cursorX - 1, 1);
                  cursorX--;
              } else if (cursorY > 0) {
                  int prevLen = lines[cursorY - 1].length();
                  lines[cursorY - 1] += lines[cursorY];
                  lines.erase(lines.begin() + cursorY);
                  cursorY--;
                  cursorX = prevLen;
              }
          } else if (sym == SDLK_RETURN) {
              std::string current = lines[cursorY];
              lines[cursorY] = current.substr(0, cursorX);
              lines.insert(lines.begin() + cursorY + 1, current.substr(cursorX));
              cursorY++;
              cursorX = 0;
          } else if (sym == SDLK_LEFT) {
              if (cursorX > 0) cursorX--;
              else if (cursorY > 0) {
                  cursorY--;
                  cursorX = lines[cursorY].length();
              }
          } else if (sym == SDLK_RIGHT) {
              if (cursorX < static_cast<int>(lines[cursorY].length())) cursorX++;
              else if (cursorY < static_cast<int>(lines.size()) - 1) {
                  cursorY++;
                  cursorX = 0;
              }
          } else if (sym == SDLK_UP) {
              if (cursorY > 0) cursorY--;
          } else if (sym == SDLK_DOWN) {
              if (cursorY < static_cast<int>(lines.size()) - 1) cursorY++;
          } else if (sym == SDLK_F2) { // Save
              std::ofstream out(filename);
              for (const auto& l : lines) out << l << "\n";
          } else if (sym == SDLK_F5) { // Run
              // Save first
              {
                  std::ofstream out(filename);
                  for (const auto& l : lines) out << l << "\n";
              }
              // Run
              // Assume potatolang is in the parent directory or PATH, or current dir
              // Since we'll build tomato in root, ./potatolang is correct
              std::string cmd = "./potatolang --run " + filename;
              std::string result;
              std::array<char, 128> buffer;
              std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
              if (pipe) {
                  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                      result += buffer.data();
                  }
              }
              outputContent = result;
              showOutput = true;
          } else if (sym == SDLK_ESCAPE) {
              if (showOutput) showOutput = false;
              else quit = true;
          }
        }
      }
      
      if (cursorY < 0) cursorY = 0;
      if (cursorY >= static_cast<int>(lines.size())) cursorY = lines.size() - 1;
      if (cursorX < 0) cursorX = 0;
      if (cursorX > static_cast<int>(lines[cursorY].length())) cursorX = lines[cursorY].length();

      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_RenderClear(renderer);

      int w, h;
      SDL_GetWindowSize(window, &w, &h);
      int lineH = 12;
      int visibleLines = h / lineH;
      
      if (showOutput) visibleLines = (h * 2 / 3) / lineH;

      if (cursorY < scrollY) scrollY = cursorY;
      if (cursorY >= scrollY + visibleLines) scrollY = cursorY - visibleLines + 1;
      
      for (int i = 0; i < visibleLines; ++i) {
          int idx = scrollY + i;
          if (idx >= static_cast<int>(lines.size())) break;
          
          int y = i * lineH;
          
          std::string num = std::to_string(idx + 1);
          drawText(5, y, num, 150, 150, 150);
          
          int textX = 40;
          std::string& line = lines[idx];

          // Syntax Highlighting
          size_t j = 0;
          while (j < line.size()) {
              if (j + 1 < line.size() && line[j] == '/' && line[j+1] == '/') {
                  // Comment
                  textX = drawText(textX, y, line.substr(j), 128, 128, 128);
                  j = line.size();
              } else if (line[j] == '"') {
                  // String
                  size_t end = j + 1;
                  while (end < line.size() && line[end] != '"') {
                      if (line[end] == '\\' && end + 1 < line.size()) end++;
                      end++;
                  }
                  if (end < line.size()) end++;
                  textX = drawText(textX, y, line.substr(j, end - j), 0, 128, 0);
                  j = end;
              } else if (std::isdigit(static_cast<unsigned char>(line[j]))) {
                  // Number
                  size_t end = j;
                  while (end < line.size() && (std::isdigit(static_cast<unsigned char>(line[end])) || line[end] == '.')) end++;
                  textX = drawText(textX, y, line.substr(j, end - j), 128, 0, 128);
                  j = end;
              } else if (std::isalpha(static_cast<unsigned char>(line[j])) || line[j] == '_') {
                  // Identifier or Keyword
                  size_t end = j;
                  while (end < line.size() && (std::isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_')) end++;
                  std::string word = line.substr(j, end - j);
                  if (keywords.count(word)) {
                      textX = drawText(textX, y, word, 0, 0, 255);
                  } else {
                      textX = drawText(textX, y, word, 0, 0, 0);
                  }
                  j = end;
              } else {
                  // Symbol/Space
                  textX = drawText(textX, y, std::string(1, line[j]), 0, 0, 0);
                  j++;
              }
          }
          
          if (idx == cursorY) {
              SDL_Rect r = {40 + cursorX * 6, y, 2, 8};
              SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
              SDL_RenderFillRect(renderer, &r);
          }
      }

      if (showOutput) {
          int panelY = visibleLines * lineH;
          SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
          SDL_Rect r = {0, panelY, w, h - panelY};
          SDL_RenderFillRect(renderer, &r);
          
          int outY = panelY + 5;
          std::stringstream ss(outputContent);
          std::string s;
          while(std::getline(ss, s)) {
              if (outY > h - 10) break;
              drawText(5, outY, s, 50, 50, 50);
              outY += 10;
          }
      }

      SDL_RenderPresent(renderer);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}