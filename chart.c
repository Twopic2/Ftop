#include <stdio.h>
#include <ncurses.h>

typedef struct {
    int* values;
    int width;
    int index; 
    int height;
} graphSinwave;

void initGraph(graphSinwave* graph, int width, int height) {
    graph -> values = (int*)malloc(width * sizeof(int));
    graph -> height = height;
    graph -> width = width;
    graph -> index = 0;
}

void freeGraph(graphSinwave* graph) {
    free(graph -> values);
    graph -> height = 0;
    graph -> values = NULL;
    graph -> width = 0;
}

void displaySineway(int y_pos, int x_pos, const graphSinwave* gh, const char* label, int maxScale) {

    int height = gh->height;

    mvprintw(y_pos, x_pos, "%s", label);
    y_pos += 1;

    for (int i = 0; i < gh -> width + 2; i++) {
        mvaddch(y_pos + height + 1, x_pos + i, '-');
    }

    for (int i = 0; i <= height + 1; i++) {
        mvaddch(y_pos + i, x_pos, '|');
    }

    mvprintw(y_pos, x_pos - 4, "%4d", maxScale);
    mvprintw(y_pos + height / 2, x_pos - 4, "%4d", 0);
    mvprintw(y_pos + height, x_pos - 4, "%4d", - maxScale);
    
    for (int i = 0; i < gh->width; i++) {
        int idx = (gh -> index + i) % gh -> width;
        int value = gh->values[idx];
        
        int y_offset = (int)(height * (0.5 - (float)value / (2 * maxScale)));

        if (y_offset < 0) y_offset = 0;
        if (y_offset > height) y_offset = height;
        
        mvaddch(y_pos + y_offset, x_pos + i + 1, '*');
    }
}
