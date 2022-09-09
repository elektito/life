#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#undef RAYGUI_IMPLEMENTATION

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>

EM_JS(void, offer_download, (const char *filename, const char *mime), {
  mime = mime || "application/octet-stream";

  filename = UTF8ToString(filename);
  mime = UTF8ToString(mime);

  let content = Module.FS.readFile(filename);
  console.log(`Offering download of "${filename}", with ${content.length} bytes...`);

  var a = document.createElement('a');
  a.download = filename;
  a.href = URL.createObjectURL(new Blob([content], {type: mime}));
  a.style.display = 'none';

  document.body.appendChild(a);
  a.click();
  setTimeout(() => {
    document.body.removeChild(a);
    URL.revokeObjectURL(a.href);
  }, 2000);
});
#else /* not web */
#define EMSCRIPTEN_KEEPALIVE
#endif

static bool gui_visible = false;
static bool should_close = false;
static bool crowd_rules_enabled = false;

enum ParticleType
{
        P_RED = 1,
        P_GREEN = 2,
        P_BLUE = 3,
        P_YELLOW = 4,
};

struct Particle {
        enum ParticleType type;
        float x;
        float y;
        float vx;
        float vy;
        float fx;
        float fy;
};

#define NPARTICLES 1750
static struct Particle particles[NPARTICLES];

struct Rule {
        enum ParticleType type1;
        enum ParticleType type2;
        float close_factor;
        float distant_factor;
        bool updated;
};

struct {
        enum ParticleType type;
        const char *name;
} colors[] = {
        {P_RED, "red"},
        {P_GREEN, "green"},
        {P_BLUE, "blue"},
        {P_YELLOW, "yellow"},
};

#define NCOLORS (sizeof(colors) / sizeof(colors[0]))
#define NRULES (NCOLORS * (NCOLORS + 1) / 2)
static struct Rule rules[NRULES];

void
draw_screen(void)
{
        ClearBackground(BLACK);
        for (int i = 0; i < NPARTICLES; i++) {
                Color color;
                switch (particles[i].type) {
                case P_RED:
                        color = RED;
                        break;
                case P_GREEN:
                        color = GREEN;
                        break;
                case P_BLUE:
                        color = BLUE;
                        break;
                case P_YELLOW:
                        color = YELLOW;
                        break;
                default:
                        color = WHITE;
                }
                DrawCircle(particles[i].x, particles[i].y, 3, color);
        }

        DrawFPS(10, 10);
}

float
rnd(void)
{
        return ((float) rand()) / ((float) RAND_MAX);
}

float
get_interaction(int i, int j, float dist)
{
        if (dist == 0)
                dist = 0.00001;

        int t1 = particles[i].type;
        int t2 = particles[j].type;

        float force = 0.0;
        for (int k = 0; k < NRULES; ++k) {
                if ((t1 == rules[k].type1 && t2 == rules[k].type2) ||
                    (t2 == rules[k].type1 && t1 == rules[k].type2))
                {
                        /*if (dist < 15) {
                                force += rules[k].close_factor / dist;
                        } else */ if (dist < 2800) {
                                force += rules[k].distant_factor / dist * 2;
                        }
                        break;
                }
        }

        return force;
}

void
step(void)
{
        int scrw = GetScreenWidth();
        int scrh = GetScreenHeight();
        int same_color_close = 0;
        int close = 0;

        for (int i = 0; i < NPARTICLES; i++) {
                particles[i].fx = 0;
                particles[i].fy = 0;
        }

        for (int i = 0; i < NPARTICLES; i++) {
                same_color_close = 0;
                close = 0;
                for (int j = 0; j < NPARTICLES; j++) {
                        if (i == j)
                                continue;
                        float dx = particles[i].x - particles[j].x;
                        float dy = particles[i].y - particles[j].y;
                        float dist = sqrt(dx * dx + dy * dy);
                        if (dist < 300) {
                                float f = get_interaction(i, j, dist);
                                particles[i].fx += f * dx;
                                particles[i].fy += f * dy;
                        }

                        if (particles[i].type == particles[j].type &&
                            dist < 50)
                        {
                                same_color_close++;
                        }

                        if (dist < 50) {
                                close++;
                        }
                }

                // I tried doing this after the current loop, that is,
                // apply all of the of forces at the same time, but that
                // would quickly result in (mostly) unmoving stable
                // structures which were not very interesting.
                particles[i].vx += particles[i].fx * 0.08;
                particles[i].vy += particles[i].fy * 0.08;

                float speed =
                        particles[i].vx * particles[i].vx +
                        particles[i].vy * particles[i].vy;
                const float max_speed = 128;
                if (speed > max_speed)
                {
                        particles[i].vx /= speed;
                        particles[i].vy /= speed;
                        particles[i].vx *= max_speed;
                        particles[i].vy *= max_speed;
                }

                particles[i].x += particles[i].vx;
                particles[i].y += particles[i].vy;


                if (particles[i].x < 0) {
                        particles[i].vx *= -1;
                        particles[i].x = 0;
                }
                if (particles[i].x > scrw - 1) {
                        particles[i].vx *= -1;
                        particles[i].x = scrw - 1;
                }
                if (particles[i].y < 0) {
                        particles[i].vy *= -1;
                        particles[i].y = 0;
                }
                if (particles[i].y > scrh - 1) {
                        particles[i].vy *= -1;
                        particles[i].y = scrh - 1;
                }

                if (crowd_rules_enabled) {
                        if (same_color_close > 100 && rnd() < 0.1)
                                particles[i].type =
                                        (int)(rnd() * 4) + 1;

                        if (close < 5 && rnd() < 0.001)
                                particles[i].type =
                                        (int)(rnd() * 4) + 1;

                        if (close > 100 && rnd() < 20.0 / close) {
                                particles[i].vx = 10 * rnd();
                                particles[i].vy = 10 * rnd();
                        }
                }
        }
}

EMSCRIPTEN_KEEPALIVE int
load_file(uint8_t *buffer, size_t size) {
        // In the web version, this function is called from javascript
        // when the file upload is activated

        int n;
        for (int i = 0; i < NRULES; ++i) {
                sscanf((const char *) buffer, "%f%n",
                       &rules[i].distant_factor,
                       &n);
                if (n == EOF) {
                        printf("Premature end of save file.\n");
                        break;
                }

                buffer += n;
        }

        return 1;
}

void
init_particles(void)
{
        int scrw = GetScreenWidth();
        int scrh = GetScreenHeight();
        for (int i = 0; i < NPARTICLES; i++) {
                particles[i] = (struct Particle) {
                        .type = (int)(rnd() * 4) + 1,
                        .x = rnd() * scrw,
                        .y = rnd() * scrh,
                        .vx = 0.0,
                        .vy = 0.0,
                };
        }
}

void
randomize_rules(void)
{
        for (int i = 0; i < NRULES; ++i) {
                rules[i].distant_factor = (rnd() * 2.0) - 1.0;
        }
}

void
draw_gui()
{
        if (!gui_visible)
                return;

        char text_left[32];
        char text_right[32];
        float min_value = -1.0;
        float max_value = 1.0;
        int n = 0;
        float x = 100.0;
        float y = 100.0;
        for (int i = 0; i < 4; ++i) {
                for (int j = i; j < 4; ++j) {
                        rules[n].updated = false;
                        snprintf(text_left, sizeof(text_left),
                                 "%s/%s:",
                                 colors[i].name, colors[j].name);
                        snprintf(text_right, sizeof(text_right),
                                 "%.1f", rules[n].distant_factor);
                        Rectangle rect = {x, y, 200, 20};
                        float new_value = GuiSlider(
                                rect,
                                text_left, text_right,
                                rules[n].distant_factor,
                                min_value, max_value
                        );
                        if (rules[n].distant_factor != new_value) {
                                rules[n].distant_factor = new_value;
                                rules[n].updated = true;
                        }

                        ++n;
                        y += 30.0;
                }
        }

        crowd_rules_enabled = GuiToggle((Rectangle){x, y, 120, 20},
                                        "Crowd Rules",
                                        crowd_rules_enabled);
        y += 25;

        if (GuiButton((Rectangle){x, y, 120, 20},
                      "Randomize Rules"))
        {
                randomize_rules();
        }

        y += 25;

        if (GuiButton((Rectangle){x, y, 120, 20},
                      "Re-Init Particles"))
        {
                init_particles();
        }

        y+= 25;

        if (GuiButton((Rectangle){x, y, 50, 20}, "Save")) {
                FILE *fp = fopen("save.life", "w");
                for (int i = 0; i < NRULES; ++i) {
                        fprintf(fp, "%f\n", rules[i].distant_factor);
                }
                fclose(fp);

                #ifdef PLATFORM_WEB
                offer_download("save.life", "application/octet-stream");
                #endif
        }
        if (GuiButton((Rectangle){x + 70, y, 50, 20}, "Load")) {
                #ifdef PLATFORM_WEB
                EM_ASM(
                        var file_selector = document.createElement(
                                'input');
                        file_selector.setAttribute('type', 'file');
                        file_selector.setAttribute(
                                'onchange',
                                'open_file(event)');
                        file_selector.setAttribute(
                                'accept',
                                '.life');
                        file_selector.click();
                );
                #else
                const char *filename = "save.life";
                FILE *fp = fopen(filename, "r");
                if (fp) {
                        fseek(fp, 0, SEEK_END);
                        long length = ftell(fp);
                        fseek(fp, 0, SEEK_SET);
                        char *buffer = malloc(length);
                        int n = 0;
                        char ch;
                        for (;;) {
                                ch = fgetc(fp);
                                if (ch == EOF)
                                        break;
                                buffer[n++] = ch;
                        };
                        fclose(fp);

                        load_file((uint8_t *) buffer, length);
                        free(buffer);
                } else {
                        printf("Cannot open file: %s\n", filename);
                }
                #endif
        }
}

void
draw_frame(void)
{
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
            IsKeyPressed(KEY_R))
        {
                randomize_rules();
        }
        if (IsKeyPressed(KEY_V)) {
                gui_visible = !gui_visible;
        }
        if (IsKeyPressed(KEY_Q)) {
                should_close = true;
                return;
        }

        BeginDrawing();
        draw_screen();
        draw_gui();
        EndDrawing();

        step();
}

int
main(int argc, char *argv[])
{
        int scrw = 800;
        int scrh = 600;
        InitWindow(scrw, scrh, "Life");
        SetWindowState(FLAG_WINDOW_RESIZABLE);

        srand(time(NULL));

        init_particles();

        /* initialize rules to zero */
        int n = 0;
        for (int i = 0; i < 4; ++i) {
                for (int j = i; j < 4; ++j) {
                        rules[n].updated = false;
                        rules[n].type1 = colors[i].type;
                        rules[n].type2 = colors[j].type;
                        ++n;
                }
        }

        GuiFade(0.9);

        #ifdef PLATFORM_WEB
        emscripten_set_main_loop(draw_frame, 0, 1);
        #else
        SetTargetFPS(60);
        while (!WindowShouldClose() && !should_close) {
                draw_frame();
        }
        #endif

        CloseWindow();

        return 0;
}
