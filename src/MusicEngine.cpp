#include "gui/GUI.h"

using namespace std;

int main() {
	try {
		GUI("Music Engine", 1400, 1060).start();
	} catch (const exception &e) {
		cerr << e.what() << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
