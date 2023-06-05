import netCDF4
root = netCDF4.Dataset("init_rect10x10.nc", "w", format="NETCDF4")
metagrp = root.createGroup("structure")
metagrp.type = "devgrid"
datagrp = root.createGroup("data")
xDim = datagrp.createDimension("x", 10)
yDim = datagrp.createDimension("y", 10)
nLay = datagrp.createDimension("nLayers", 1)
mask = datagrp.createVariable("mask", "f8", ("x", "y"))
mask[:,:] = [[0,0,0,0,0,0,0,0,0,0],
             [0,0,0,0,1,1,0,0,0,0],
             [0,0,0,1,1,1,1,0,0,0],
             [0,0,1,1,1,1,1,1,0,0],
             [0,0,1,1,1,1,1,1,0,0],
             [0,0,0,1,1,1,1,1,0,0],
             [0,1,0,1,1,1,1,1,1,0],
             [0,1,1,0,1,1,1,1,1,0],
             [1,1,0,0,1,1,1,0,0,0],
             [1,1,1,1,1,1,0,0,0,0]]
cice = datagrp.createVariable("cice", "f8", ("x", "y",))
cice[:,:] = 0.5
hice = datagrp.createVariable("hice", "f8", ("x", "y",))
hice[:,:] = 0.1
hsnow = datagrp.createVariable("hsnow", "f8", ("x", "y",))
hsnow[:,:] = 0.0
tice = datagrp.createVariable("tice", "f8", ("x", "y", "nLayers"))
tice[:,:,:] = -1.
root.close()
