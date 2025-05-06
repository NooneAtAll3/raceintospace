#ifndef BUTTON_INTERACTION_H
#define BUTTON_INTERACTION_H

#include <vector>

struct Interaction
{
	int x[2]{}; // left edge, right edge
	int y[2]{}; // top edge, bottom edge
	int key = -1;
};

int CheckInteraction(const std::vector<Interaction>& button_list);
#endif // BUTTON_INTERACTION_H
