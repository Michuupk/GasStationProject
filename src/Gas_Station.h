//
// Created by Wiktor on 07.06.2025.
//

#ifndef GAS_STATION_H
#define GAS_STATION_H
#include <pthread.h>

#include <cstdint>

#include "Car.h"
#include "Washing_Station.h"

class Gas_Station {
    pthread_mutex_t queue_mutex;
    pthread_mutex_t resupply_mutex;
    double total_fueled[3] = {};
    Car* pump[3] = {};
    std::vector<Car*> queue = {};
    char* status;

    class tank {
        friend class Gas_Station;

        static uint32_t capacity;
        double content = 0;
        bool is_getting_filled = 0;
    } tanks[3] = {};

    struct fill_thread_init_t {
        Gas_Station* gas_station;
        uint8_t fuelID;
    };

    Washing_Station* washing_station = nullptr;

    bool is_running, resupply_running;
    void* parent;
    pthread_t thread, resupply_thread, fill_thread[3];
    pthread_mutex_t fill_mutex[3];
    pthread_cond_t fill_cond[3], resupply_cond;
    FuelType required_fuel;

   public:
    Gas_Station(void* parent);
    ~Gas_Station();

    static void* gas_station_thread(void* arg);
    static void* gas_station_resupply_thread(void* arg);
    static void* gas_station_fill_thread(void* arg);

    void fill_tank(uint8_t tank_id, uint8_t amount);
    void pump_car(Car* car);
    void wash_car(Car* car);
    void finish_pumping(FuelType fuelType);
    void assign_washing_station(Washing_Station* washing_station);
    char* getStatus() const;

    Washing_Station* getWashingStation();

    [[nodiscard]] bool has_washing_station();
    [[nodiscard]] bool has_enough_fuel(FuelType fuel_type, double amount) const;
    uint8_t getQueueSize(FuelType fuel_type);
};
#endif  // GAS_STATION_H