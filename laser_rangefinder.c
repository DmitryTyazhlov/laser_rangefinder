#include <furi.h>
#include <gui/gui.h>
#include <furi_hal.h>
#include "vl53l1x.h"



void laser_rangefinder_draw_callback(Canvas* canvas, void* context) {
    uint16_t distance = *(uint16_t*)context;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    char buf[32];
    snprintf(buf, sizeof(buf), "Distance: %u mm", distance);
    canvas_draw_str(canvas, 5, 20, buf);
}

static void app_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}


int32_t laser_rangefinder_app(void* p) {
    UNUSED(p);

    uint16_t distance = 0;

    vl53l1x_init();


    // Alloc message queue
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    // Configure view port
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, laser_rangefinder_draw_callback, &distance);
    view_port_input_callback_set(view_port, app_input_callback, event_queue);

    // Register view port in GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Initialize ADC
    FuriHalAdcHandle* adc_handle = furi_hal_adc_acquire();
    furi_hal_adc_configure(adc_handle);

    // Process events
    InputEvent event;
    bool running = true;
    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if(event.type == InputTypePress && event.key == InputKeyBack) {
                running = false;
            }
        } else {
            if(!vl53l1x_read_distance(&distance)) {
                distance = 0xFFFF; // Ошибка
            }
            view_port_update(view_port);
        }
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_record_close(RECORD_GUI);

    return 0;
}