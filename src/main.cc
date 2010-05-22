#include <cstdio>
#include <clocale>
#include <vector>
#include <iostream>
#include <unicode/ustream.h>
#include <unistd.h>
#include "ast.h"
#include "parser.h"

using namespace iv::core;

int main(void) {

  std::setlocale(LC_ALL, "");

  std::vector<char> v;

  if (std::FILE *fp = std::fopen("test.js", "r")) {
    char buf[1024];
    while (std::size_t len = std::fread(buf, 1, sizeof(buf), fp))
      v.insert(v.end(), buf, buf + len);
    v.push_back('\0');
    std::fclose(fp);
  }

  iv::core::Parser p(v.data());
  FunctionLiteral* global = p.ParseProgram();
  if (global) {
    UnicodeString ustr;
    global->Serialize(&ustr);
    std::cout << ustr << std::endl;
  }
  return 0;
}

