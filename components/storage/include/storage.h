#ifndef Storage_H
#define Storage_H

#include "componentInterface.h"
#include "esp_err.h"
#include "sd_protocol_types.h"
#include <memory>


namespace storage {
class Storage : public ComponentInterface {
  private:
    static std::unique_ptr<Storage> instance;
    struct _cons {
        explicit _cons() = default;
    };

    sdmmc_card_t *card{nullptr};
    uint32_t total_sect{0};
    uint32_t free_sect{0};

  public:
    Storage(_cons) : ComponentInterface(ComponentId::COMPONENT_ID_STORAGE) {
    }

    static std::unique_ptr<Storage> instanceFactory() {
        return std::make_unique<Storage>(_cons{});
    }

    static Storage *getInstance() {
        /*
            If control enters the declaration concurrently while the variable is being initialized,
            the concurrent execution shall wait for completion of the initialization.
        */
        static std::unique_ptr<Storage> instance{Storage::instanceFactory()};
        return instance.get();
    }

    esp_err_t mount();
    esp_err_t unmount();

    esp_err_t write_file(const char *path, char *data, std::ios_base::openmode mode = std::ios::out);
    esp_err_t read_file(const char *path, char *buffer, size_t buffer_size);
    esp_err_t delete_file(const char *path);

    uint32_t getTotalSectors() const {
        return total_sect;
    }
    uint32_t getFreeSectors() const {
        return free_sect;
    }

    // trampoline Function for performance testing
    static void writeWrapper(void *params) {
        // Cast the generic pointer back to our Object type
        Storage *obj = static_cast<Storage *>(params);

        // Call the member function with your desired arguments
        obj->write_file("/sdcard/test.txt", "Hello, World!");
    }

    esp_err_t messageHandler(const Message &msg) override;
};
} // namespace storage

#endif // Storage_H