#ifndef Storage_H
#define Storage_H

#include "esp_err.h"
#include "sd_protocol_types.h"

class Storage
{
private:
    static std::unique_ptr<Storage> instance;
    struct _cons
    {
        explicit _cons() = default;
    };

    sdmmc_card_t *card{nullptr};
    uint32_t total_sect{0};
    uint32_t free_sect{0};

public:
    Storage(_cons) {}

    static std::unique_ptr<Storage> instanceFactory()
    {
        return std::make_unique<Storage>(_cons{});
    }

    static Storage *getInstance()
    {
        /*
            If control enters the declaration concurrently while the variable is being initialized,
            the concurrent execution shall wait for completion of the initialization.
        */
        static std::unique_ptr<Storage> instance{Storage::instanceFactory()};
        return instance.get();
    }

    esp_err_t mount();
    esp_err_t unmount();

    uint32_t getTotalSectors() const { return total_sect; }
    uint32_t getFreeSectors() const { return free_sect; }
};

#endif /* Storage_H */