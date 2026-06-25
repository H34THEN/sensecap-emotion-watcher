#include "circe_encoder.h"

#include "lvgl.h"

void circe_encoder_state_reset(circe_encoder_state_t *st)
{
    if (!st) {
        return;
    }
    st->pressed = false;
    st->press_start_ms = 0;
    st->last_release_ms = 0;
    st->tap_count = 0;
    st->long_fired = false;
}

static lv_indev_t *find_encoder_indev(void)
{
    lv_indev_t *indev = NULL;
    while ((indev = lv_indev_get_next(indev)) != NULL) {
        if (indev->driver->type == LV_INDEV_TYPE_ENCODER) {
            return indev;
        }
    }
    return NULL;
}

int circe_encoder_read_diff(void)
{
    lv_indev_t *enc = find_encoder_indev();
    if (!enc || !enc->driver || !enc->driver->read_cb) {
        return 0;
    }
    lv_indev_data_t data;
    lv_memset_00(&data, sizeof(data));
    enc->driver->read_cb(enc->driver, &data);
    return data.enc_diff;
}

bool circe_encoder_read_pressed(void)
{
    lv_indev_t *indev = NULL;
    while ((indev = lv_indev_get_next(indev)) != NULL) {
        if (indev->driver->type == LV_INDEV_TYPE_ENCODER) {
            return indev->proc.state == LV_INDEV_STATE_PRESSED;
        }
    }
    return false;
}

int circe_encoder_poll(circe_encoder_state_t *st, int enc_diff, bool pressed)
{
    (void)enc_diff;
    if (!st) {
        return CIRCE_ENC_ACTION_NONE;
    }

    uint32_t now = lv_tick_get();
    int action = CIRCE_ENC_ACTION_NONE;

    if (st->tap_count == 1 && st->last_release_ms != 0 && !st->pressed &&
        (now - st->last_release_ms) >= CIRCE_ENC_TRIPLE_MS) {
        st->tap_count = 0;
        st->last_release_ms = 0;
        return CIRCE_ENC_ACTION_SELECT;
    }

    if (pressed && !st->pressed) {
        st->pressed = true;
        st->press_start_ms = now;
        st->long_fired = false;
    } else if (!pressed && st->pressed) {
        st->pressed = false;
        if (st->long_fired) {
            st->tap_count = 0;
            st->last_release_ms = 0;
        } else if (st->last_release_ms != 0 && (now - st->last_release_ms) < CIRCE_ENC_TRIPLE_MS) {
            st->tap_count++;
        } else {
            st->tap_count = 1;
        }
        st->last_release_ms = now;

        if (st->tap_count >= 3) {
            action = CIRCE_ENC_ACTION_TRIPLE;
            st->tap_count = 0;
            st->last_release_ms = 0;
        } else if (st->tap_count == 2) {
            action = CIRCE_ENC_ACTION_DOUBLE;
            st->tap_count = 0;
            st->last_release_ms = 0;
        }
    } else if (pressed && st->pressed && !st->long_fired && (now - st->press_start_ms) >= CIRCE_ENC_LONG_MS) {
        st->long_fired = true;
        st->tap_count = 0;
        st->last_release_ms = 0;
        action = CIRCE_ENC_ACTION_LONG;
    }

    return action;
}
