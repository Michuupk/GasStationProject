#ifndef CAR_H
#define CAR_H
#include <pthread.h>

#include "FuelType.h"

class Car {
    FuelType fuel_type;
    double fuel_consumption;
    double fuel_capacity;
    double fuel_content;
    double dirtiness;

    uint32_t tank_cycles;
    uint32_t wash_cycles;

    static double dirtiness_rate;

    bool is_running;
    void* parent;
    pthread_t thread;
    pthread_cond_t wash_cond, fill_cond;

    char* status;

   public:
    pthread_mutex_t mutex;
    enum state_t { Idle, Moving, Washing, Filling, Waiting } state;

    Car(FuelType fuel_type, double fuel_consumption, double fuel_capacity,
        void* parent);
    ~Car();

    static void* car_thread(void* arg);

    [[nodiscard]] double get_fuel_missing() const;
    [[nodiscard]] FuelType get_fuel_type() const;
    uint32_t getTankCycles();
    uint32_t getWashCycles();
    [[nodiscard]] double get_dirtiness();

    char* getStatus() const;

    void fill_fuel(double amount);
    void clean_car(double amount);

    void finish_pumping();
    void finish_washing();
};

#endif  // CAR_H