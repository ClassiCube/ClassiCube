#include <string.h>
#include <math.h>
#include <stdio.h>

#include "private.h"

Viewport VIEWPORT = {
    0, 0, 640, 480, 320.0f, 240.0f, 320.0f, 240.0f
};

void _glInitMatrices() {
    const VideoMode* vid_mode = GetVideoMode();

    glViewport(0, 0, vid_mode->width, vid_mode->height);
}

/* Set the GL viewport */
void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    VIEWPORT.x = x;
    VIEWPORT.y = y;
    VIEWPORT.width = width;
    VIEWPORT.height = height;
    VIEWPORT.hwidth = ((GLfloat) VIEWPORT.width) * 0.5f;
    VIEWPORT.hheight = ((GLfloat) VIEWPORT.height) * 0.5f;
    VIEWPORT.x_plus_hwidth = VIEWPORT.x + VIEWPORT.hwidth;
    VIEWPORT.y_plus_hheight = VIEWPORT.y + VIEWPORT.hheight;
}
