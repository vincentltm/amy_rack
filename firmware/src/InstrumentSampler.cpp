#include "InstrumentSampler.h"
#include "amy.h"
#include <cmath>
#include <algorithm>

static const char *pcmSampleNames[12] = {
    "808 Bass Drum",
    "808 Snare Drum",
    "808 Closed Hat",
    "808 Open Hat",
    "808 Clap",
    "808 Low Tom",
    "808 Mid Tom",
    "808 High Tom",
    "808 Cowbell",
    "808 Maraca",
    "808 Clave",
    "Live Recorder"
};

InstrumentSampler::InstrumentSampler() {
    _currentPatch = 0;
    _record_buffer = (int16_t *)malloc(SAMPLER_MAX_SAMPLES * sizeof(int16_t));
    if (_record_buffer) {
        memset(_record_buffer, 0, SAMPLER_MAX_SAMPLES * sizeof(int16_t));
    }
}

InstrumentSampler::~InstrumentSampler() {
    if (_record_buffer) {
        free(_record_buffer);
        _record_buffer = nullptr;
    }
}

void InstrumentSampler::init() {
    _amy_preset_num = 1;
    buildBaseParams();

    // Tab: SYNTH
    _samplerParams[0] = PARAM_INT("Record", "", 0, 1,           &_param_record,     TAB_SYNTH);
    _samplerParams[1] = PARAM_PCT("In Volume",  0.0f, 100.0f, 1.0f, &_param_in_vol,    TAB_SYNTH);
    _samplerParams[2] = PARAM_PCT("Trim Start", 0.0f, 90.0f, 2.0f, &_param_trim_start, TAB_SYNTH);
    _samplerParams[3] = PARAM_PCT("Trim End",   10.0f, 100.0f, 2.0f, &_param_trim_end, TAB_SYNTH);
    _samplerParams[4] = PARAM_FLOAT("Gain", "x", 0.1f, 7.0f, 0.1f, &_param_gain,     TAB_SYNTH);

    _samplerParamCount = 5;
    for (int i = 0; i < _baseParamCount; i++) {
        _samplerParams[_samplerParamCount++] = _baseParams[i];
    }
}

void InstrumentSampler::start() {
    isActive = true;
    setupSynthVoices();
    needsUIRedraw = true;
}

void InstrumentSampler::stop() {
    if (isRecording) {
        stopRecording();
    }

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.velocity = 0.0f;
    amy_add_event(&e);

    isActive = false;
}

void InstrumentSampler::setPatch(int index) {
    if (index < 0) index = 11;
    if (index > 11) index = 0;
    _currentPatch = index;

    if (_currentPatch < 11) {
        _amy_preset_num = _currentPatch + 1; // 1 to 11 are 808 ROM samples
        setupSynthVoices();
    } else {
        _amy_preset_num = 11; // User recorded RAM slot
        if (_record_buffer && original_length > 0) {
            reloadTrimmedSample();
        } else {
            setupSynthVoices();
        }
    }
    needsUIRedraw = true;
}

const char *InstrumentSampler::getPatchName(int idx) const {
    if (idx >= 0 && idx < 12) {
        return pcmSampleNames[idx];
    }
    return "Unknown";
}

void InstrumentSampler::noteOn(uint8_t note, float velocity) {
    if (!isActive) return;

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.midi_note = note;
    e.velocity = velocity;
    amy_add_event(&e);
}

void InstrumentSampler::noteOff(uint8_t note) {
    if (!isActive) return;

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.midi_note = note;
    e.velocity = 0.0f;
    amy_add_event(&e);
}

void InstrumentSampler::startRecording() {
    if (isRecording || !_record_buffer) return;

    isRecording = true;
    recordingFinished = false;
    sample_index = 0;
    _param_record = 1.0f;

    xTaskCreatePinnedToCore(
        recordingTaskWrapper,
        "SamplerRec",
        4096,
        this,
        1,
        &_recordingTaskHandle,
        0
    );

    needsUIRedraw = true;
}

void InstrumentSampler::stopRecording() {
    if (!isRecording) return;
    isRecording = false;
}

void InstrumentSampler::update() {
    if (recordingFinished) {
        recordingFinished = false;
        finishRecording();
    }
}

void InstrumentSampler::recordingTaskWrapper(void *arg) {
    InstrumentSampler *self = (InstrumentSampler *)arg;
    
    // Simulate live audio capture stream into buffer
    float phase = 0.0f;
    float freq = 220.0f * 6.28318530718f / (float)SAMPLER_SAMPLE_RATE;

    while (self->isRecording && self->sample_index < SAMPLER_MAX_SAMPLES) {
        int chunk_size = 128;
        if (self->sample_index + chunk_size > SAMPLER_MAX_SAMPLES) {
            chunk_size = SAMPLER_MAX_SAMPLES - self->sample_index;
        }

        for (int i = 0; i < chunk_size; i++) {
            float env = 1.0f - ((float)(self->sample_index + i) / (float)SAMPLER_MAX_SAMPLES);
            float s = sinf(phase) * env * 24000.0f;
            phase += freq;
            if (phase > 6.28318530718f) phase -= 6.28318530718f;

            self->_record_buffer[self->sample_index + i] = (int16_t)s;
        }

        self->sample_index += chunk_size;
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    self->sample_length = self->sample_index;
    self->recordingFinished = true;
    self->_recordingTaskHandle = nullptr;
    self->isRecording = false;
    self->_param_record = 0.0f;
    vTaskDelete(nullptr);
}

void InstrumentSampler::finishRecording() {
    if (sample_length == 0 || !_record_buffer) return;

    original_length = sample_length;
    _trim_start_samples = (uint32_t)((_param_trim_start / 100.0f) * (float)original_length);
    _trim_end_samples = (uint32_t)((_param_trim_end / 100.0f) * (float)original_length);
    if (_trim_end_samples <= _trim_start_samples) _trim_end_samples = original_length;

    reloadTrimmedSample();
    needsUIRedraw = true;
}

void InstrumentSampler::reloadTrimmedSample() {
    if (!_record_buffer || original_length == 0) return;

    _trim_start_samples = (uint32_t)((_param_trim_start / 100.0f) * 0.9f * (float)original_length);
    _trim_end_samples = (uint32_t)((0.1f + (_param_trim_end / 100.0f) * 0.9f) * (float)original_length);

    if (_trim_end_samples <= _trim_start_samples + 220) {
        _trim_end_samples = std::min(_trim_start_samples + 220, original_length);
    }
    if (_trim_end_samples > original_length) _trim_end_samples = original_length;

    uint32_t trimmed_len = _trim_end_samples - _trim_start_samples;

    _amy_preset_num = 11; // User preset slot

    int16_t *amy_buf = pcm_load(
        _amy_preset_num,
        trimmed_len,
        SAMPLER_SAMPLE_RATE,
        60, // Middle C
        0,  // Loop start
        0,  // Loop end
        0   // 0 = one-shot
    );

    if (amy_buf) {
        for (uint32_t i = 0; i < trimmed_len; i++) {
            float val = (float)_record_buffer[_trim_start_samples + i] * _param_gain;
            if (val > 32767.0f) val = 32767.0f;
            if (val < -32768.0f) val = -32768.0f;
            amy_buf[i] = (int16_t)val;
        }
    }

    setupSynthVoices();
}

void InstrumentSampler::setupSynthVoices() {
    uint16_t presetNum = (_currentPatch == 11) ? 11 : (_currentPatch + 1);

    amy_event e = amy_default_event();
    e.reset_osc = RESET_PATCH;
    e.patch_number = 1024;
    amy_add_event(&e);

    // 1. Template osc 0
    e = amy_default_event();
    e.osc = 0;
    e.patch_number = 1024;
    e.wave = PCM;
    e.preset = presetNum;
    e.amp_coefs[COEF_CONST] = 1.0f;
    e.amp_coefs[COEF_VEL] = 1.0f;
    amy_add_event(&e);

    // 2. Instantiate on synth channel 1
    e = amy_default_event();
    e.synth = getSynthChannel();
    e.patch_number = 1024;
    e.num_voices = 6;
    e.volume = 3.0f;
    amy_add_event(&e);

    sendAllParams();
}

void InstrumentSampler::drawUI(U8G2 &u8g2) {
    const int BOX_X = 4;
    const int BOX_Y = 16;
    const int BOX_W = 120;
    const int BOX_H = 44;
    const int MID_Y = BOX_Y + BOX_H / 2; // y = 38

    u8g2.setDrawColor(1);

    if (isRecording) {
        u8g2.drawRFrame(BOX_X, BOX_Y, BOX_W, BOX_H, 3);
        u8g2.setFont(u8g2_font_7x14B_tr);
        u8g2.drawStr(12, 32, "* RECORDING... *");

        int progW = (int)(((float)sample_index / SAMPLER_MAX_SAMPLES) * (BOX_W - 12));
        u8g2.drawFrame(BOX_X + 6, BOX_Y + 24, BOX_W - 12, 8);
        if (progW > 0) {
            u8g2.drawBox(BOX_X + 6, BOX_Y + 24, progW, 8);
        }
    } else if (_currentPatch < 11) {
        // Built-in ROM Sample Stylized Preview
        u8g2.drawRFrame(BOX_X, BOX_Y, BOX_W, BOX_H, 3);
        u8g2.drawHLine(BOX_X + 2, MID_Y, BOX_W - 4);

        int dots = 40;
        int step = (BOX_W - 8) / dots;
        for (int i = 0; i < dots; i++) {
            float t = (float)i / (float)dots;
            float decay = expf(-t * 4.0f);
            float freqMult = 1.0f + (11 - _currentPatch) * 0.4f;
            float s = sinf(t * 30.0f * freqMult) * decay;
            int bar_h = (int)(fabsf(s) * (BOX_H / 2 - 4));
            if (bar_h > 0) {
                int px = BOX_X + 4 + i * step;
                u8g2.drawVLine(px, MID_Y - bar_h, bar_h * 2 + 1);
            }
        }

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(BOX_X + 4, BOX_Y + 9, pcmSampleNames[_currentPatch]);
        u8g2.drawStr(BOX_X + BOX_W - 36, BOX_Y + 9, "ROM PCM");
    } else {
        // User Live Recorded Audio Waveform Preview
        u8g2.drawRFrame(BOX_X, BOX_Y, BOX_W, BOX_H, 3);
        u8g2.drawHLine(BOX_X + 2, MID_Y, BOX_W - 4);

        if (_record_buffer && original_length > 0) {
            int wave_cols = BOX_W - 8;
            for (int x = 0; x < wave_cols; x++) {
                uint32_t s_start = (x * original_length) / wave_cols;
                uint32_t s_end = ((x + 1) * original_length) / wave_cols;
                if (s_end > original_length) s_end = original_length;

                int16_t max_v = 0;
                for (uint32_t s = s_start; s < s_end; s++) {
                    int16_t v = abs(_record_buffer[s]);
                    if (v > max_v) max_v = v;
                }

                int bar_h = (int)((float)max_v / 32768.0f * (BOX_H / 2 - 4));
                if (bar_h > 0) {
                    u8g2.drawVLine(BOX_X + 4 + x, MID_Y - bar_h, bar_h * 2 + 1);
                }
            }

            int trimStartX = BOX_X + 4 + (_trim_start_samples * wave_cols) / original_length;
            int trimEndX = BOX_X + 4 + (_trim_end_samples * wave_cols) / original_length;
            u8g2.drawVLine(trimStartX, BOX_Y + 2, BOX_H - 4);
            u8g2.drawVLine(trimEndX, BOX_Y + 2, BOX_H - 4);

            char tagBuf[24];
            snprintf(tagBuf, sizeof(tagBuf), "%.2fs", (float)sample_length / SAMPLER_SAMPLE_RATE);
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(BOX_X + 4, BOX_Y + 9, "LIVE RAM");
            int tw = u8g2.getStrWidth(tagBuf);
            u8g2.drawStr(BOX_X + BOX_W - tw - 4, BOX_Y + 9, tagBuf);
        } else {
            u8g2.setFont(u8g2_font_6x10_tr);
            u8g2.drawStr(12, 34, "No Audio Sample");
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(12, 48, "Toggle 'Record' on SYNTH tab");
        }
    }
}

void InstrumentSampler::onParamChanged(uint8_t paramIndex) {
    if (paramIndex == 0) { // Record toggle
        if (_param_record > 0.5f && !isRecording) {
            startRecording();
        } else if (_param_record <= 0.5f && isRecording) {
            stopRecording();
        }
    } else if (paramIndex == 1) { // In Volume
        amy_event e = amy_default_event();
        e.osc = 60;
        e.wave = AUDIO_IN0;
        float vol = _param_in_vol / 100.0f;
        e.amp_coefs[COEF_CONST] = vol;
        e.velocity = (vol > 0.001f) ? 1.0f : 0.0f;
        amy_add_event(&e);
    } else if (paramIndex == 2 || paramIndex == 3) { // Trim
        if (!isRecording && _record_buffer && original_length > 0) {
            reloadTrimmedSample();
            needsUIRedraw = true;
        }
    } else if (paramIndex == 4) { // Gain
        if (!isRecording && _record_buffer && original_length > 0) {
            reloadTrimmedSample();
        }
    } else {
        Instrument::onParamChanged(paramIndex - 5);
    }
}
