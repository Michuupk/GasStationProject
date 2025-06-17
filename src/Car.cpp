#include "Car.h"

#include <complex>
#include <mutex>
#include <random>

#include "Simulation.h"

Car::Car(FuelType fuel_type, double fuel_consumption, double fuel_capacity,
         void* parent) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);

    dirtiness = dis(gen);
    fuel_content = dis(gen) * fuel_capacity;

    this->fuel_type = fuel_type;
    this->fuel_consumption = fuel_consumption;
    this->fuel_capacity = fuel_capacity;
    this->fuel_content = fuel_capacity;
    this->parent = parent;

    tank_cycles = 0;
    wash_cycles = 0;

    is_running = true;
    pthread_create(&this->thread, nullptr, car_thread, this);
    pthread_cond_init(&fill_cond, nullptr);
    pthread_cond_init(&wash_cond, nullptr);
    pthread_mutex_init(&mutex, nullptr);

    status = new char[64];
}

Car::~Car() {
    is_running = false;
    pthread_join(thread, nullptr);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&fill_cond);
    pthread_cond_destroy(&wash_cond);
    delete[] status;
}

void* Car::car_thread(void* arg) {
    auto car = (Car*)arg;
    auto simulation = (Simulation*)car->parent;

    while (car->is_running) {
        if (car->fuel_content < car->fuel_capacity * 0.2) {
            uint8_t best_queue = 255, best_id = 0;
            for (int i = 0; i < Gas_Station_Count; i++) {
                auto gs = simulation->gas_stations[i];
                if (gs->has_enough_fuel(
                        car->fuel_type,
                        car->fuel_capacity - car->fuel_content)) {
                    uint8_t current_queue_size =
                        gs->getQueueSize(car->fuel_type);
                    if (current_queue_size < best_queue) {
                        best_queue = current_queue_size;
                        best_id = i;
                    }
                }
            }
            car->tank_cycles++;
            sprintf(car->status, "Driving to %d gs", best_id + 1);
            simulation->gas_stations[best_id]->pump_car(car);

            pthread_mutex_lock(&car->mutex);
            while (car->fuel_capacity > car->fuel_content) {
                pthread_cond_wait(&car->fill_cond, &car->mutex);
            }
            pthread_mutex_unlock(&car->mutex);
        }

        else if (car->dirtiness > 0.9) {
            uint8_t best_queue = 255, best_id = 0;
            for (int i = 0; i < Gas_Station_Count; i++) {
                if (simulation->gas_stations[i]->has_washing_station() &&
                    simulation->gas_stations[i]
                        ->getWashingStation()
                        ->has_enough_detergent(car->dirtiness) &&
                        !simulation->gas_stations[i]->getWashingStation()->resupply_running)
                    {
                    uint8_t current_queue_size = simulation->gas_stations[i]
                                                     ->getWashingStation()
                                                     ->get_queue_size();
                    if (current_queue_size < best_queue) {
                        best_queue = current_queue_size;
                        best_id = i;
                    }
                }
            }
            car->wash_cycles++;
            sprintf(car->status, "Driving to %d ws", best_id + 1);
            simulation->gas_stations[best_id]->getWashingStation()->wash_car(car);

            pthread_mutex_lock(&car->mutex);
            while (car->dirtiness > 0) {
                pthread_cond_wait(&car->wash_cond, &car->mutex);
            }
            pthread_mutex_unlock(&car->mutex);
        }

        else {
            car->state = Moving;
            auto maxDistance =
                floor(car->fuel_content / car->fuel_consumption);
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, maxDistance);
            uint32_t distance = dis(gen);
            // Assert travelling 10 km takes 5 ms
            timespec ts{0, 5000000};
            for (int i = 0; i < distance; i += 10) {
                sprintf(car->status, "driven %d km", i);

                if (i + 10 > distance) {
                    car->fuel_content -= car->fuel_consumption * (distance - i);
                    ts.tv_nsec =
                        ceil((distance - i) / (double)distance * ts.tv_nsec);
                    break;
                }

                car->dirtiness += 0.05;
                car->fuel_content -= car->fuel_consumption * 7.2;
                nanosleep(&ts, nullptr);
            }
        }
        car->state = Idle;
    }
    return nullptr;
}

char* Car::getStatus() const { return status; }

double Car::get_fuel_missing() const { return fuel_capacity - fuel_content; }

double Car::get_dirtiness() { return dirtiness; }

void Car::fill_fuel(double amount) {
    fuel_content += amount;
    if (fuel_content > fuel_capacity) {
        fuel_content = fuel_capacity;
    }
}

void Car::clean_car(double amount) { dirtiness -= amount; }

FuelType Car::get_fuel_type() const { return fuel_type; }

uint32_t Car::getTankCycles() { return tank_cycles; }

uint32_t Car::getWashCycles() { return wash_cycles; }

void Car::finish_pumping() { pthread_cond_signal(&fill_cond); }

void Car::finish_washing() { pthread_cond_signal(&wash_cond); }