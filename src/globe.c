#include "globe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define PI 3.14159265358979323846f
#define STACKS 32
#define SECTORS 64

static char *read_file(const char *path) {
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

static GLuint compile_shader(const char *path, GLenum type) {
    char *src = read_file(path);
    if (!src) { fprintf(stderr, "Shader read fail: %s\n", path); return 0; }
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, (const char **)&src, NULL);
    glCompileShader(s);
    free(src);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        fprintf(stderr, "Shader compile error (%s): %s\n", path, log);
    }
    return s;
}

static GLuint build_program(const char *vp, const char *fp) {
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

static void mat4_identity(float *m) {
    memset(m, 0, 16*sizeof(float));
    m[0]=m[5]=m[10]=m[15]=1.0f;
}

static void mat4_rotate_y(float *m, float a) {
    mat4_identity(m);
    m[0]=cosf(a); m[2]=sinf(a);
    m[8]=-sinf(a); m[10]=cosf(a);
}

static void mat4_perspective(float *m, float fov, float asp, float n, float f) {
    memset(m, 0, 16*sizeof(float));
    float t = 1.0f/tanf(fov/2.0f);
    m[0]=t/asp; m[5]=t;
    m[10]=(f+n)/(n-f); m[11]=-1.0f;
    m[14]=(2.0f*f*n)/(n-f);
}

static void mat4_lookat(float *m,
    float ex,float ey,float ez,
    float cx,float cy,float cz,
    float ux,float uy,float uz) {
    float fx=cx-ex,fy=cy-ey,fz=cz-ez;
    float fl=sqrtf(fx*fx+fy*fy+fz*fz);
    fx/=fl;fy/=fl;fz/=fl;
    float rx=fy*uz-fz*uy,ry=fz*ux-fx*uz,rz=fx*uy-fy*ux;
    float rl=sqrtf(rx*rx+ry*ry+rz*rz);
    rx/=rl;ry/=rl;rz/=rl;
    float ux2=ry*fz-rz*fy,uy2=rz*fx-rx*fz,uz2=rx*fy-ry*fx;
    mat4_identity(m);
    m[0]=rx;m[4]=ry;m[8]=rz;
    m[1]=ux2;m[5]=uy2;m[9]=uz2;
    m[2]=-fx;m[6]=-fy;m[10]=-fz;
    m[12]=-(rx*ex+ry*ey+rz*ez);
    m[13]=-(ux2*ex+uy2*ey+uz2*ez);
    m[14]=(fx*ex+fy*ey+fz*ez);
}

int globe_init(Globe *g, int fbo_w, int fbo_h) {
    memset(g, 0, sizeof(Globe));
    g->fbo_w = fbo_w;
    g->fbo_h = fbo_h;
    g->tint_r = 0.0f;
    g->tint_g = 1.0f;
    g->tint_b = 0.25f;

    g->shader = build_program("shaders/globe_vert.glsl", "shaders/globe_frag.glsl");
    g->iss_shader = build_program("shaders/iss_vert.glsl", "shaders/iss_frag.glsl");

    int vc = (STACKS+1)*(SECTORS+1);
    float *verts = malloc(vc*8*sizeof(float));
    int vi = 0;
    for (int i = 0; i <= STACKS; i++) {
        float sa = PI/2 - i*(PI/STACKS);
        float xy = cosf(sa), z = sinf(sa);
        for (int j = 0; j <= SECTORS; j++) {
            float ca = j*(2*PI/SECTORS);
            float x = xy*cosf(ca), y = xy*sinf(ca);
            verts[vi++]=x; verts[vi++]=z; verts[vi++]=y;
            verts[vi++]=(float)j/SECTORS; verts[vi++]=(float)i/STACKS;
            verts[vi++]=x; verts[vi++]=z; verts[vi++]=y;
        }
    }

    g->index_count = STACKS*SECTORS*6;
    unsigned int *idx = malloc(g->index_count*sizeof(unsigned int));
    int ii = 0;
    for (int i = 0; i < STACKS; i++) {
        for (int j = 0; j < SECTORS; j++) {
            int k1=i*(SECTORS+1)+j, k2=k1+SECTORS+1;
            idx[ii++]=k1; idx[ii++]=k2; idx[ii++]=k1+1;
            idx[ii++]=k1+1; idx[ii++]=k2; idx[ii++]=k2+1;
        }
    }

    glGenVertexArrays(1,&g->vao);
    glGenBuffers(1,&g->vbo);
    glGenBuffers(1,&g->ebo);
    glBindVertexArray(g->vao);
    glBindBuffer(GL_ARRAY_BUFFER,g->vbo);
    glBufferData(GL_ARRAY_BUFFER,vc*8*sizeof(float),verts,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,g->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,g->index_count*sizeof(unsigned int),idx,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(5*sizeof(float)));
    glEnableVertexAttribArray(2);
    free(verts); free(idx);

    glGenVertexArrays(1,&g->iss_vao);
    glGenBuffers(1,&g->iss_vbo);
    glBindVertexArray(g->iss_vao);
    glBindBuffer(GL_ARRAY_BUFFER,g->iss_vbo);
    float dummy[3] = {0,0,0};
    glBufferData(GL_ARRAY_BUFFER,sizeof(dummy),dummy,GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    stbi_set_flip_vertically_on_load(1);
    int tw,th,tc;
    unsigned char *data = stbi_load("assets/earth_map.jpg",&tw,&th,&tc,3);
    if (!data) { fprintf(stderr,"Earth texture load failed\n"); return 0; }
    glGenTextures(1,&g->texture);
    glBindTexture(GL_TEXTURE_2D,g->texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,tw,th,0,GL_RGB,GL_UNSIGNED_BYTE,data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    stbi_image_free(data);

    glGenFramebuffers(1,&g->fbo);
    glGenTextures(1,&g->fbo_tex);
    glBindTexture(GL_TEXTURE_2D,g->fbo_tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,fbo_w,fbo_h,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER,g->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,g->fbo_tex,0);
    glGenRenderbuffers(1,&g->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER,g->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT,fbo_w,fbo_h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,g->rbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
        fprintf(stderr,"Globe FBO incomplete!\n");
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    return 1;
}

void globe_update(Globe *g, float dt, float iss_lat, float iss_lon) {
    float prev_lat = g->iss_lat;
    float prev_lon = g->iss_lon;
    g->iss_lat = iss_lat;
    g->iss_lon = iss_lon;

    // First time we get valid ISS data, rotate to center it
    if (!g->iss_centered && (iss_lat != 0.0f || iss_lon != 0.0f)) {
        float target = -(iss_lon * 3.14159f / 180.0f);
        float diff = target - g->rotation;
        while (diff > 3.14159f) diff -= 2*3.14159f;
        while (diff < -3.14159f) diff += 2*3.14159f;
        g->rotation += diff * dt * 2.0f;
        if (fabsf(diff) < 0.01f) {
            g->rotation = target;
            g->iss_centered = 1;
        }
    }
    (void)prev_lat; (void)prev_lon;
}

void globe_handle_event(Globe *g, SDL_Event *e, int panel_x, int panel_y, int panel_w, int panel_h) {
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        int mx = e->button.x;
        int my = e->button.y;
        if (mx >= panel_x && mx <= panel_x + panel_w &&
            my >= panel_y && my <= panel_y + panel_h) {
            g->dragging = 1;
            g->last_mouse_x = mx;
            g->last_mouse_y = my;
            g->iss_centered = 1;
        }
    }
    if (e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT) {
        g->dragging = 0;
    }
    if (e->type == SDL_MOUSEMOTION && g->dragging) {
        int dx = e->motion.x - g->last_mouse_x;
        int dy = e->motion.y - g->last_mouse_y;
        g->rotation += dx * 0.005f;
        g->tilt += dy * 0.005f;
        if (g->tilt > 1.5f) g->tilt = 1.5f;
        if (g->tilt < -1.5f) g->tilt = -1.5f;
        g->last_mouse_x = e->motion.x;
        g->last_mouse_y = e->motion.y;
    }
}

void globe_blit_to_surface(Globe *g, SDL_Surface *canvas, int x, int y, int w, int h, int win_w, int win_h) {
    glBindFramebuffer(GL_FRAMEBUFFER, g->fbo);
    glViewport(0, 0, g->fbo_w, g->fbo_h);
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    float model[16], view[16], proj[16];
    mat4_rotate_y(model, g->rotation);
    mat4_lookat(view, 0,0,2.5f, 0,0,0, 0,-1,0);
    mat4_perspective(proj, PI/2, (float)g->fbo_w/g->fbo_h, 0.1f, 100.0f);

    // Draw globe
    glUseProgram(g->shader);
    glUniformMatrix4fv(glGetUniformLocation(g->shader,"model"),1,GL_FALSE,model);
    glUniformMatrix4fv(glGetUniformLocation(g->shader,"view"),1,GL_FALSE,view);
    glUniformMatrix4fv(glGetUniformLocation(g->shader,"proj"),1,GL_FALSE,proj);
    float lx = cosf(g->rotation*0.3f);
    float lz = sinf(g->rotation*0.3f);
    glUniform3f(glGetUniformLocation(g->shader,"light_dir"),lx,0.5f,lz);
    glUniform3f(glGetUniformLocation(g->shader,"tint_color"),g->tint_r,g->tint_g,g->tint_b);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g->texture);
    glUniform1i(glGetUniformLocation(g->shader,"earth_tex"),0);
    glBindVertexArray(g->vao);
    glDrawElements(GL_TRIANGLES, g->index_count, GL_UNSIGNED_INT, 0);

    // Draw ISS marker
    float phi = g->iss_lat * PI / 180.0f;
    float lam = g->iss_lon * PI / 180.0f + PI;
    float ix = cosf(phi) * cosf(lam);
    float iy = sinf(phi);
    float iz = cosf(phi) * sinf(lam);
    float iss_pos[3] = { ix, iy, iz };

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBindVertexArray(g->iss_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g->iss_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(iss_pos), iss_pos);
    glUseProgram(g->iss_shader);
    glUniformMatrix4fv(glGetUniformLocation(g->iss_shader,"model"),1,GL_FALSE,model);
    glUniformMatrix4fv(glGetUniformLocation(g->iss_shader,"view"),1,GL_FALSE,view);
    glUniformMatrix4fv(glGetUniformLocation(g->iss_shader,"proj"),1,GL_FALSE,proj);
    glUniform3f(glGetUniformLocation(g->iss_shader,"tint_color"),g->tint_r,g->tint_g,g->tint_b);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, 1);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);

    unsigned char *pixels = malloc(g->fbo_w * g->fbo_h * 4);
    glReadPixels(0, 0, g->fbo_w, g->fbo_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, win_w, win_h);

    int copy_w = w < g->fbo_w ? w : g->fbo_w;
    int copy_h = h < g->fbo_h ? h : g->fbo_h;
    for (int row = 0; row < copy_h; row++) {
        for (int col = 0; col < copy_w; col++) {
            int src_row = g->fbo_h - 1 - row;
            int src_idx = (src_row * g->fbo_w + col) * 4;
            int dst_x = x + col;
            int dst_y = y + row;
            if (dst_x >= canvas->w || dst_y >= canvas->h) continue;
            Uint32 *p = (Uint32*)((Uint8*)canvas->pixels + dst_y*canvas->pitch + dst_x*4);
            *p = SDL_MapRGB(canvas->format, pixels[src_idx], pixels[src_idx+1], pixels[src_idx+2]);
        }
    }
    free(pixels);
}

void globe_destroy(Globe *g) {
    glDeleteVertexArrays(1,&g->vao);
    glDeleteBuffers(1,&g->vbo);
    glDeleteBuffers(1,&g->ebo);
    glDeleteVertexArrays(1,&g->iss_vao);
    glDeleteBuffers(1,&g->iss_vbo);
    glDeleteTextures(1,&g->texture);
    glDeleteFramebuffers(1,&g->fbo);
    glDeleteTextures(1,&g->fbo_tex);
    glDeleteRenderbuffers(1,&g->rbo);
    glDeleteProgram(g->shader);
    glDeleteProgram(g->iss_shader);
}
