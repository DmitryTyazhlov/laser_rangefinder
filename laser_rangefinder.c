#include <furi.h>
#include <gui/gui.h>
#include <furi_hal.h>

#include "vl53l1_platform.h"
#include "vl53l1_platform_log.h"
#include "vl53l1_api.h"


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
    
    VL53L1_RangingMeasurementData_t RangingData;
    VL53L1_Dev_t  vl53l1; // center module 
    VL53L1_DEV  Dev = &vl53l1; 
    uint16_t distance = 0;
    
    
    // Alloc message queue
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    
    // Configure view port
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, laser_rangefinder_draw_callback, &distance);
    view_port_input_callback_set(view_port, app_input_callback, event_queue);
    
    // Register view port in GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    
    
    Dev->I2cHandle = &furi_hal_i2c_handle_external;
    Dev->I2cDevAddr = (0x29 << 1);
    
    
    /*** VL53L1X Initialization ***/
    VL53L1_WaitDeviceBooted( Dev );
    VL53L1_DataInit( Dev );
    VL53L1_StaticInit( Dev );
    VL53L1_SetDistanceMode( Dev, VL53L1_DISTANCEMODE_LONG );
    VL53L1_SetMeasurementTimingBudgetMicroSeconds( Dev, 50000 );
    VL53L1_SetInterMeasurementPeriodMilliSeconds( Dev, 500 );
    VL53L1_StartMeasurement( Dev );
    
    // Process events
    InputEvent event;
    bool running = true;
    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if(event.type == InputTypePress && event.key == InputKeyBack) {
                running = false;
            }
        } else {
            VL53L1_WaitMeasurementDataReady( Dev );
            VL53L1_GetRangingMeasurementData( Dev, &RangingData );
            distance = RangingData.RangeMilliMeter;
            VL53L1_ClearInterruptAndStartMeasurement( Dev );
            view_port_update(view_port);

            // sprintf( (char*)buff, "%d, %d, %.2f, %.2f\n\r", RangingData.RangeStatus, RangingData.RangeMilliMeter,
            //         ( RangingData.SignalRateRtnMegaCps / 65536.0 ), RangingData.AmbientRateRtnMegaCps / 65336.0 );
            // HAL_UART_Transmit( &huart2, buff, strlen( (char*)buff ), 0xFFFF );
        }
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_record_close(RECORD_GUI);

    return 0;
}