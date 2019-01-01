#include <cmath>
#include <iostream>
#include <thread>
#include <sstream>

#include <SDL.h>

#include <boost/core/ignore_unused.hpp>

extern "C" {
    #include "emulator.h"
}

struct cycle_budget_t {
    double cycles_per_ns_;
    unsigned int current_;
    double partial_;

    explicit cycle_budget_t(double cycle_per_ns);
    unsigned int update(double delta_time_ns);
};


struct sdl_context_t {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    sdl_context_t(const char *window_name, 
        unsigned int width, unsigned int height);
    ~sdl_context_t();

    static std::string get_error(const char* str);
};

bool sdl_handle_events();
void sdl_draw(sdl_context_t *s, cpu_state_t* state);


const long long frames_per_sec = 120;
const long long ns_per_sec = 1000000000;
const std::chrono::nanoseconds skip_ns{ns_per_sec / frames_per_sec};

struct frame_state_t {
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> current_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> next_frame_time;
    std::chrono::nanoseconds delta_time;
    unsigned long long frame_count;

    frame_state_t();
    void update();
    void wait_next_frame();
};


void print_debug_info(unsigned long long cycle_cnt, frame_state_t* frame_state);

void read_binary(const char* filename, uint8_t* buffer);

void read_binary(const char* filename, uint8_t* buffer) {
    FILE* f = fopen(filename, "rb");
    if (f == nullptr) {
        printf("Error: Couldn't open file\n");
        exit(EXIT_FAILURE);
    }

    fseek(f, 0L, SEEK_END);
    long size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    fread(buffer, static_cast<size_t >(size), 1, f);
    fclose(f);
}


uint8_t in_port1 = 0;
uint8_t shift0 = 0;         // LSB of Space Invader's external shift hardware
uint8_t shift1 = 0;         // MSB
uint8_t shift_offset = 0;   // offset for external shift hardware

// Output ports for sounds
uint8_t out_port3, out_port5, last_out_port3, last_out_port5;

static uint8_t input(uint8_t port) {
    switch (port) {
        case 0:
            return 0xf;
        case 1:
            return in_port1;
        case 2:
            return 0;
        case 3: {
            uint16_t v = (shift1 << 8) | shift0;
            return static_cast<uint8_t >((v >> (8 - shift_offset)) & 0xff);
        }
        default:
            return 0;
    }
}

static void output(uint8_t port, uint8_t value) {
    switch (port) {
        case 2:
            shift_offset = static_cast<uint8_t>(value & 0x7);
            break;
        case 3:
            out_port3 = value;
            break;
        case 4:
            shift0 = shift1;
            shift1 = value;
            break;
        case 5:
            out_port5 = value;
            break;
        default:
            break;
    }
}

static uint32_t* pixels = nullptr;

int main(int argc, char *argv[]) {
    boost::ignore_unused(argc);
    boost::ignore_unused(argv);

    try {
        pixels = new uint32_t[224 * 256];

        auto state = new cpu_state_t;
        memset(state, 0, sizeof(cpu_state_t));
        state->memory = new uint8_t[16 * 0x1000]; //(uint8_t*)malloc(16 * 0x1000);
        memset(state->memory, 0, 16 * 0x1000);
        state->input = input;
        state->output = output;

        read_binary("invaders.h", state->memory);
        read_binary("invaders.g", state->memory + 0x800);
        read_binary("invaders.f", state->memory + 0x1000);
        read_binary("invaders.e", state->memory + 0x1800);

        int interruptNum = 1;

        unsigned long long cycle_cnt = 0;

        sdl_context_t sdl_ctx{"Invaders 8080", 224, 256};
        cycle_budget_t cycle_budget{0.002};
        frame_state_t frame_state{};

        while (true) {
            if (sdl_handle_events()) {
                break;
            }

            frame_state.update();

            cycle_cnt += cycle_budget.update(frame_state.delta_time.count());
            while (cycle_budget.current_ >= 18) {
                cycle_budget.current_ -= emulate(state);
            }

            if (state->int_enable) {
                interrupt(state, interruptNum);
                interruptNum = interruptNum == 1 ? 2 : 1;
            }

            if (frame_state.frame_count % frames_per_sec == 0) {
                print_debug_info(cycle_cnt, &frame_state);
            }

            sdl_draw(&sdl_ctx, state);

            frame_state.wait_next_frame();
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
}

cycle_budget_t::cycle_budget_t(double cycle_per_ns)
: cycles_per_ns_{cycle_per_ns}
, current_{0}
, partial_{0.0} {
}

unsigned int cycle_budget_t::update(double delta_time_ns) {
    unsigned int new_budget = 0;
    const double delta_cycles = delta_time_ns * cycles_per_ns_;
    new_budget += static_cast<unsigned int>(std::floor(delta_cycles));
    partial_ += delta_cycles - new_budget;
    if (partial_ > 1.0) {
        new_budget += static_cast<unsigned int>(std::floor(partial_));
        partial_ -= std::floor(partial_);
    }
    current_ += new_budget;
    return new_budget;
}

sdl_context_t::sdl_context_t(const char *window_name, unsigned int width, unsigned int height)
: window{nullptr}
, renderer{nullptr}
, texture{nullptr} {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error("Failed to initialise SDL");
    }
    window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, static_cast<int>(width), static_cast<int>(height), 0);
    if (window == nullptr) {
        throw std::runtime_error(get_error("Failed to create SDL window: "));
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        SDL_DestroyWindow(window);
        throw std::runtime_error(get_error("Failed to create SDL renderer: "));
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, static_cast<int>(width), static_cast<int>(height));
    if (texture == nullptr) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        throw std::runtime_error(get_error("Failed to create SDL texture: "));
    }
}

sdl_context_t::~sdl_context_t() {
    if (renderer != nullptr) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    SDL_Quit();
}

std::string sdl_context_t::get_error(const char *str) {
    std::stringstream ss;
    ss << str << SDL_GetError();
    return ss.str();
}

bool sdl_handle_events() {
    SDL_Event event;
    if (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return true;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_c:
                        in_port1 |= 0x01;
                        break;
                    case SDLK_LEFT:
                        in_port1 |= 0x20;
                        break;
                    case SDLK_RIGHT:
                        in_port1 |= 0x40;
                        break;
                    case SDLK_SPACE:
                        in_port1 |= 0x10;
                        break;
                    case SDLK_1:
                        in_port1 |= 0x04;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_c:
                        in_port1 &= ~0x01;
                        break;
                    case SDLK_LEFT:
                        in_port1 &= ~0x20;
                        break;
                    case SDLK_RIGHT:
                        in_port1 &= ~0x40;
                        break;
                    case SDLK_SPACE:
                        in_port1 &= ~0x10;
                        break;
                    case SDLK_1:
                        in_port1 &= ~0x04;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    return false;
}

void sdl_draw(sdl_context_t *s, cpu_state_t* state) {
    SDL_SetRenderDrawColor(s->renderer, 0, 0, 0, 255);

    //Translate the 1-bit space invaders frame buffer into
    // my 32bpp RGB bitmap.  We have to rotate and
    // flip the image as we go.
    //
    uint32_t* b = pixels;
    uint8_t* fb = &state->memory[0x2400];

    for (int i = 0; i < 224; i++) {
        for (int j = 0; j < 256; j += 8) {
            //Read the first 1-bit pixel
            // divide by 8 because there are 8 pixels
            // in a byte
            uint8_t pix = fb[(i * (256 / 8)) + j / 8];

            //That makes 8 output vertical pixels
            // we need to do a vertical flip
            // so j needs to start at the last line
            // and advance backward through the buffer
            int offset = (255 - j) * (224) + (i);
            uint32_t* p1 = &b[offset];
            for (int p = 0; p < 8; p++) {
                if (0 != (pix & (1 << p))) {
                    *p1 = 0xFFFFFFFFL;
                } else {
                    *p1 = 0x000000000;
                }
                p1 -= 224;  //next line
            }
        }
    }

    SDL_UpdateTexture(s->texture, nullptr, pixels, 224 * sizeof(uint32_t));
    SDL_RenderClear(s->renderer);
    SDL_RenderCopy(s->renderer, s->texture, nullptr, nullptr);
    SDL_RenderPresent(s->renderer);
}

frame_state_t::frame_state_t()
: start_time{std::chrono::high_resolution_clock::now()}
, current_time{start_time}
, last_time{start_time}
, next_frame_time{start_time}
, delta_time{std::chrono::nanoseconds{1}}
, frame_count{0} {
}

void frame_state_t::update() {
    frame_count++;
    current_time = std::chrono::high_resolution_clock::now();
    delta_time = current_time - last_time;
    last_time = current_time;
}

void frame_state_t::wait_next_frame() {
    next_frame_time += skip_ns;
    auto sleep_time = next_frame_time - std::chrono::high_resolution_clock::now();
    if (sleep_time.count() >= 0) {
        std::this_thread::sleep_for(sleep_time);
    }
}

void print_debug_info(unsigned long long cycle_cnt, frame_state_t* frame_state) {
    std::cout << cycle_cnt / std::chrono::duration<double, std::milli>(frame_state->current_time - frame_state->start_time).count() << " cycles/ms, "
              << cycle_cnt / frame_state->frame_count << " cycles/frame, "
              << frame_state->frame_count / std::chrono::duration<double, std::ratio<1>>(frame_state->current_time - frame_state->start_time).count() << " frames/second" << std::endl;
}
