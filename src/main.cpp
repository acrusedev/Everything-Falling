#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <cstdint>
#include <cstdlib>

constexpr int WIDTH = 1024;
constexpr int HEIGHT = 768;
constexpr const char *TITLE = "Pixel simulator engine";

uint32_t *init_pixels() {
  uint32_t *pixels =
      static_cast<uint32_t *>(malloc(WIDTH * HEIGHT * sizeof(uint32_t)));
  for (uint32_t i = 0; i < WIDTH * HEIGHT; i++) {
    pixels[i] = 0xFFFFFFFF;
  }
  return pixels;
}

int main() {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  uint32_t *pixels = init_pixels();

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer(TITLE, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE,
                                   &window, &renderer)) {
    SDL_Log("Couldn't create SDL window: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

 if (!texture) {
    SDL_Log("Couldn't create texture: %s", SDL_GetError());
    return 1;
  }

  SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);

  bool running = true;

  while (running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      SDL_ConvertEventToRenderCoordinates(renderer, &event);
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        int x = (int)event.button.x;
        int y = (int)event.button.y;

        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
          pixels[y * WIDTH + x] = 0x000000FF;
        }
      }
    }
  
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_UpdateTexture(texture, nullptr, pixels, WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer); // needed for wayland

    SDL_Delay(16);
  }
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  free(pixels);
  return 0;
}
