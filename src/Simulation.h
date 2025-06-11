#ifndef SIMULATION_H
#define SIMULATION_H
#include "Car.h"
#include "Cargo_Car.h"
#include "Gas_Station.h"
#include "Tanker.h"
#include "Washing_Station.h"

class Simulation {
   public:
#define Washing_Station_Count 2
#define Car_Count 2
#define Tanker_Count 4
#define Gas_Station_Count 5
#define Cargo_Car_Count 1

    Washing_Station** washing_stations;
    Car** cars;
    Tanker* tankers;
    Gas_Station** gas_stations;
    Cargo_Car* cargo_cars;

   public:
    Simulation();
    ~Simulation();

    void run();
};

#endif  // SIMULATION_H