#include <SDL3/SDL.h>

constexpr int WIDTH = 1024;
constexpr int HEIGHT = 1024;
constexpr const char *TITLE = "Pixel simulator engine";

int main() {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer(TITLE, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE, &window,
                                   &renderer)) {

    SDL_Log("Couldn't create SDL window: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  bool running = true;

  while (running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    SDL_Delay(16);
  }

  return 0;
}
