#include <iostream>

#include "ska/helloworld/config.h"
#include "ska/helloworld/hello.h"
#include "ska/helloworld/wave.h"

int main() {

	ska::helloworld::wave theHello;

	std::cout << "Hello world!" << std::endl;

	theHello.greeting();

	return 0;
}
