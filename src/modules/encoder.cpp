#include "encoder.h"
#include "board_config.h"
#if !defined(UI_SINGLE_BUTTON)

int Encoder::getStepCount() const {
    return stepQueue;
}

void Encoder::begin() {
    pinMode(ENCODER_BTN_PIN, INPUT_PULLUP);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    rotary.attachFullQuad(ENCODER_A_PIN, ENCODER_B_PIN);
    rotary.setFilter(200);
    rotary.clearCount();

    lastCount = 0;
    pulseAccumulator = 0;
    stepQueue = 0;
}

void Encoder::update() {
    int64_t currentCount = rotary.getCount();
    int32_t delta = static_cast<int32_t>(currentCount - lastCount);
    lastCount = currentCount;

    if (delta != 0) {
        pulseAccumulator += delta;

        while (pulseAccumulator >= COUNTS_PER_DETENT) {
            stepQueue++;
            pulseAccumulator -= COUNTS_PER_DETENT;
        }

        while (pulseAccumulator <= -COUNTS_PER_DETENT) {
            stepQueue--;
            pulseAccumulator += COUNTS_PER_DETENT;
        }

        if (stepQueue > STEP_QUEUE_LIMIT) stepQueue = STEP_QUEUE_LIMIT;
        if (stepQueue < -STEP_QUEUE_LIMIT) stepQueue = -STEP_QUEUE_LIMIT;
    }

    bool btn = digitalRead(ENCODER_BTN_PIN) == LOW;
    unsigned long now = millis();
    if (btn && !lastBtn && (now - lastClickMs >= CLICK_DEBOUNCE_MS)) {
        clickFlag = true;
        lastClickMs = now;
    }
    lastBtn = btn;

}

bool Encoder::rotatedCW() {
    if (stepQueue > 0) {
        stepQueue--;
        return true;
    }

    return false;
}

bool Encoder::rotatedCCW() {
    if (stepQueue < 0) {
        stepQueue++;
        return true;
    }

    return false;
}

bool Encoder::clicked() {
    if (clickFlag) { clickFlag = false; return true; }
    return false;
}

void Encoder::clearQueue() {
    stepQueue = 0;
    pulseAccumulator = 0;
}

//
#endif
