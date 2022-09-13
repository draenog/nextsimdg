import netCDF4

# Currently creates the 30 x 30 rect grid files with land-sea mask. For the
# earlier, more rudimentary version of this script, please see the git file
# history. 

nx = 30
ny = 30
nLayers = 1

root = netCDF4.Dataset(f"init_rect{nx}{ny}.res.nc", "w", format="NETCDF4")

metagrp = root.createGroup("structure")
metagrp.type = "simple_rectangular"


datagrp = root.createGroup("data")

xDim = datagrp.createDimension("x", nx)
yDim = datagrp.createDimension("y", ny)
nLay = datagrp.createDimension("nLayers", nLayers)

mask = datagrp.createVariable("mask", "f8", ("x", "y"))
mask[:,:] = [[0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,0,0,0,0,0],
             [0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,0,0,0,0,0],
             [0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0],
             [0,0,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0],
             [0,1,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0],
             [0,1,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0],
             [0,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0],
             [0,0,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0],
             [0,0,0,1,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0,0,0,0,0,0],
             [0,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0],
             [1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0],
             [0,1,0,0,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0],
             [0,0,0,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0],
             [0,0,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,1,1,0,0,0],
             [1,0,1,1,1,1,0,0,0,0,0,0,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,0,0,0],
             [1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,0,0],
             [1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0],
             [1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0],
             [1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0],
             [1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,0],
             [1,0,0,0,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,0,0,0,0,0],
             [1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0],
             [1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,0,0,0,0,0,0]]
antimask = 1 - mask[:,:]
cice = datagrp.createVariable("cice", "f8", ("x", "y",))
cice[:,:] = [[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,1,1,2,2,2,2,2,2,1,1,2,2,1,1,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,1,1,2,3,4,5,4,3,3,2,2,3,2,1,1,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,1,2,4,5,6,5,5,4,3,3,4,2,1,1,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,1,2,5,7,8,7,8,6,4,5,3,2,2,1,1,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,3,5,7,9,9,9,8,6,8,6,4,2,1,1,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,6,0,7,8,9,9,9,9,9,9,8,5,3,2,1,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,6,0,8,8,9,9,9,9,9,9,9,9,7,5,4,2,1,1,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,7,8,9,9,9,9,9,9,9,9,9,6,5,3,2,1,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,8,9,9,9,9,9,9,9,9,9,9,9,6,5,3,1,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,9,0,8,9,9,9,9,9,9,9,9,9,9,8,0,4,3,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,9,9,9,8,9,9,9,9,9,9,7,5,3,1,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,8,9,9,9,7,6,4,3,2,1,1,1,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,8,7,5,3,2,2,1,1,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,9,7,5,3,3,4,2,1,1,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,6,5,5,4,3,2,2,2,1,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,4,2,1,1,2,1,1,1,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,3,2,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]]
cice[:,:] /= 10
hice = datagrp.createVariable("hice", "f8", ("x", "y",))
hice[:,:] = cice[:,:] * 2
hsnow = datagrp.createVariable("hsnow", "f8", ("x", "y",))
hsnow[:,:] = cice[:,:] / 2
tice = datagrp.createVariable("tice", "f8", ("x", "y", "nLayers"))
tice[:,:,0] = -0.5 - cice[:,:]

mdi = -2.**300
# mask data
cice[:,:] = cice[:,:] * mask[:,:] + antimask * mdi
cice.missing_value = mdi
hice[:,:] = hice[:,:] * mask[:,:] + antimask * mdi
hice.missing_value = mdi
hsnow[:,:] = hsnow[:,:] * mask[:,:] + antimask * mdi
hsnow.missing_value = mdi
tice[:,:,0] = tice[:,:,0] * mask[:,:] + antimask * mdi
tice.missing_value = mdi

root.close()
