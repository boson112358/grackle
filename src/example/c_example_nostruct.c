/***********************************************************************
/
/ Example executable using libgrackle
/
/
/ Copyright (c) 2013, Enzo/Grackle Development Team.
/
/ Distributed under the terms of the Enzo Public Licence.
/
/ The full license is in the file LICENSE, distributed with this 
/ software.
************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <grackle.h>

#define mh     1.67262171e-24   
#define kboltz 1.3806504e-16

int main(int argc, char *argv[])
{

  // Set parameters

  // chemistry on
  gr_int use_grackle = 1;
  // cooling on
  gr_int with_radiative_cooling = 1;
  // molecular network with H, He, D
  gr_int primordial_chemistry = 3;
  // metal cooling on
  gr_int metal_cooling = 1;
  // UV background on
  gr_int UVbackground = 1;
  // cooling data for Haardt & Madau 2012 background
  char *grackle_data_file = "../../input/CloudyData_UVB=HM2012.h5";
  // no dust
  gr_int h2_on_dust = 0;
  // include CMB cooling floor
  gr_int cmb_temperature_floor = 1;

  // First, set up the units system.
  // These are conversions from code units to cgs.
  gr_int comoving_coordinates = 0; // 1 if cosmological sim, 0 if not
  gr_float density_units = 1.67e-24;
  gr_float length_units = 1.0;
  gr_float time_units = 1.0e12;
  gr_float velocity_units = length_units / time_units;
  gr_float a_units = 1.0; // units for the expansion factor

  // Set initial expansion factor (for internal units).
  // Set expansion factor to 1 for non-cosmological simulation.
  gr_float initial_redshift = 0.;
  gr_float a_value = 1. / (1. + initial_redshift);

  // Initialize everything.
  if (initialize_grackle(comoving_coordinates,
                         density_units, length_units,
                         time_units, velocity_units,
                         a_units, a_value,
                         use_grackle, with_radiative_cooling,
                         grackle_data_file,
                         primordial_chemistry, metal_cooling,
                         UVbackground, h2_on_dust,
                         cmb_temperature_floor) == 0) {
    fprintf(stderr, "Error in initialize_grackle.\n");
    return 0;
  }

  // Allocate field arrays.
  gr_float *density, *energy, *x_velocity, *y_velocity, *z_velocity,
    *HI_density, *HII_density, *HM_density,
    *HeI_density, *HeII_density, *HeIII_density,
    *H2I_density, *H2II_density,
    *DI_density, *DII_density, *HDI_density,
    *e_density, *metal_density;
  gr_float tiny_number = 1.e-20;

  // Set grid dimension and size.
  // grid_start and grid_end are used to ignore ghost zones.
  gr_int field_size = 1;
  gr_int grid_rank = 3;
  gr_int grid_dimension[3], grid_start[3], grid_end[3];
  int i;
  for (i = 0;i < 3;i++) {
    grid_dimension[i] = 1; // the active dimension not including ghost zones.
    grid_start[i] = 0;
    grid_end[i] = 0;
  }
  grid_dimension[0] = field_size;
  grid_end[0] = field_size - 1;

  density       = malloc(field_size * sizeof(gr_float));
  energy        = malloc(field_size * sizeof(gr_float));
  x_velocity    = malloc(field_size * sizeof(gr_float));
  y_velocity    = malloc(field_size * sizeof(gr_float));
  z_velocity    = malloc(field_size * sizeof(gr_float));
  // for primordial_chemistry >= 1
  HI_density    = malloc(field_size * sizeof(gr_float));
  HII_density   = malloc(field_size * sizeof(gr_float));
  HeI_density   = malloc(field_size * sizeof(gr_float));
  HeII_density  = malloc(field_size * sizeof(gr_float));
  HeIII_density = malloc(field_size * sizeof(gr_float));
  e_density     = malloc(field_size * sizeof(gr_float));
  // for primordial_chemistry >= 2
  HM_density    = malloc(field_size * sizeof(gr_float));
  H2I_density   = malloc(field_size * sizeof(gr_float));
  H2II_density  = malloc(field_size * sizeof(gr_float));
  // for primordial_chemistry >= 3
  DI_density    = malloc(field_size * sizeof(gr_float));
  DII_density   = malloc(field_size * sizeof(gr_float));
  HDI_density   = malloc(field_size * sizeof(gr_float));
  // for metal_cooling = 1
  metal_density = malloc(field_size * sizeof(gr_float));

  // set temperature units
  gr_float temperature_units =  mh * pow(a_units * 
                                         length_units /
                                         time_units, 2) / kboltz;

  gr_float HydrogenFractionByMass = 0.76;

  for (i = 0;i < field_size;i++) {
    density[i] = 1.0;
    HI_density[i] = HydrogenFractionByMass * density[i];
    HII_density[i] = tiny_number * density[i];
    HM_density[i] = tiny_number * density[i];
    HeI_density[i] = (1.0 - HydrogenFractionByMass) * density[i];
    HeII_density[i] = tiny_number * density[i];
    HeIII_density[i] = tiny_number * density[i];
    H2I_density[i] = tiny_number * density[i];
    H2II_density[i] = tiny_number * density[i];
    DI_density[i] = 2.0 * 3.4e-5 * density[i];
    DII_density[i] = tiny_number * density[i];
    HDI_density[i] = tiny_number * density[i];
    e_density[i] = tiny_number * density[i];
    // solar metallicity
    metal_density[i] = my_chemistry.SolarMetalFractionByMass * density[i];

    x_velocity[i] = 0.0;
    y_velocity[i] = 0.0;
    z_velocity[i] = 0.0;

    // initilize internal energy (here 1000 K for no reason)
    energy[i] = 1000. / temperature_units;
  }

  /*********************************************************************
  / Calling the chemistry solver
  / These routines can now be called during the simulation.
  *********************************************************************/

  // Evolving the chemistry.
  // some timestep
  gr_float dt = 3.15e7 * 1e6 / time_units;

  if (solve_chemistry_(&comoving_coordinates,
                       &density_units, &length_units,
                       &time_units, &velocity_units,
                       &a_units, &a_value, &dt,
                       &grid_rank, grid_dimension,
                       grid_start, grid_end,
                       density, energy,
                       x_velocity, y_velocity, z_velocity,
                       HI_density, HII_density, HM_density,
                       HeI_density, HeII_density, HeIII_density,
                       H2I_density, H2II_density,
                       DI_density, DII_density, HDI_density,
                       e_density, metal_density) == 0) {
    fprintf(stderr, "Error in solve_chemistry.\n");
    return 0;
  }

  // Calculate cooling time.
  gr_float *cooling_time;
  cooling_time = malloc(field_size * sizeof(gr_float));
  if (calculate_cooling_time_(&comoving_coordinates,
                              &density_units, &length_units,
                              &time_units, &velocity_units,
                              &a_units, &a_value,
                              &grid_rank, grid_dimension,
                              grid_start, grid_end,
                              density, energy,
                              x_velocity, y_velocity, z_velocity,
                              HI_density, HII_density, HM_density,
                              HeI_density, HeII_density, HeIII_density,
                              H2I_density, H2II_density,
                              DI_density, DII_density, HDI_density,
                              e_density, metal_density,
                              cooling_time) == 0) {
    fprintf(stderr, "Error in calculate_cooling_time.\n");
    return 0;
  }

  fprintf(stderr, "Cooling time = %le s.\n", cooling_time[0] *
          time_units);

  // Calculate temperature.
  gr_float *temperature;
  temperature = malloc(field_size * sizeof(gr_float));
  if (calculate_temperature_(&comoving_coordinates,
                             &density_units, &length_units,
                             &time_units, &velocity_units,
                             &a_units,
                             &grid_rank, grid_dimension,
                             density, energy,
                             HI_density, HII_density, HM_density,
                             HeI_density, HeII_density, HeIII_density,
                             H2I_density, H2II_density,
                             DI_density, DII_density, HDI_density,
                             e_density, metal_density, 
                             temperature) == 0) {
    fprintf(stderr, "Error in calculate_temperature.\n");
    return 0;
  }

  fprintf(stderr, "Temperature = %le K.\n", temperature[0]);

  // Calculate pressure.
  gr_float *pressure;
  pressure = malloc(field_size * sizeof(gr_float));
  if (calculate_pressure_(&comoving_coordinates,
                          &density_units, &length_units,
                          &time_units, &velocity_units,
                          &a_units,
                          &grid_rank, grid_dimension,
                          density, energy,
                          HI_density, HII_density, HM_density,
                          HeI_density, HeII_density, HeIII_density,
                          H2I_density, H2II_density,
                          DI_density, DII_density, HDI_density,
                          e_density, metal_density,
                          pressure) == 0) {
    fprintf(stderr, "Error in calculate_pressure.\n");
    return 0;
  }

  fprintf(stderr, "Pressure = %le.\n", pressure[0]);

  // Calculate gamma.
  gr_float *gamma;
  gamma = malloc(field_size * sizeof(gr_float));
  if (calculate_gamma_(&comoving_coordinates,
                       &density_units, &length_units,
                       &time_units, &velocity_units,
                       &a_units,
                       &grid_rank, grid_dimension,
                       density, energy,
                       HI_density, HII_density, HM_density,
                       HeI_density, HeII_density, HeIII_density,
                       H2I_density, H2II_density,
                       DI_density, DII_density, HDI_density,
                       e_density, metal_density,
                       gamma) == 0) {
    fprintf(stderr, "Error in calculate_gamma.\n");
    return 0;
  }

  fprintf(stderr, "gamma = %le.\n", gamma[0]);

  return 1;
}