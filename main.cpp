#include <iostream>
#include "filesystem.h"

int main() {
    File* root = new File("root", "/", DIR);
    chdir("x", root);
}
