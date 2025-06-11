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


    status = new char[64];
    is_running = true;
    detergent_capacity = 1000;
    detergent_content = 1000;

    this->parent = parent;

    pthread_create(&this->thread, nullptr,washing_station_thread, this);
    pthread_create(&this->thread, nullptr,washing_station_cleaning_thread, this);
    pthread_create(&this->thread, nullptr,washing_station_resupply_thread, this);
}

Washing_Station::~Washing_Station() {
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&detergent_mutex);
    pthread_mutex_destroy(&wash_mutex);

    pthread_cond_destroy(&wash_cond);

    delete[] status;
}

void* Washing_Station::washing_station_thread(void* arg) {
    Washing_Station* washer = (Washing_Station*)arg;
    timespec ts = {0, (uint32_t)5e7};



    while (washer->is_running) {
        if (washer->washing_machine != nullptr) {
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

        sprintf(washer->status, "|| Detergent low ||");

        bool success = false;
        uint16_t count = 0;
        while (!success) {
            if (sim->cargo_cars[count].state == Cargo_Car::Idle) {
                success = sim->cargo_cars[count].call_resupply(washer);
            }
            count = (count + 1) % Cargo_Car_Count;
        }
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
        printf("Washing car\n");

        pthread_mutex_unlock(&washer->wash_mutex);
        double dirtiness = washer->washing_machine->get_dirtiness();

        timespec sleeptime = {0, (uint32_t)1e7};
        while (dirtiness > 0) {
            if (dirtiness < 0.11) {
                washer->washing_machine->clean_car(dirtiness);
                dirtiness = 0.0;
                washer->detergent_content -= 2;
                nanosleep(&sleeptime, nullptr);
            }else {
                washer->washing_machine->clean_car(dirtiness);
                dirtiness -= 0.11;
                washer->detergent_content -= 3;
                nanosleep(&sleeptime, nullptr);
            }
        }

        pthread_mutex_lock(&washer->washing_machine->mutex);
        washer->washing_machine->finish_washing();
        pthread_mutex_unlock(&washer->washing_machine->mutex);
        sprintf(washer->status, "Idle");

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
    return amount < detergent_content;
}

void Washing_Station::fill_detergent(double amount) {
    pthread_mutex_lock(&detergent_mutex);
    detergent_content += amount;
    pthread_mutex_unlock(&detergent_mutex);
}

char* Washing_Station::getStatus() const {return status;}
