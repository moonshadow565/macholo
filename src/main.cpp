#include <cstdlib>
#include "loader.hpp"

int main(int argc, char** argv, char** env) {
  loader::Image::main = loader::Image::load(argv[1]);
  loader::Image::main->bind();
  loader::Image::main->init();
  return loader::Image::main->run(argc - 1, argv + 1, env);
}
