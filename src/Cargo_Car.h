#ifndef CARGO_CAR_H
#define CARGO_CAR_H
#include <pthread.h>

#include "Washing_Station.h"

class Cargo_Car {
    static double capacity;
    double content;
    Washing_Station* target;

    char* status;
    uint32_t cargo_cycles;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    bool is_running;
    void* parent;
    pthread_t thread;

   public:
    enum state_t { Idle, Delivering, Loading, Unloading } state;
    Cargo_Car();
    ~Cargo_Car();
    static void* cargo_car_thread(void* arg);

    [[nodiscard]] char* getStatus() const;
    [[nodiscard]] uint32_t getCargoCycles() const;

    bool call_resupply(Washing_Station* target);
};
#endif  // CARGO_CAR_H