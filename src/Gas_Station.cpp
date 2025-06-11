#include "Gas_Station.h"

#include <cstdio>
#include <mutex>

#include "Cargo_Car.h"
#include "Gas_Station.h"
#include "Simulation.h"
#include "Tanker.h"

uint32_t Gas_Station::tank::capacity = 5000;

Gas_Station::Gas_Station(void* parent) : parent(parent) {
    pthread_mutex_init(&queue_mutex, nullptr);
    pthread_mutex_init(&fill_mutex[0], nullptr);
    pthread_mutex_init(&fill_mutex[1], nullptr);
    pthread_mutex_init(&fill_mutex[2], nullptr);
    pthread_mutex_init(&resupply_mutex, nullptr);

    pthread_cond_init(&fill_cond[0], nullptr);
    pthread_cond_init(&fill_cond[1], nullptr);
    pthread_cond_init(&fill_cond[2], nullptr);
    pthread_cond_init(&resupply_cond, nullptr);

    is_running = true;
    resupply_running = false;

    for (int i = 0; i < 3; i++) {
        tanks[i].content = 5000;
        tanks[i].is_getting_filled = false;
    }

    washing_station = nullptr;
    for (int i = 0; i < 3; i++) pump[i] = nullptr;

    status = new char[64];
    required_fuel = FuelType::None;

    fill_thread_init_t* a = new fill_thread_init_t();
    a->gas_station = this;
    a->fuelID = 0;

    fill_thread_init_t* b = new fill_thread_init_t();
    b->gas_station = this;
    b->fuelID = 1;

    fill_thread_init_t* c = new fill_thread_init_t();
    c->gas_station = this;
    c->fuelID = 2;

    pthread_create(&this->thread, nullptr, gas_station_thread, this);
    pthread_create(&this->resupply_thread, nullptr, gas_station_resupply_thread,
                   this);

    pthread_create(&this->fill_thread[0], nullptr, gas_station_fill_thread, a);
    pthread_create(&this->fill_thread[1], nullptr, gas_station_fill_thread, b);
    pthread_create(&this->fill_thread[2], nullptr, gas_station_fill_thread, c);
}

Gas_Station::~Gas_Station() {
    is_running = false;

    pthread_join(thread, nullptr);
    pthread_cond_signal(&resupply_cond);
    pthread_join(resupply_thread, nullptr);
    pthread_cond_signal(&fill_cond[0]);
    pthread_join(fill_thread[0], nullptr);
    pthread_cond_signal(&fill_cond[1]);
    pthread_join(fill_thread[1], nullptr);
    pthread_cond_signal(&fill_cond[2]);
    pthread_join(fill_thread[2], nullptr);

    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&fill_mutex[0]);
    pthread_mutex_destroy(&fill_mutex[1]);
    pthread_mutex_destroy(&fill_mutex[2]);
    pthread_mutex_destroy(&resupply_mutex);

    pthread_cond_destroy(&fill_cond[0]);
    pthread_cond_destroy(&fill_cond[1]);
    pthread_cond_destroy(&fill_cond[2]);
    pthread_cond_destroy(&resupply_cond);

    delete[] status;
}

void* Gas_Station::gas_station_thread(void* arg) {
    Gas_Station* gas_station = (Gas_Station*)arg;
    timespec ts{0, (uint32_t)5e6};

    while (gas_station->is_running) {
        for (int i = 0; i < 3; i++) {
            if (gas_station->pump[i] == nullptr) {
                pthread_mutex_lock(&gas_station->queue_mutex);
                // printf("a\n");

                for (int element = 0; element < gas_station->queue.size();
                     element++) {
                    if (gas_station->queue[element]->get_fuel_type() ==
                        (FuelType)i) {
                        gas_station->pump[i] = gas_station->queue[element];
                        gas_station->queue.erase(gas_station->queue.begin() +
                                                 element);
                        pthread_cond_signal(&gas_station->fill_cond[i]);
                    }
                }

                pthread_mutex_unlock(&gas_station->queue_mutex);
            }
        }

        for (int i = 0; i < 3; i++) {
            if (!gas_station->resupply_running) {
                pthread_mutex_lock(&gas_station->fill_mutex[i]);

                if (!gas_station->tanks[i].is_getting_filled &&
                    gas_station->tanks[i].content + Tanker::capacity <
                        tank::capacity) {
                    gas_station->resupply_running = true;
                    gas_station->required_fuel = (FuelType)i;
                    pthread_mutex_lock(&gas_station->resupply_mutex);
                    pthread_cond_signal(&gas_station->resupply_cond);
                    pthread_mutex_unlock(&gas_station->resupply_mutex);
                }

                pthread_mutex_unlock(&gas_station->fill_mutex[i]);
            }
        }
        pthread_mutex_lock(&gas_station->queue_mutex);
        sprintf(gas_station->status, "%3llu %5d %7.2lf, %2d %7.2lf, %2d %7.2lf",
                gas_station->queue.size(), gas_station->pump[0] != nullptr,
                gas_station->tanks[0].content, gas_station->pump[1] != nullptr,
                gas_station->tanks[1].content, gas_station->pump[2] != nullptr,
                gas_station->tanks[2].content);
        pthread_mutex_unlock(&gas_station->queue_mutex);
        nanosleep(&ts, nullptr);
    }
    return nullptr;
}

void* Gas_Station::gas_station_resupply_thread(void* arg) {
    auto gas_station = (Gas_Station*)arg;

    while (gas_station->is_running) {
        gas_station->resupply_running = false;
        gas_station->required_fuel = FuelType::None;

        pthread_mutex_lock(&gas_station->resupply_mutex);
        while (gas_station->required_fuel == FuelType::None) {
            pthread_cond_wait(&gas_station->resupply_cond,
                              &gas_station->resupply_mutex);
        }
        pthread_mutex_unlock(&gas_station->resupply_mutex);

        auto success = false;
        auto simulation = (Simulation*)gas_station->parent;
        while (!success) {
            for (int i = 0; i < Tanker_Count && !success; i++) {
                if (simulation->tankers[i].get_state() ==
                    Tanker::state_t::Idle) {
                    success = simulation->tankers[i].begin_delivery(
                        gas_station, gas_station->required_fuel);
                    if (success) {
                        gas_station->tanks[(uint8_t)gas_station->required_fuel]
                            .is_getting_filled = true;
                    }
                }
            }
        }
    }
}

void Gas_Station::finish_pumping(FuelType fuel) {
    tanks[(uint16_t)fuel].is_getting_filled = false;
}

void* Gas_Station::gas_station_fill_thread(void* arg) {
    fill_thread_init_t* fill_thread_init = (fill_thread_init_t*)arg;
    Gas_Station* gas_station = fill_thread_init->gas_station;
    uint8_t fuelID = fill_thread_init->fuelID;

    while (gas_station->is_running) {
        pthread_mutex_lock(&gas_station->fill_mutex[fuelID]);
        while (gas_station->pump[fuelID] == nullptr) {
            if (!gas_station->is_running) return nullptr;
            pthread_cond_wait(&gas_station->fill_cond[fuelID],
                              &gas_station->fill_mutex[fuelID]);
        }
        pthread_mutex_unlock(&gas_station->fill_mutex[fuelID]);

        auto car = gas_station->pump[fuelID];
        auto required_fuel = car->get_fuel_missing();

        timespec ts = {0, (uint32_t)5e6};
        while (required_fuel > 0) {
            if (required_fuel < 5) {
                pthread_mutex_lock(&gas_station->fill_mutex[fuelID]);
                if (gas_station->tanks[fuelID].content >= 5.0) {
                    car->fill_fuel(required_fuel + 0.2);
                    gas_station->tanks[fuelID].content -= required_fuel;
                    required_fuel = 0;
                }
                pthread_mutex_unlock(&gas_station->fill_mutex[fuelID]);

                ts.tv_nsec = (uint64_t)required_fuel * 1e7;
                nanosleep(&ts, nullptr);

                break;
            }

            pthread_mutex_lock(&gas_station->fill_mutex[fuelID]);

            if (gas_station->tanks[fuelID].content >= 5.0) {
                car->fill_fuel(5.0);
                gas_station->tanks[fuelID].content -= 5.0;
                required_fuel -= 5.0;
            }
            pthread_mutex_unlock(&gas_station->fill_mutex[fuelID]);

            nanosleep(&ts, nullptr);
        }

        pthread_mutex_lock(&car->mutex);
        car->finish_pumping();
        gas_station->pump[fuelID] = nullptr;
        pthread_mutex_unlock(&car->mutex);
    }
    delete fill_thread_init;
    return nullptr;
}

/// Fills tank a given amount of fuel
void Gas_Station::fill_tank(uint8_t tank_id, uint8_t amount) {
    pthread_mutex_lock(&fill_mutex[tank_id]);
    tanks[tank_id].content += amount;
    pthread_mutex_unlock(&fill_mutex[tank_id]);
}

/// Pumps a given amount of fuel to a tank
void Gas_Station::pump_car(Car* car) {
    pthread_mutex_lock(&queue_mutex);
    queue.push_back(car);
    pthread_mutex_unlock(&queue_mutex);
}

void Gas_Station::wash_car(Car* car) { washing_station->wash_car(car); }

bool Gas_Station::has_washing_station() { return washing_station != nullptr; }

void Gas_Station::assign_washing_station(Washing_Station* washing_station) {
    this->washing_station = washing_station;
}

bool Gas_Station::has_enough_fuel(FuelType fuel_type, double amount) const {
    return tanks[(uint8_t)fuel_type].content >= amount;
}

uint8_t Gas_Station::getQueueSize(FuelType fuel_type) {
    pthread_mutex_lock(&queue_mutex);
    uint8_t result = 0;
    for (auto& e : queue) {
        result += e->get_fuel_type() == fuel_type;
    }
    result += pump[(uint8_t)fuel_type] != nullptr;
    pthread_mutex_unlock(&queue_mutex);
    return result;
}

Washing_Station* Gas_Station::getWashingStation() { return washing_station; }

char* Gas_Station::getStatus() const { return status; }
