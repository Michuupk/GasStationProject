#include "Washing_Station.h"

#include <stdio.h>
#include <string.h>

#include <cstdint>
#include <vector>

#include "Car.h"
#include "Simulation.h"

Washing_Station::Washing_Station(void* parent) {
    pthread_mutex_init(&queue_mutex, nullptr);
    pthread_mutex_init(&detergent_mutex, nullptr);
    pthread_mutex_init(&wash_mutex, nullptr);

    pthread_cond_init(&wash_cond, nullptr);
    pthread_cond_init(&resupply_cond, nullptr);

    status = new char[64];
    is_running = true;
    resupply_running = false;
    detergent_capacity = 1000;
    detergent_content = 500;

    washing_machine = nullptr;
    washer_cycles = 0;

    this->parent = parent;

    pthread_create(&this->thread, nullptr, washing_station_thread, this);
    pthread_create(&this->wash_thread, nullptr, washing_station_cleaning_thread,
                   this);
    pthread_create(&this->resupply_thread, nullptr,
                   washing_station_resupply_thread, this);
}

Washing_Station::~Washing_Station() {
    is_running = false;

    pthread_join(this->thread, nullptr);
    pthread_cond_signal(&wash_cond);
    pthread_join(this->wash_thread, nullptr);
    pthread_cond_signal(&resupply_cond);
    pthread_join(this->resupply_thread, nullptr);

    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&detergent_mutex);
    pthread_mutex_destroy(&wash_mutex);

    pthread_cond_destroy(&wash_cond);
    pthread_cond_destroy(&resupply_cond);

    delete[] status;
}

void* Washing_Station::washing_station_thread(void* arg) {
    Washing_Station* washer = (Washing_Station*)arg;
    timespec ts = {0, (uint32_t)5e7};

    while (washer->is_running) {
        if (washer->washing_machine == nullptr) {
            sprintf(washer->status, "Idle");
            pthread_mutex_lock(&washer->queue_mutex);

            if (washer->queue.size() > 0) {
                washer->washing_machine = washer->queue.front();
                washer->queue.pop_front();

                pthread_mutex_lock(&washer->wash_mutex);
                pthread_cond_signal(&washer->wash_cond);
                pthread_mutex_unlock(&washer->wash_mutex);
            }

            pthread_mutex_unlock(&washer->queue_mutex);
        }

        if (!washer->resupply_running) {
            pthread_mutex_lock(&washer->wash_mutex);

            if (washer->detergent_content < washer->detergent_capacity * 0.2) {
                washer->resupply_running = true;
                pthread_mutex_lock(&washer->detergent_mutex);
                pthread_cond_signal(&washer->resupply_cond);
                pthread_mutex_unlock(&washer->detergent_mutex);
            }
            pthread_mutex_unlock(&washer->wash_mutex);
        }
    }
    return nullptr;
}

void* Washing_Station::washing_station_resupply_thread(void* arg) {
    Washing_Station* washer = (Washing_Station*)arg;
    Simulation* sim = (Simulation*)washer->parent;
    while (washer->is_running) {
        pthread_mutex_lock(&washer->detergent_mutex);
        while (washer->washing_machine == nullptr) {
            if (!washer->is_running) return nullptr;
            pthread_cond_wait(&washer->resupply_cond, &washer->detergent_mutex);
        }
        pthread_mutex_unlock(&washer->detergent_mutex);

        sprintf(washer->status, "Detergent low");

        bool success = false;
        uint16_t count = 0;
        while (!success) {
            if (sim->cargo_cars[count].state == Cargo_Car::Idle) {
                success = sim->cargo_cars[count].call_resupply(washer);
            }
            count = (count + 1) % Cargo_Car_Count;
        }
        washer->resupply_running = false;
    }
    return nullptr;
}

void* Washing_Station::washing_station_cleaning_thread(void* arg) {
    Washing_Station* washer = (Washing_Station*)arg;
    while (washer->is_running) {
        pthread_mutex_lock(&washer->wash_mutex);
        while (washer->washing_machine == nullptr) {
            if (!washer->is_running) return nullptr;
            pthread_cond_wait(&washer->wash_cond, &washer->wash_mutex);
        }
        sprintf(washer->status, "Washing car");

        pthread_mutex_unlock(&washer->wash_mutex);
        double dirtiness = washer->washing_machine->get_dirtiness();

        timespec sleeptime = {0, (uint32_t)1e8};

        while (dirtiness > 0) {
            if (dirtiness < 0.11) {
                washer->washing_machine->clean_car(dirtiness);
                dirtiness = 0.0;
                washer->detergent_content -= 2;
                nanosleep(&sleeptime, nullptr);
            } else {
                washer->washing_machine->clean_car(dirtiness);
                dirtiness -= 0.11;
                washer->detergent_content -= 3;
                nanosleep(&sleeptime, nullptr);
            }
        }

        pthread_mutex_lock(&washer->washing_machine->mutex);
        washer->washer_cycles++;
        washer->washing_machine->finish_washing();
        pthread_mutex_unlock(&washer->washing_machine->mutex);

        washer->washing_machine = nullptr;
    }

    return nullptr;
}

uint8_t Washing_Station::get_queue_size() {
    pthread_mutex_lock(&queue_mutex);
    auto ret = queue.size();
    pthread_mutex_unlock(&queue_mutex);
    return ret;
}

void Washing_Station::wash_car(Car* car) {
    pthread_mutex_lock(&queue_mutex);
    queue.push_back(car);
    pthread_mutex_unlock(&queue_mutex);
}

bool Washing_Station::has_enough_detergent(double amount) const {
    return (amount*3) < detergent_content;
}

void Washing_Station::fill_detergent(double amount) {
    pthread_mutex_lock(&detergent_mutex);
    if (detergent_content < amount && (detergent_content + amount) < detergent_capacity) {
        detergent_content += amount;
    }
    else {
        detergent_content = detergent_capacity;
    }
    pthread_mutex_unlock(&detergent_mutex);
}

char* Washing_Station::getStatus() const { return status; }

uint32_t Washing_Station::getWasherContent() const {
    return this->detergent_content;
}

uint32_t Washing_Station::getWasherCycles() const { return washer_cycles; }
