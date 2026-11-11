#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "launcher.h"
#include "platform.h"
#include "render.h"
#include "text.h"

static int map_key(SDL_Keycode sym, LauncherKey *out) {
    switch (sym) {
        case SDLK_LEFT:
            *out = CH_KEY_LEFT;
            return 1;
        case SDLK_RIGHT:
            *out = CH_KEY_RIGHT;
            return 1;
        case SDLK_UP:
            *out = CH_KEY_UP;
            return 1;
        case SDLK_DOWN:
            *out = CH_KEY_DOWN;
            return 1;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            *out = CH_KEY_OK;
            return 1;
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE:
        case SDLK_AC_BACK:
            *out = CH_KEY_BACK;
            return 1;
        case SDLK_AUDIOPLAY:
            *out = CH_KEY_MEDIA_PLAY;
            return 1;
        case SDLK_AUDIONEXT:
            *out = CH_KEY_MEDIA_NEXT;
            return 1;
        case SDLK_AUDIOPREV:
            *out = CH_KEY_MEDIA_PREV;
            return 1;
        default:
            return 0;
    }
}


static void press(Launcher *launcher, LauncherKey key) {
    launcher_key(launcher, key, 1, 0);
    launcher_key(launcher, key, 0, 0);
}

static void nav(Launcher *launcher, LauncherKey key) {
    launcher_key(launcher, key, 1, 0);
}

static void apply_state(Launcher *launcher, const char *state) {
    if (!state || strcmp(state, "home") == 0) return;
    if (strcmp(state, "grid") == 0) {
        nav(launcher, CH_KEY_UP);
        nav(launcher, CH_KEY_RIGHT);
        nav(launcher, CH_KEY_RIGHT);
        press(launcher, CH_KEY_OK);
        nav(launcher, CH_KEY_RIGHT);
        nav(launcher, CH_KEY_RIGHT);
        return;
    }
    if (strcmp(state, "controls") == 0) {
        nav(launcher, CH_KEY_UP);
        for (int i = 0; i < 5; ++i) nav(launcher, CH_KEY_RIGHT);
        press(launcher, CH_KEY_OK);
        return;
    }
    if (strcmp(state, "wallpaper") == 0) {
        nav(launcher, CH_KEY_UP);
        for (int i = 0; i < 4; ++i) nav(launcher, CH_KEY_RIGHT);
        press(launcher, CH_KEY_OK);
        return;
    }
}

static int write_ppm(const char *path, const unsigned char *pixels, int width,
                     int height) {
    FILE *file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "cannot write %s\n", path);
        return -1;
    }
    fprintf(file, "P6\n%d %d\n255\n", width, height);
    unsigned char *row = (unsigned char *)malloc((size_t)width * 3);
    if (!row) {
        fclose(file);
        return -1;
    }
    for (int y = height - 1; y >= 0; --y) {
        const unsigned char *src = pixels + (size_t)y * (size_t)width * 4;
        for (int x = 0; x < width; ++x) {
            row[x * 3 + 0] = src[x * 4 + 0];
            row[x * 3 + 1] = src[x * 4 + 1];
            row[x * 3 + 2] = src[x * 4 + 2];
        }
        fwrite(row, 3, (size_t)width, file);
    }
    free(row);
    fclose(file);
    return 0;
}

static int take_screenshot(Launcher *launcher, Renderer *renderer,
                           const char *path) {
    GLuint texture = 0;
    GLuint framebuffer = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1920, 1080, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "framebuffer incomplete\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texture);
        return -1;
    }

    render_set_size(renderer, 1920, 1080);
    launcher_render(launcher);
    glFinish();

    unsigned char *pixels =
        (unsigned char *)malloc((size_t)1920 * 1080 * 4);
    if (!pixels) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texture);
        return -1;
    }
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    int result = write_ppm(path, pixels, 1920, 1080);
    free(pixels);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &texture);
    return result;
}

int main(int argc, char **argv) {
    const char *screenshot_path = NULL;
    const char *state = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            screenshot_path = argv[++i];
        } else if (strncmp(argv[i], "--state=", 8) == 0) {
            state = argv[i] + 8;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_Window *window = SDL_CreateWindow(
        "CleanHome Native", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screenshot_path ? 1920 : 1280, screenshot_path ? 1080 : 720,
        SDL_WINDOW_OPENGL | (screenshot_path ? SDL_WINDOW_HIDDEN : SDL_WINDOW_RESIZABLE));
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_SetSwapInterval(1);

    if (platform_init() != 0) {
        fprintf(stderr, "platform_init failed\n");
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Renderer *renderer = NULL;
    if (render_init(&renderer) != 0) {
        fprintf(stderr, "render_init failed\n");
        platform_shutdown();
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    render_set_size(renderer, 1280, 720);

    Fonts *fonts = NULL;
    if (text_init(&fonts, platform_asset_root()) != 0) {
        fprintf(stderr, "text_init failed\n");
        render_shutdown(renderer);
        platform_shutdown();
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Images *images = NULL;
    if (image_init(&images) != 0) {
        fprintf(stderr, "image_init failed\n");
        text_shutdown(fonts);
        render_shutdown(renderer);
        platform_shutdown();
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Launcher *launcher = NULL;
    if (launcher_init(&launcher, renderer, fonts, images) != 0) {
        fprintf(stderr, "launcher_init failed\n");
        image_shutdown(images);
        text_shutdown(fonts);
        render_shutdown(renderer);
        platform_shutdown();
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (screenshot_path) {
        apply_state(launcher, state);
        launcher_tick(launcher);
        int result = take_screenshot(launcher, renderer, screenshot_path);
        launcher_shutdown(launcher);
        image_shutdown(images);
        text_shutdown(fonts);
        render_shutdown(renderer);
        platform_shutdown();
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return result == 0 ? 0 : 1;
    }

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_WINDOWEVENT &&
                       event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                render_set_size(renderer, event.window.data1, event.window.data2);
            } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                LauncherKey key;
                if (map_key(event.key.keysym.sym, &key)) {
                    launcher_key(launcher, key, event.type == SDL_KEYDOWN,
                                 event.key.repeat);
                }
                if (event.type == SDL_KEYDOWN &&
                    event.key.keysym.sym == SDLK_q &&
                    (event.key.keysym.mod & KMOD_CTRL)) {
                    running = 0;
                }
            }
        }

        launcher_tick(launcher);
        launcher_render(launcher);
        SDL_GL_SwapWindow(window);
        SDL_Delay(1);
    }

    launcher_shutdown(launcher);
    image_shutdown(images);
    text_shutdown(fonts);
    render_shutdown(renderer);
    platform_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
