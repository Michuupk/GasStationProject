#include "Tanker.h"

#include <pthread.h>
#include <stdio.h>

#include <cstdint>

#include "Tanker.h"

uint32_t Tanker::capacity = 3000;

Tanker::Tanker() {
    status = new char[64];
    is_running = true;
    target_fuel = FuelType::None;
    target = nullptr;
    state = Idle;
    tanker_refills = 0;

    pthread_cond_init(&cond, nullptr);
    pthread_mutex_init(&mutex, nullptr);

    pthread_create(&thread, nullptr, tanker_thread, this);

    sprintf(status, "Tanker is idle");
}

Tanker::~Tanker() {
    is_running = false;
    pthread_cond_signal(&cond);

    pthread_join(thread, nullptr);
    delete[] status;
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

void* Tanker::tanker_thread(void* arg) {
    auto* tanker = (Tanker*)arg;

    while (tanker->is_running) {
        pthread_mutex_lock(&tanker->mutex);
        while (tanker->target == nullptr) {
            if (!tanker->is_running) return nullptr;
            pthread_cond_wait(&tanker->cond, &tanker->mutex);
        }
        pthread_mutex_unlock(&tanker->mutex);

        timespec ts = {0, (uint32_t)5e7};
        tanker->state = Filling;
        uint32_t required_fuel = capacity;
        while (required_fuel > 0) {
            sprintf(tanker->status, "Loading fuel, %4d left", required_fuel);
            if (required_fuel < 50) {
                tanker->content += required_fuel;
                ts.tv_nsec = (uint64_t)required_fuel * 1e6;
                required_fuel = 0;
                nanosleep(&ts, nullptr);
                break;
            }
            tanker->content += 50;
            required_fuel -= 50;
            nanosleep(&ts, nullptr);
        }

        sprintf(tanker->status, "Hauling Fuel");
        tanker->state = Hauling;

        tanker->tanker_refills++;

        ts.tv_nsec = (uint64_t)1e7;
        nanosleep(&ts, nullptr);

        tanker->state = Emptying;
        required_fuel = capacity;
        while (required_fuel > 0) {
            sprintf(tanker->status, "Unloading fuel, %4d left", required_fuel);
            if (required_fuel < 50) {
                tanker->target->fill_tank((uint8_t)tanker->target_fuel,
                                          required_fuel);
                tanker->content -= required_fuel;
                ts.tv_nsec = (uint64_t)required_fuel * 1e5;
                required_fuel = 0;
                nanosleep(&ts, nullptr);

                break;
            }
            tanker->content -= 50;
            tanker->target->fill_tank((uint8_t)tanker->target_fuel, 50);
            required_fuel -= 50;
            nanosleep(&ts, nullptr);
        }
        sprintf(tanker->status, "Tanker is idle");
        tanker->target->finish_pumping(tanker->target_fuel);

        tanker->target = nullptr;
        tanker->target_fuel = FuelType::None;
        tanker->state = Idle;
    }
    return nullptr;
}

Tanker::state_t Tanker::get_state() {
    pthread_mutex_lock(&mutex);
    auto str = state;
    pthread_mutex_unlock(&mutex);

    return state;
}

bool Tanker::begin_delivery(Gas_Station* gas_station, FuelType fuel) {
    pthread_mutex_lock(&mutex);

    if (state == Idle) {
        this->target = gas_station;
        this->target_fuel = fuel;
        pthread_cond_signal(&cond);
    }

    pthread_mutex_unlock(&mutex);

    return target == gas_station;
}

char* Tanker::getStatus() const { return status; }

uint32_t Tanker::getRefills() { return tanker_refills; }
