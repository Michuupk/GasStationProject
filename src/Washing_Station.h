#ifndef WASHING_STATION_H
#define WASHING_STATION_H
#include <cstdint>
#include <deque>
#include <vector>

#include "Car.h"

class Washing_Station {
    uint32_t detergent_capacity;
    uint32_t detergent_content;
    std::deque<Car*> queue;
    Car* washing_machine;
    bool taken;
    static uint32_t wash_time;

    pthread_mutex_t queue_mutex, detergent_mutex, wash_mutex;
    pthread_cond_t wash_cond, resupply_cond;

    bool is_running;
    void* parent;
    pthread_t thread, wash_thread, resupply_thread;

    char* status;
    uint32_t washer_cycles;


   public:
    Washing_Station(void* parent);
    ~Washing_Station();

    static void* washing_station_thread(void* arg);
    static void* washing_station_cleaning_thread(void* arg);
    static void* washing_station_resupply_thread(void* arg);

    void wash_car(Car* car);
    bool resupply_running;

    uint8_t get_queue_size();
    bool has_enough_detergent(double amount) const;
    void fill_detergent(double amount);

    [[nodiscard]] char* getStatus() const;
    [[nodiscard]] uint32_t getWasherContent() const;
    [[nodiscard]] uint32_t getWasherCycles() const;
};

#endif  // WASHING_STATION_H