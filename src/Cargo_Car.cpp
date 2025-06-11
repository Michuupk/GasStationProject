#include "Cargo_Car.h"

#include <filesystem>

double Cargo_Car::capacity = 5e3;

Cargo_Car::Cargo_Car() {
    content = capacity;
    status = new char[64];
    is_running = true;
    target = nullptr;
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);

    pthread_create(&thread, nullptr,cargo_car_thread, this);

}

Cargo_Car::~Cargo_Car() {

    pthread_join(thread, nullptr);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

}

void* Cargo_Car::cargo_car_thread(void* arg) {
    Cargo_Car* cargo_car = (Cargo_Car*)arg;

    sprintf(cargo_car->status, "Idle");
    while (cargo_car->is_running) {
        pthread_mutex_lock(&cargo_car->mutex);
        while (cargo_car->target == nullptr) {
            pthread_cond_wait(&cargo_car->cond, &cargo_car->mutex);
        }
        pthread_mutex_unlock(&cargo_car->mutex);

        timespec ts = {0, (uint32_t)5e6};
        cargo_car->state = Loading;
        uint32_t required = capacity;
        while (required > 0) {
            sprintf(cargo_car->status, "Loading fuel, %4d left", required);
            if (required < 50) {
                cargo_car->content += required;
                ts.tv_nsec = (uint64_t)required * 1e6;
                required = 0;
                nanosleep(&ts, nullptr);
                break;
            }
            cargo_car->content += 50;
            required -= 50;
            nanosleep(&ts, nullptr);
        }

        sprintf(cargo_car->status, "Hauling Detergent");
        cargo_car->state = Delivering;
        ts.tv_nsec = (uint64_t)1e7;
        nanosleep(&ts, nullptr);

        cargo_car->state = Unloading;
        required = capacity;
        while (required > 0) {
            sprintf(cargo_car->status, "Resupplying detergent, %4d left", required);
            if (required < 50) {
                cargo_car->target->fill_detergent(required);
                cargo_car->content -= required;
                ts.tv_nsec = (uint64_t)required * 1e5;
                required = 0;
                nanosleep(&ts, nullptr);

                break;
            }
            cargo_car->target->fill_detergent(50);
            required -= 50;
            nanosleep(&ts, nullptr);
        }
    }

}
char* Cargo_Car::getStatus() const {return status;}

bool Cargo_Car::call_resupply(Washing_Station* target) {
    pthread_mutex_lock(&mutex);

    if (state == Idle) {
        this->target = target;
        state = Delivering;
        pthread_cond_signal(&cond);
    }

    pthread_mutex_unlock(&mutex);

    return this->target == target;
}
