#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysinfo.h"
#include "alert.h"
#include "globe.h"
#include "iss.h"

#define FONT_SIZE 16
#define FONT_PATH "fonts/Px437_IBM_VGA_8x16.ttf"

typedef struct {
    SDL_Color primary;
    SDL_Color secondary;
    SDL_Color warning;
    SDL_Color critical;
    SDL_Color background;
    char name[32];
} SDLTheme;

static SDLTheme themes[4] = {
    {{0,255,65,255},   {0,180,40,255},   {255,170,0,255},{255,50,50,255}, {0,0,0,255},    "PHOSPHOR GREEN"},
    {{255,176,0,255},  {200,120,0,255},  {255,80,0,255}, {255,30,30,255}, {0,0,0,255},    "AMBER"},
    {{0,212,255,255},  {0,140,180,255},  {255,170,0,255},{255,50,50,255}, {0,8,20,255},   "COLD CYAN"},
    {{220,220,220,255},{140,140,140,255},{240,165,0,255},{232,64,64,255}, {13,13,13,255}, "MINIMAL WHITE"},
};

static int WIN_W = 1920;
static int WIN_H = 1080;

char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

GLuint compile_shader(const char *path, GLenum type) {
    char *src = read_file(path);
    if (!src) { fprintf(stderr, "Failed to read: %s\n", path); return 0; }
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, (const char **)&src, NULL);
    glCompileShader(s);
    free(src);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        fprintf(stderr, "Shader error (%s): %s\n", path, log);
    }
    return s;
}

GLuint build_program(const char *vp, const char *fp) {
    GLuint v = compile_shader(vp, GL_VERTEX_SHADER);
    GLuint f = compile_shader(fp, GL_FRAGMENT_SHADER);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

SDL_Surface *create_canvas(void) {
    return SDL_CreateRGBSurface(0, WIN_W, WIN_H, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
}

void canvas_clear(SDL_Surface *s, SDL_Color c) {
    SDL_FillRect(s, NULL, SDL_MapRGB(s->format, c.r, c.g, c.b));
}

void canvas_rect(SDL_Surface *s, int x, int y, int w, int h, SDL_Color c) {
    Uint32 col = SDL_MapRGB(s->format, c.r, c.g, c.b);
    SDL_Rect rects[4] = {
        {x,     y,     w, 1},
        {x,     y+h-1, w, 1},
        {x,     y,     1, h},
        {x+w-1, y,     1, h},
    };
    for (int i = 0; i < 4; i++)
        SDL_FillRect(s, &rects[i], col);
}

void canvas_text(SDL_Surface *canvas, TTF_Font *font,
                 const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surf = TTF_RenderText_Solid(font, text, color);
    if (!surf) return;
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_BlitSurface(surf, NULL, canvas, &dst);
    SDL_FreeSurface(surf);
}

void canvas_panel(SDL_Surface *canvas, TTF_Font *font,
                  int x, int y, int w, int h,
                  const char *title, SDL_Color color, SDL_Color bg) {
    canvas_rect(canvas, x, y, w, h, color);
    char header[64];
    snprintf(header, sizeof(header), "[ %s ]", title);
    int tw, th;
    TTF_SizeText(font, header, &tw, &th);
    SDL_Rect clear = {x + 8, y - th/2, tw + 4, th};
    SDL_FillRect(canvas, &clear, SDL_MapRGB(canvas->format, bg.r, bg.g, bg.b));
    canvas_text(canvas, font, header, x + 10, y - th/2, color);
}

void upload_canvas(SDL_Surface *canvas, GLuint tex) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIN_W, WIN_H,
        GL_BGRA, GL_UNSIGNED_BYTE, canvas->pixels);
}

void render_canvas(GLuint prog, GLuint tex, GLuint vao, float t,
                   float bloom, float scanline, float curvature, float flicker) {
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "screen"), 0);
    glUniform1f(glGetUniformLocation(prog, "time"), t);
    glUniform1f(glGetUniformLocation(prog, "bloom_strength"), bloom);
    glUniform1f(glGetUniformLocation(prog, "scanline_strength"), scanline);
    glUniform1f(glGetUniformLocation(prog, "curvature"), curvature);
    glUniform1f(glGetUniformLocation(prog, "flicker"), flicker);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

int show_theme_select(SDL_Window *win, TTF_Font *font,
                      GLuint prog, GLuint tex, GLuint vao) {
    SDL_Color white = {200,200,200,255};
    SDL_Color dim   = {80,80,80,255};
    SDL_Event e;
    SDL_Surface *canvas = create_canvas();

    while (1) {
        canvas_clear(canvas, (SDL_Color){0,0,0,255});
        canvas_text(canvas, font, "HELIOS MISSION CONTROL v1.0.0", 40, 40, white);
        canvas_text(canvas, font, "SELECT THEME:", 40, 90, dim);
        for (int i = 0; i < 4; i++) {
            char line[64];
            snprintf(line, sizeof(line), "[%d]  %s", i+1, themes[i].name);
            canvas_text(canvas, font, line, 60, 130 + i*30, themes[i].primary);
        }
        canvas_text(canvas, font, "PRESS 1-4 TO SELECT", 40, 280, dim);
        upload_canvas(canvas, tex);
        glClear(GL_COLOR_BUFFER_BIT);
        render_canvas(prog, tex, vao, 0.0f, 0.4f, 0.08f, 0.02f, 0.3f);
        SDL_GL_SwapWindow(win);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { SDL_FreeSurface(canvas); return -1; }
            if (e.type == SDL_KEYDOWN) {
                switch(e.key.keysym.sym) {
                    case SDLK_1: SDL_FreeSurface(canvas); return 0;
                    case SDLK_2: SDL_FreeSurface(canvas); return 1;
                    case SDLK_3: SDL_FreeSurface(canvas); return 2;
                    case SDLK_4: SDL_FreeSurface(canvas); return 3;
                    case SDLK_ESCAPE: SDL_FreeSurface(canvas); return -1;
                }
            }
        }
        SDL_Delay(16);
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    SetProcessDPIAware();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF init failed: %s\n", TTF_GetError());
        return 1;
    }

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    WIN_W = dm.w;
    WIN_H = dm.h;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window *win = SDL_CreateWindow(
        "HELIOS MISSION CONTROL",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "GLEW init failed\n");
        return 1;
    }

    SDL_GL_GetDrawableSize(win, &WIN_W, &WIN_H);
    glViewport(0, 0, WIN_W, WIN_H);

    TTF_Font *font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (!font) {
        fprintf(stderr, "Font load failed: %s\n", TTF_GetError());
        return 1;
    }

    GLuint prog = build_program("shaders/vert.glsl", "shaders/frag.glsl");

    float verts[] = {
        -1.0f,  1.0f,  0.0f, 0.0f,
        -1.0f, -1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 0.0f,
    };
    unsigned int indices[] = {0,1,2,0,2,3};

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIN_W, WIN_H, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int theme_id = show_theme_select(win, font, prog, tex, vao);
    if (theme_id < 0) {
        SDL_GL_DeleteContext(gl_ctx);
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    SDLTheme theme = themes[theme_id];
    SysInfo info = {0};
    AlertLog alerts;
    alert_init(&alerts);
    alert_push(&alerts, ALERT_INFO, "HELIOS ONLINE");

    int panel_y = 50;
    int panel_h = WIN_H - panel_y - 10;
    int panel_w = (WIN_W - 30) / 3;

    Globe globe;
    globe_init(&globe, panel_w - 40, panel_h - 20);
    iss_init();

    ISSData last_iss = {0};
    Uint32 last_update = 0;
    int running = 1;
    SDL_Event e;
    SDL_Surface *canvas = create_canvas();

    int globe_x = 20 + panel_w + 20;
    int globe_y = panel_y + 10;
    int globe_w = panel_w - 40;
    int globe_h = panel_h - 20;
    int tx = 20 + panel_w + 10;
    int ty = panel_y + panel_h - 120;

    while (running) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q) running = 0;
            globe_handle_event(&globe, &e, globe_x, globe_y, globe_w, globe_h);
        }

        Uint32 now = SDL_GetTicks();
        float t = now / 1000.0f;

        if (now - last_update > 1000) {
            sysinfo_update(&info);
            ISSData fresh = iss_get();
            if (fresh.valid) {
                last_iss = fresh;
            }
            alert_check_sysinfo(&alerts, info.cpu_usage, info.mem_percent);
            last_update = now;

            canvas_clear(canvas, theme.background);

            canvas_text(canvas, font, "HELIOS MISSION CONTROL v1.0.0", 20, 10, theme.primary);
            Uint32 met = now / 1000;
            char met_buf[32];
            snprintf(met_buf, sizeof(met_buf), "MET: %02d:%02d:%02d",
                met/3600, (met%3600)/60, met%60);
            canvas_text(canvas, font, met_buf, WIN_W/2 - 60, 10, theme.primary);
            canvas_text(canvas, font, "PRESS Q TO EXIT", WIN_W - 200, 10, theme.secondary);

            canvas_panel(canvas, font, 10, panel_y, panel_w, panel_h,
                "SUBSYSTEMS", theme.primary, theme.background);
            canvas_panel(canvas, font, 20 + panel_w, panel_y, panel_w, panel_h,
                "TELEMETRY", theme.primary, theme.background);
            canvas_panel(canvas, font, 30 + panel_w*2, panel_y, panel_w, panel_h,
                "ALERT LOG", theme.primary, theme.background);

            char buf[64];
            int lx = 18, ly = panel_y + 16;
            snprintf(buf, sizeof(buf), "CPU USAGE  : %.1f%%", info.cpu_usage);
            canvas_text(canvas, font, buf, lx, ly, theme.primary);
            snprintf(buf, sizeof(buf), "RAM USED   : %ld MB", info.mem_used / 1024);
            canvas_text(canvas, font, buf, lx, ly+24, theme.primary);
            snprintf(buf, sizeof(buf), "RAM TOTAL  : %ld MB", info.mem_total / 1024);
            canvas_text(canvas, font, buf, lx, ly+48, theme.primary);
            snprintf(buf, sizeof(buf), "RAM USAGE  : %.1f%%", info.mem_percent);
            SDL_Color ram_col = info.mem_percent > 80 ? theme.critical :
                                info.mem_percent > 60 ? theme.warning : theme.primary;
            canvas_text(canvas, font, buf, lx, ly+72, ram_col);

            int ax = 30 + panel_w*2 + 10;
            int ay = panel_y + 20;
            for (int i = 0; i < alerts.count && i < 28; i++) {
                int idx = ((alerts.head - 1 - i) + MAX_ALERTS) % MAX_ALERTS;
                Alert *a = &alerts.entries[idx];
                SDL_Color col = theme.primary;
                char prefix[8];
                if (a->level == ALERT_CRIT) {
                    col = theme.critical;
                    snprintf(prefix, sizeof(prefix), "[CRIT]");
                } else if (a->level == ALERT_WARN) {
                    col = theme.warning;
                    snprintf(prefix, sizeof(prefix), "[WARN]");
                } else {
                    snprintf(prefix, sizeof(prefix), "[INFO]");
                }
                char line[128];
                snprintf(line, sizeof(line), "%s %s %s", a->timestamp, prefix, a->message);
                canvas_text(canvas, font, line, ax, ay + i * 20, col);
            }

            globe.tint_r = theme.primary.r / 255.0f;
            globe.tint_g = theme.primary.g / 255.0f;
            globe.tint_b = theme.primary.b / 255.0f;
        }

        globe_update(&globe, 0.016f, last_iss.latitude, last_iss.longitude);
        globe_blit_to_surface(&globe, canvas, globe_x, globe_y, globe_w, globe_h, WIN_W, WIN_H);

        char iss_buf[64];
        if (last_iss.valid) {
            snprintf(iss_buf, sizeof(iss_buf), "ISS LAT  : %.2f", last_iss.latitude);
            canvas_text(canvas, font, iss_buf, tx, ty, theme.primary);
            snprintf(iss_buf, sizeof(iss_buf), "ISS LON  : %.2f", last_iss.longitude);
            canvas_text(canvas, font, iss_buf, tx, ty+20, theme.primary);
            snprintf(iss_buf, sizeof(iss_buf), "ISS ALT  : %.0f km", last_iss.altitude);
            canvas_text(canvas, font, iss_buf, tx, ty+40, theme.primary);
            snprintf(iss_buf, sizeof(iss_buf), "ISS VEL  : %.2f km/s", last_iss.velocity);
            canvas_text(canvas, font, iss_buf, tx, ty+60, theme.primary);
        } else {
            canvas_text(canvas, font, "ISS      : ACQUIRING...", tx, ty, theme.secondary);
        }

        upload_canvas(canvas, tex);
        render_canvas(prog, tex, vao, t, 0.8f, 0.15f, 0.02f, 1.0f);
        SDL_GL_SwapWindow(win);
        SDL_Delay(16);
    }

    iss_destroy();
    SDL_FreeSurface(canvas);
    globe_destroy(&globe);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteTextures(1, &tex);
    glDeleteProgram(prog);
    TTF_CloseFont(font);
    SDL_GL_DeleteContext(gl_ctx);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
