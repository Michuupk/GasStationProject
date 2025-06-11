#include "Simulation.h"

#include <inttypes.h>
#include <stdio.h>

#include <random>

#include "Car.h"
#include "Cargo_Car.h"
#include "Gas_Station.h"
#include "Tanker.h"
#include "Washing_Station.h"

Simulation::Simulation() {
    cars = new Car*[Car_Count];
    tankers = new Tanker[Tanker_Count];
    gas_stations = new Gas_Station*[Gas_Station_Count];
    washing_stations = new Washing_Station*[Washing_Station_Count];
    cargo_cars = new Cargo_Car[Cargo_Car_Count];

    for (int i = 0; i < Gas_Station_Count; i++) {
        gas_stations[i] = new Gas_Station(this);
    }

    for (int i = 0; i < Washing_Station_Count; i++) {
        washing_stations[i] = new Washing_Station(this);
    }

    std::vector<int> v;
    for (int i = 0; i < Gas_Station_Count; i++) {
        v.push_back(i);
    }

    for (int i = 0; i < Washing_Station_Count; i++) {
        int index = rand() % v.size();
        gas_stations[index]->assign_washing_station(washing_stations[i]);
        if (index != v.size() - 1) {
            std::swap(v[index], v[v.size() - 1]);
        }
        v.pop_back();
    }

    for (int i = 0; i < Car_Count; i++) {
        FuelType ft = (FuelType)(i % 3);
        auto cons = (rand() % 100 + 50) / 100;
        cars[i] = new Car(ft, cons, (int32_t)rand() % 50 + 30, this);
    }
}

Simulation::~Simulation() {
    for (int i = 0; i < Car_Count; i++) {
        delete cars[i];
    }

    for (int i = 0; i < Gas_Station_Count; i++) {
        delete gas_stations[i];
    }

    for (int i = 0; i < Washing_Station_Count; i++) {
        delete washing_stations[i];
    }

    delete[] cars;
    delete[] gas_stations;
    delete[] tankers;
    delete[] washing_stations;
}

void Simulation::run() {
    timespec ts = {1, 0};

    while (true) {
        printf("%11s| %5s | %9s | %9s | %9s\n", "GS number", "Queue",
               "C   distD", "C   distG", "C distLPG");
        for (int i = 0; i < Gas_Station_Count; i++) {
            printf("%9d: %s\n", i + 1, gas_stations[i]->getStatus());
            // printf("%s\n", gas_stations[i]->getStatus());
        }

        printf("%9s| %15s | %7s | %4s | %4s \n", "Car number", "Car Status",
               "Tank cycles", "Wash cycles");
        for (int i = 0; i < Car_Count; i++) {
            printf("Car %5d %20s %6d %6d \n", i, cars[i]->getStatus(),
                   cars[i]->getTankCycles(), cars[i]->getWashCycles());
        }

        printf("%18s| %25s | %7s \n", "Tanker number",
               "Tanker Status", "Refill Cycles");
        for (int i = 0; i < Tanker_Count; i++) {
            printf("Tanker %2d %25s %5u\n",i, tankers[i].getStatus(), tankers[i].getRefills());
        }


        printf("%18s| %25s | %7s \n", "Washer number","Washer Status", "Washer Cycles");
        for (int i = 0; i <Washing_Station_Count; i++) {
            printf("WS %6d %20s\n", i, washing_stations[i]->getStatus());
        }

        printf("%18s| %25s | %7s \n", "CargoCar number","CargoCar Status", "CargoCar Cycles");
        for (int i =0;i<Cargo_Car_Count;i++) {
            printf("CargoCar %d %10s \n",i, cargo_cars[i].getStatus());
        }

        nanosleep(&ts, nullptr);
        printf("%*c\r", Tanker_Count + Car_Count + Gas_Station_Count, "\033[A");
    }
}
