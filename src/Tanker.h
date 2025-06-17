#ifndef TANKER_H
#define TANKER_H
#include <pthread.h>

#include <cstdint>

#include "Gas_Station.h"

class Tanker {
    pthread_cond_t cond;

    uint32_t content;
    bool is_running;
    void* parent;
    pthread_t thread;
    Gas_Station* target;
    FuelType target_fuel;

    uint32_t tanker_refills;

    char* status;

   public:
    pthread_mutex_t mutex;
    static uint32_t capacity;
    enum state_t : uint8_t { Hauling, Emptying, Filling, Idle } state;

    Tanker();
    ~Tanker();

    uint32_t getRefills();

    static void* tanker_thread(void* arg);
    char* getStatus() const;
    [[nodiscard]] state_t get_state();

    bool begin_delivery(Gas_Station* gas_station, FuelType fuel);
};

#endif  // TANKER_H