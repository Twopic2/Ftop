#include <stdio.h>
#include <ncurses.h>

#define SINEWIDTH 60

typedef struct {
    int values[SINEWIDTH];
    int index; 

} graphSinwave;


void displaySineway(int y_pos, int x_pos, const graphSinwave* gh, const char* label, int max_value_for_scale) {
    

    


}