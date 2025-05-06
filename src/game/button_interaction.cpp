#include "button_interaction.h"

#include <vector>

#include "game_main.h"


// Simple 
int CheckInteraction(const std::vector<Interaction>& button_list)
{
	GetMouse();
	for (int i=0; i < button_list.length(); ++i)
	{
		const Interaction& b = button_list[i];
		if (key == b.key) return i;
		if (mousebuttons == 0) continue;
		if (b.x[0] <= x && x <= b.x[1]
		 && b.y[0] <= y && y <= b.y[1])
			return i;
	}
	return -1;
}
