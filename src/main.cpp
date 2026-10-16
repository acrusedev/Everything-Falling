#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <utility>

constexpr int WIDTH = 1024;
constexpr int HEIGHT = 768;
constexpr const char *TITLE = "Pixel simulator engine";
constexpr double FIXED_SIMULATION_DELTA = 1.0 / 60.0;

enum MaterialKind { EMPTY, SAND };

class Material {
public:
  virtual ~Material() = default;
  virtual MaterialKind materialKind() const = 0;
  virtual uint32_t getColor() const = 0;
  virtual double getDensity() const = 0;
  bool canDisplace(Material &m) {
    return getDensity() > m.getDensity();
  };
};

class Air : public Material {
public:
  MaterialKind materialKind() const override { return MaterialKind::EMPTY; }
  uint32_t getColor() const override { return 0x00000000; }
  double getDensity() const override { return 0; }
};

class Sand : public Material {
public:
  MaterialKind materialKind() const override { return MaterialKind::SAND; }
  uint32_t getColor() const override { return 0xF5EB27FF; }
  double getDensity() const override { return 0; }
};

struct Cell {
  bool was_updated_this_iteration = false;
  std::unique_ptr<Material> material;

  Cell() : material(std::make_unique<Air>()) {}
  explicit Cell(std::unique_ptr<Material> m) : material(std::move(m)) {}
};

double get_current_time() {
  using namespace std::chrono;
  return duration<double>(steady_clock::now().time_since_epoch()).count();
}

uint32_t *init_pixels() {
  uint32_t *pixels =
      static_cast<uint32_t *>(malloc(WIDTH * HEIGHT * sizeof(uint32_t)));

  for (size_t i{}; i < WIDTH * HEIGHT; i++) {
    pixels[i] = 0xFFFFFFFF;
  }
  return pixels;
}

bool gravity_applies(Cell *cell) {
  if (cell->material->materialKind() == MaterialKind::SAND) {
    return 1;
  }
  return 0;
}

void update_pixels(Cell *cells, uint32_t *pixels) {
  for (size_t i{WIDTH * HEIGHT - WIDTH}; i-- > 0;) {
    if (cells[i].material->materialKind() != MaterialKind::EMPTY &&
        !cells[i].was_updated_this_iteration) {
      if (cells[i + 1].material->materialKind() == MaterialKind::EMPTY) {
        if (gravity_applies(&cells[i])) {
          pixels[i + WIDTH] = pixels[i];
          pixels[i] = 0xFFFFFFFF;
          std::swap(cells[i + WIDTH], cells[i]);
        }
      }
      cells[i + WIDTH].was_updated_this_iteration = true;
    }
  }
}

void reset_cell_update_state(Cell *cells) {
  for (int i = 0; i < WIDTH * HEIGHT; i++) {
    cells[i].was_updated_this_iteration = false;
  }
}

int main() {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  uint32_t *pixels = init_pixels();
  Cell *cells = new Cell[WIDTH * HEIGHT];

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

  double previous_frame_timestamp = get_current_time();
  double time_accumulator = 0.0;
  while (running) {
    const double current_frame_timestamp = get_current_time();
    time_accumulator += current_frame_timestamp - previous_frame_timestamp;
    previous_frame_timestamp = current_frame_timestamp;

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      SDL_ConvertEventToRenderCoordinates(renderer, &event);
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        const int x = static_cast<int>(event.button.x);
        const int y = static_cast<int>(event.button.y);

        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
          pixels[y * WIDTH + x] = 0x000000FF;
        }
      }
    }

    while (time_accumulator >= FIXED_SIMULATION_DELTA) {
      reset_cell_update_state(cells);
      update_pixels(cells, pixels);
      time_accumulator -= FIXED_SIMULATION_DELTA;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_UpdateTexture(texture, nullptr, pixels, WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer); // needed for wayland
  }
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  free(pixels);
  delete[] cells;
  return 0;
}
