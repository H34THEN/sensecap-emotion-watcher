idf_component_register(
    SRCS
        "main.c"
        "circe_entry.c"
        "circe_copy.c"
        "circe_conversation_engine.c"
        "circe_entry_modes.c"
        "circe_index.c"
        "circe_storage.c"
        "circe_ui.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES sensecap-watcher cjson nvs_flash esp_io_expander esp_io_expander_pca95xx_16bit esp_lvgl_port sscma_client knob fatfs
)
