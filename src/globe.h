#ifndef GLOBE_H
#define GLOBE_H
#include <GL/glew.h>
#include <SDL2/SDL.h>

typedef struct {
    GLuint vao, vbo, ebo;
    GLuint iss_vao, iss_vbo;
    GLuint texture;
    GLuint shader;
    GLuint iss_shader;
    int index_count;
    float rotation;
    float tilt;
    float iss_lat, iss_lon;
    GLuint fbo, fbo_tex, rbo;
    int fbo_w, fbo_h;
    float tint_r, tint_g, tint_b;
    int iss_centered;
    int dragging;
    int last_mouse_x, last_mouse_y;
} Globe;

int  globe_init(Globe *g, int fbo_w, int fbo_h);
void globe_update(Globe *g, float dt, float iss_lat, float iss_lon);
void globe_handle_event(Globe *g, SDL_Event *e, int panel_x, int panel_y, int panel_w, int panel_h);
void globe_blit_to_surface(Globe *g, SDL_Surface *canvas, int x, int y, int w, int h, int win_w, int win_h);
void globe_destroy(Globe *g);
#endif
