#include "Simulation.h"

#include <inttypes.h>
#include <stdio.h>
#include <fstream>

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
        cars[i] = new Car(ft, cons, (int32_t)rand() % 20 + 30, this);
    }

    log_file.open(R"(R:\studia\gas_station\gas_station\src\output.log)", std::fstream::out | std::ios_base::app);
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

    if (log_file.is_open()) {
        log_file.close();
    }

}

void Simulation::run() {
    timespec ts = {1, 0};
    uint32_t counter = 0;

    while (true) {
        char buffer[4096];

        // Gas station status
        sprintf(buffer, "%11s || %5s | %-9s | %-9s | %-9s\n", "GS number", "Queue",
               "C distD", "C distG", "C distLPG");
        printf("%s", buffer);
        //log_file << buffer;

        for (int i = 0; i < Gas_Station_Count; i++) {
            sprintf(buffer, "%-12d: %35s\n", i + 1, gas_stations[i]->getStatus());
            printf("%s", buffer);
        }
        log_file << buffer;

        // Car status
        sprintf(buffer, "%10s || %19s | %-13s | %4s |\n", "Car number", "Car Status",
               "Tank cycles", "Wash cycles");
        printf("%s", buffer);
        log_file << buffer;

        for (int i = 0; i < Car_Count; i++) {
            sprintf(buffer, "Car %-6d | %20s | %-13d | %-10d \n", i,
                   cars[i]->getStatus(), cars[i]->getTankCycles(),
                   cars[i]->getWashCycles());
            printf("%s", buffer);
            log_file << buffer;
        }

        // Tanker status
        sprintf(buffer, "%12s || %-24s | %-7s |\n", "Tanker number", "Tanker Status",
               "Refill Cycles");
        printf("%s", buffer);
        log_file << buffer;

        for (int i = 0; i < Tanker_Count; i++) {
            sprintf(buffer, "Tanker %-6d | %-25s | %-5u\n", i, tankers[i].getStatus(),
                   tankers[i].getRefills());
            printf("%s", buffer);
            log_file << buffer;
        }

        // Washing station status
        sprintf(buffer, "%13s || %-14s | %-7s | %-15s | %-7s |\n", "Washer number",
               "Washer Queue", "Detergent left", "Washer status",
               "Washer Cycles");
        printf("%s", buffer);
        log_file << buffer;

        for (int i = 0; i < Washing_Station_Count; i++) {
            sprintf(buffer, "WS %-10d | %15hhu | %14u | %15s | %3d \n", i,
                   washing_stations[i]->get_queue_size(),
                   washing_stations[i]->getWasherContent(),
                   washing_stations[i]->getStatus(),
                   washing_stations[i]->getWasherCycles());
            printf("%s", buffer);
            log_file << buffer;
        }

        // Cargo car status
        sprintf(buffer, "%-12s || %-25s | %7s | \n", "CargoCar number",
               "CargoCar Status", "CargoCar Cycles");
        printf("%s", buffer);
        log_file << buffer;

        for (int i = 0; i < Cargo_Car_Count; i++) {
            sprintf(buffer, "CargoCar %-6d | %26s | %3d \n", i,
                   cargo_cars[i].getStatus(), cargo_cars[i].getCargoCycles());
            printf("%s", buffer);
            log_file << buffer;
        }

        // Flush the log file to ensure writing
        log_file.flush();

        counter+=1;
        sprintf(buffer, "%s %d \n", "Seconds passed:", counter);
        printf("%s", buffer);
        log_file << buffer;

        nanosleep(&ts, nullptr);
        printf("%*s\r", Tanker_Count + Car_Count + Gas_Station_Count, "\033[A");
    }
}
