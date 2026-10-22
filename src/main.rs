use sdl3::event::Event;
use sdl3::pixels::{Color, PixelFormat};
use sdl3::render::ScaleMode;
use sdl3::sys::render::SDL_RendererLogicalPresentation;
use std::time::Instant;

const TITLE: &str = "Pixel simulator engine";
const WIDTH: usize = 1024;
const HEIGHT: usize = 768;
const FIXED_DT: f64 = 1.0 / 60.0;

#[derive(Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
enum MaterialKind {
    Air,
    Sand,
}

struct MaterialProps {
    color: u32,
    density: f64,
}

const PROPS: [MaterialProps; 2] = [
    /* Air  */ MaterialProps { color: 0x0000_0000, density: 0.2 },
    /* Sand */ MaterialProps { color: 0xF5EB_27FF, density: 1.0 },
];

impl MaterialKind {
    fn props(self) -> &'static MaterialProps {
        &PROPS[self as usize]
    }
    fn can_displace(self, other: MaterialKind) -> bool {
        self.props().density > other.props().density
    }
}

#[derive(Clone, Copy)]
struct Cell {
    kind: MaterialKind,
    was_updated: bool,
}

const EMPTY: Cell = Cell { kind: MaterialKind::Air, was_updated: false };

fn reset_update_state(cells: &mut [Cell]) {
    for c in cells.iter_mut() {
        c.was_updated = false;
    }
}

fn update_cells(cells: &mut [Cell]) {
    // od przedostatniego wiersza w górę, tak jak w C++
    for i in (0..WIDTH * HEIGHT - WIDTH).rev() {
        if cells[i].kind != MaterialKind::Air && !cells[i].was_updated {
            if cells[i].kind.can_displace(cells[i + WIDTH].kind) {
                cells.swap(i, i + WIDTH);
            }
            cells[i + WIDTH].was_updated = true;
            cells[i].was_updated = true;
        }
    }
}

fn main() {
    let sdl = sdl3::init().unwrap();
    let video = sdl.video().unwrap();
    let window = video
        .window(TITLE, WIDTH as u32, HEIGHT as u32)
        .resizable()
        .build()
        .unwrap();

    let mut canvas = window.into_canvas();
    canvas
        .set_logical_size(
            WIDTH as u32,
            HEIGHT as u32,
            SDL_RendererLogicalPresentation::LETTERBOX,
        )
        .unwrap();
    canvas.set_draw_color(Color::RGBA(0, 0, 0, 255)); // tło pod przezroczystym AIR

    let texture_creator = canvas.texture_creator();
    let mut texture = texture_creator
        .create_texture_streaming(PixelFormat::RGBA8888, WIDTH as u32, HEIGHT as u32)
        .unwrap();
    texture.set_scale_mode(ScaleMode::Nearest);

    let mut cells = vec![EMPTY; WIDTH * HEIGHT];

    let mut event_pump = sdl.event_pump().unwrap();
    let mut running = true;
    let mut previous = Instant::now();
    let mut accumulator = 0.0_f64;

    while running {
        let now = Instant::now();
        accumulator += now.duration_since(previous).as_secs_f64();
        previous = now;

        for mut event in event_pump.poll_iter() {
            event.convert_coords(&canvas);
            match event {
                Event::Quit { .. } => running = false,
                Event::MouseButtonDown { x, y, .. } => {
                    let (x, y) = (x as i32, y as i32);
                    if x >= 0 && (x as usize) < WIDTH && y >= 0 && (y as usize) < HEIGHT {
                        cells[y as usize * WIDTH + x as usize].kind = MaterialKind::Sand;
                    }
                }
                _ => {}
            }
        }

        while accumulator >= FIXED_DT {
            reset_update_state(&mut cells);
            update_cells(&mut cells);
            accumulator -= FIXED_DT;
        }

        // komórki -> piksele, prosto do tekstury (zastępuje bufor `pixels` i update_pixels)
        texture
            .with_lock(None, |buf: &mut [u8], pitch: usize| {
                for y in 0..HEIGHT {
                    let row = &mut buf[y * pitch..y * pitch + WIDTH * 4];
                    for x in 0..WIDTH {
                        let color = cells[y * WIDTH + x].kind.props().color;
                        row[x * 4..x * 4 + 4].copy_from_slice(&color.to_ne_bytes());
                    }
                }
            })
            .unwrap();

        canvas.clear();
        canvas.copy(&texture, None, None).unwrap();
        canvas.present();
    }
}
