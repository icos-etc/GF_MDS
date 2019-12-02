# GF_MDS

This code applies the gapfilling Marginal Distribution Sampling method described in Reichstein et al. 2005 (Global Change Biology).
This version allows larger flexibility in the selection of the drivers.

Differences respect to the original method are the possibilities to:
1) define the variable to fill and the drivers to use
2) change the tolerance of the different drivers (see the paper for details)
3) process a multi-years dataset
4) process hourly timeseries

Basic on the use:
The MDS method uses look-up-tables defined around each single gap, looking for the best compromise between size of the window (as small as possible) and number of drivers used.
The main driver (driver1) is used when it is not possible to fill the gap using all the three drivers (driver1, driver2a and driver2b).
For details see the original paper.
