#include "InstrumentDX7.h"
#include "dx7_patches.h"
#include "dx7_algos.h"
#include <vector>
#include <utility>
#include <algorithm>
#include <cmath>

void textCenteredAtX(U8G2 &u8g2, int centerX, int y, const char *text) {
    int textWidth = u8g2.getStrWidth(text);
    int startX = centerX - (textWidth / 2);
    u8g2.drawStr(startX, y, text);
}

void draw_algo(U8G2 &u8g2,
               const std::vector<std::pair<int, int>> &top_connections,
               const std::vector<std::pair<int, int>> &bottom_connections,
               const std::vector<int> &carriers,
               int bottom_offset, int base_y) {
    char buffer[2];

    for (int op = 0; op < 6; op++) {
        int x_0 = 2 + op * 21;
        int w = 16;
        int h = 16;
        int y_0 = base_y - bottom_offset - h;

        u8g2.setDrawColor(1);
        itoa(op+1, buffer, 10);
        textCenteredAtX(u8g2, x_0+8, y_0 + 13, buffer);

        bool is_carrier = (std::find(carriers.begin(), carriers.end(), op) != carriers.end());

        if (is_carrier) {
            u8g2.setDrawColor(2); 
            u8g2.drawBox(x_0, y_0, w, h);
        } else {
            u8g2.drawFrame(x_0, y_0, w, h);
        }
    }

    u8g2.setDrawColor(1);

    for (const auto &conn : top_connections) {
        int a = conn.first;
        int b = conn.second;

        int x_a = 2 + a * 21;
        int y_hline = base_y - bottom_offset - 16 - 4;

        u8g2.drawVLine(x_a + 8, y_hline, 3);

        if (a != b) {
            int x_b = 2 + b * 21;
            u8g2.drawVLine(x_b + 8, y_hline, 3);

            int start_x = std::min(x_a, x_b);
            int width = std::abs(x_b - x_a) + 1;
            u8g2.drawHLine(start_x + 8, y_hline, width);
        } else {
            u8g2.drawHLine(x_a + 17, base_y - bottom_offset - 8, 2);
            u8g2.drawVLine(x_a + 18, y_hline, 12);
            u8g2.drawHLine(x_a + 8, y_hline, 11);
        }
    }

    for (const auto &conn : bottom_connections) {
        int a = conn.first;
        int b = conn.second;

        int x_a = 2 + a * 21;
        int y_hline = base_y - bottom_offset + 3;

        u8g2.drawVLine(x_a + 8, y_hline - 2, 3);

        if (a != b) {
            int x_b = 2 + b * 21;
            u8g2.drawVLine(x_b + 8, y_hline - 2, 3);

            int start_x = std::min(x_a, x_b);
            int width = std::abs(x_b - x_a) + 1;
            u8g2.drawHLine(start_x + 8, y_hline, width);
        } else {
            u8g2.drawHLine(x_a + 17, base_y - bottom_offset - 8, 2);
            u8g2.drawVLine(x_a + 18, y_hline - 12, 12);
            u8g2.drawHLine(x_a + 8, y_hline, 11);
        }
    }
}

void InstrumentDX7::init() {
    buildBaseParams();
    _dx7ParamCount = _baseParamCount;
    for (int i = 0; i < _baseParamCount; i++) {
        _dx7Params[i] = _baseParams[i];
    }
    // Debug: Serial.println("  [DX-7] Initialized (Patches 128-255)");
}

void InstrumentDX7::start() {
    isActive = true;

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.num_voices = 8;
    e.patch_number = patch + 128;
    amy_add_event(&e);

    needsUIRedraw = true;
    // Debug: Serial.printf("  [DX-7] Ready (patch %d)\n", patch);
}

void InstrumentDX7::stop() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.velocity = 0.0f;
    amy_add_event(&e);
    isActive = false;
}

void InstrumentDX7::setPatch(int index) {
    if (index < 0) index = 127;
    if (index > 127) index = 0;
    patch = index;
    
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.patch_number = patch + 128;
    amy_add_event(&e);
    // Debug: Serial.printf("  [DX-7] Patch %d\n", patch);
    needsUIRedraw = true;
}

const char *InstrumentDX7::getPatchName(int idx) const {
    if (idx >= 0 && idx < 128) return dx7_patches[idx].name;
    return "";
}

void InstrumentDX7::drawUI(U8G2 &u8g2) {
    u8g2.setFont(u8g2_font_spleen12x24_mu);
    u8g2.setCursor(0, INSTRUMENT_UI_Y + 18);
    u8g2.printf("%s", dx7_patches[patch].name);

    uint8_t algoNum = pgm_read_byte(&dx7_patches[patch].algo);
    const DX7Algo &currentAlgo = dx7_algorithms[algoNum-1];

    u8g2.setFont(u8g2_font_tenfatguys_tf);
    int base_y = INSTRUMENT_UI_Y + INSTRUMENT_UI_H;
    draw_algo(u8g2, currentAlgo.top, currentAlgo.bottom, currentAlgo.carriers, 9, base_y);
}

void InstrumentDX7::sendAdsr() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();

    uint16_t a_ms = (uint16_t)fmax(params.attack * 1000.0f, 1.0f);
    uint16_t d_ms = (uint16_t)fmax(params.decay * 1000.0f, 1.0f);
    uint16_t r_ms = (uint16_t)fmax(params.release * 1000.0f, 1.0f);
    
    e.eg1_times[0] = a_ms;
    e.eg1_values[0] = 1.0f;

    e.eg1_times[1] = d_ms;
    e.eg1_values[1] = params.sustain;

    e.eg1_times[2] = r_ms;
    e.eg1_values[2] = 0.0f;

    amy_add_event(&e);
}
