#include "InstrumentDX7.h"
#include "dx7_patches.h"
#include "dx7_algos.h"
#include <vector>
#include <utility>
#include <algorithm>
#include <cmath>

static void textCenteredAtX(U8G2 &u8g2, int centerX, int y, const char *text) {
    int textWidth = u8g2.getStrWidth(text);
    int startX = centerX - (textWidth / 2);
    u8g2.drawStr(startX, y, text);
}

static void draw_algo(U8G2 &u8g2,
                      const std::vector<std::pair<int, int>> &top_connections,
                      const std::vector<std::pair<int, int>> &bottom_connections,
                      const std::vector<int> &carriers,
                      int base_y) {
    char buffer[2];

    for (int op = 0; op < 6; op++) {
        int x_0 = 2 + op * 21;
        int w = 16;
        int h = 16;
        int y_0 = base_y - h;

        u8g2.setDrawColor(1);
        itoa(op+1, buffer, 10);
        u8g2.setFont(u8g2_font_5x7_tr);
        textCenteredAtX(u8g2, x_0 + 8, y_0 + 11, buffer);

        bool is_carrier = (std::find(carriers.begin(), carriers.end(), op) != carriers.end());

        if (is_carrier) {
            u8g2.setDrawColor(2); // XOR mode
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
        int y_hline = base_y - 16 - 4;

        u8g2.drawVLine(x_a + 8, y_hline, 4);

        if (a != b) {
            int x_b = 2 + b * 21;
            u8g2.drawVLine(x_b + 8, y_hline, 4);

            int start_x = std::min(x_a, x_b);
            int width = std::abs(x_b - x_a) + 1;
            u8g2.drawHLine(start_x + 8, y_hline, width);
        } else {
            u8g2.drawHLine(x_a + 17, base_y - 8, 2);
            u8g2.drawVLine(x_a + 18, y_hline, 10);
            u8g2.drawHLine(x_a + 8, y_hline, 11);
        }
    }

    for (const auto &conn : bottom_connections) {
        int a = conn.first;
        int b = conn.second;

        int x_a = 2 + a * 21;
        int y_hline = base_y + 4;

        u8g2.drawVLine(x_a + 8, y_hline - 4, 4);

        if (a != b) {
            int x_b = 2 + b * 21;
            u8g2.drawVLine(x_b + 8, y_hline - 4, 4);

            int start_x = std::min(x_a, x_b);
            int width = std::abs(x_b - x_a) + 1;
            u8g2.drawHLine(start_x + 8, y_hline, width);
        } else {
            u8g2.drawHLine(x_a + 17, base_y - 8, 2);
            u8g2.drawVLine(x_a + 18, y_hline - 10, 10);
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
}

void InstrumentDX7::start() {
    isActive = true;

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.num_voices = 8;
    e.patch_number = patch + 128;
    amy_add_event(&e);

    sendAllParams();
    needsUIRedraw = true;
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

    sendAllParams();
    needsUIRedraw = true;
}

const char *InstrumentDX7::getPatchName(int idx) const {
    if (idx >= 0 && idx < 128) return dx7_patches[idx].name;
    return "";
}

void InstrumentDX7::drawUI(U8G2 &u8g2) {
    uint8_t algoNum = pgm_read_byte(&dx7_patches[patch].algo);
    if (algoNum < 1 || algoNum > 32) algoNum = 1;
    const DX7Algo &currentAlgo = dx7_algorithms[algoNum - 1];

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(1);
    char algoBuf[16];
    snprintf(algoBuf, sizeof(algoBuf), "ALGO %d", algoNum);
    u8g2.drawStr(4, 38, algoBuf);

    int base_y = 70;
    draw_algo(u8g2, currentAlgo.top, currentAlgo.bottom, currentAlgo.carriers, base_y);
}

void InstrumentDX7::onParamChanged(uint8_t paramIndex) {
    sendAdsr();
    sendFilter();
    configReverb();
    configDelay();
    needsUIRedraw = true;
}

void InstrumentDX7::sendAdsr() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();

    uint16_t a_ms = (uint16_t)fmax(params.attack_ms, 1.0f);
    uint16_t d_ms = (uint16_t)fmax(params.decay_ms, 1.0f);
    uint16_t r_ms = (uint16_t)fmax(params.release_ms, 1.0f);
    float s_val   = constrain(params.sustain_pct / 100.0f, 0.0f, 1.0f);
    
    e.eg1_times[0] = a_ms;
    e.eg1_values[0] = 1.0f;

    e.eg1_times[1] = d_ms;
    e.eg1_values[1] = s_val;

    e.eg1_times[2] = r_ms;
    e.eg1_values[2] = 0.0f;

    amy_add_event(&e);
}
