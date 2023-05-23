# generates c++ header files to define several static const matrices
# used in the dG transport solver.
#

import numpy as np


### DG
def dgdofs(d):    # Number of unknowns per element depending on gauss degree
    if d==0:
        return 1
    elif d==1:
        return 3
    elif d==2:
        return 6
    else:
        assert False,'dG3 and higher is not implemented'

### Inverse element mass matrix for the dg methods
inversemass = np.array([1., 12., 12., 180., 180., 144.])

### Gauss quadrature
gausspoints = np.array([
    [0.5,0,0,0],
    [0.5-np.sqrt(1./12.),0.5+np.sqrt(1./12.),0,0],
    [0.5-np.sqrt(3./20.),0.5,0.5+np.sqrt(3./20.),0],
    [0.5-0.5*np.sqrt(3./7.+2./7.*np.sqrt(6./5.)),
     0.5-0.5*np.sqrt(3./7.-2./7.*np.sqrt(6./5.)),
     0.5+0.5*np.sqrt(3./7.-2./7.*np.sqrt(6./5.)),
     0.5+0.5*np.sqrt(3./7.+2./7.*np.sqrt(6./5.))]])

gaussweights = np.array([
    [1.0,0,0,0],
    [0.5,0.5,0,0],
    [5./18.,8./18.,5./18.,0],
    [(18.-np.sqrt(30.0))/72.,
     (18.+np.sqrt(30.0))/72.,
     (18.+np.sqrt(30.0))/72.,
     (18.-np.sqrt(30.0))/72.]])


def sanitycheck_gauss():
    for p in range(10):    # integrate x^p for p=0,1,2,...,9
        ex = 1.0/(1.0+p)
#        print("Integrate x^{0} over [0,1]: ".format(p),end='')

        for q in range(4): # gauss 1,2,3,4
            gi = 0.0
            for k in range(q+1):
                gi = gi + gaussweights[q,k] * gausspoints[q,k]**p
            if np.fabs(gi-ex)>1.e-14:
                assert p>=2*(q+1), 'Gauss rule should be exact'

# Evaluates the dG-basis functions on [0,1]^2 in (x,y)
def basisfunction(j,x,y):
    if j==0:
        return 1.
    elif j==1:
        return x-0.5
    elif j==2:
        return y-0.5
    elif j==3:
        return (x-0.5)*(x-0.5)-1.0/12.0
    elif j==4:
        return (y-0.5)*(y-0.5)-1.0/12.0
    elif j==5:
        return (x-0.5)*(y-0.5)
    else:
        print("dG3 and higher not implemented (yet)")
        assert False

# Evaluates 1d dG-basis on the edge [0,1]
def edgebasisfunction(j,x):
    if j==0:
        return 1.
    elif j==1:
        return x-0.5
    elif j==2:
        return (x-0.5)*(x-0.5)-1.0/12.0
    else:
        print("dG3 and higher not implemented (yet)")
        assert False

#
# evaluate basis functions in the quadrature points on
# the edges: left, right, up, down
#
# edge: left, right, up, down
# d   : order of dG (0, 1, 2)
# g   : number of gauss points (1,2,3)
def basisfunctions_in_gausspoints(edge, d, g):

    # print header
    print('static const Eigen::Matrix<double, {0}, {1}, Eigen::RowMajor> BiG{2}{3}_{4} ='.format(g,dgdofs(d),d,g,edge))
    print('\t(Eigen::Matrix<double, {0}, {1}, Eigen::RowMajor>() <<'.format(g,dgdofs(d)))
    print('\t',end=' ')
    for gp in range(g):
        for dp in range(dgdofs(d)):
            if (edge==3):
                print(inversemass[dp]*gaussweights[g-1,gp] * basisfunction(dp, 0.0, gausspoints[g-1,gp]),end='')
            elif (edge==1):
                print(inversemass[dp]*gaussweights[g-1,gp] * basisfunction(dp, 1.0, gausspoints[g-1,gp]),end='')
            elif (edge==0):
                print(inversemass[dp]*gaussweights[g-1,gp] * basisfunction(dp, gausspoints[g-1,gp], 0.0),end='')
            elif (edge==2):
                print(inversemass[dp]*gaussweights[g-1,gp] * basisfunction(dp, gausspoints[g-1,gp], 1.0),end='')
            if (gp<g-1 or dp<dgdofs(d)-1):
                print(', ',end='')
            else:
                print(').finished();')


#
# evaluates integral with basis function in cell, scaled by inverse mass
#   M_d^(-1) ( (X), Psi_i(g) )
#      =
#   M_d^(-1) weight(g) * (X) * Psi_i(g)
#
# edge: left, right, up, down
# d   : order of dG (0, 1, 2)
# g   : number of gauss points (1,2,3) in each direction
def integration_basisfunctions_in_gausspoints_cell(d, g):

    # print header
    print('static const Eigen::Matrix<double, {0}, {1}, Eigen::RowMajor> IBC{2}{3} ='.format(g*g,dgdofs(d),d,g))
    print('\t(Eigen::Matrix<double, {0}, {1}, Eigen::RowMajor>() <<'.format(g*g,dgdofs(d)))
    print('\t',end=' ')
    for gx in range(g):
        for gy in range(g):
            for dp in range(dgdofs(d)):
                print(inversemass[dp]*gaussweights[g-1,gx]*gaussweights[g-1,gy] * basisfunction(dp, gausspoints[g-1,gx],gausspoints[g-1,gy]),end='')
                
                if (gx<g-1 or gy<g-1 or dp<dgdofs(d)-1):
                    print(', ',end='')
                else:
                    print(').finished();')

                
                #
# evaluate basis functions in the quadrature points on
# the cell
#
# d   : order of dG (0, 1, 2)
# g   : Gauss points in each direction (2,3)
def basisfunctions_in_gausspoints_cell(d, g):

    # print header
    print('static const Eigen::Matrix<double, {0}, {1}, Eigen::RowMajor> BiG{2}{3} ='.format(dgdofs(d),g*g,d,g))
    print('\t(Eigen::Matrix<double, {0}, {1}, Eigen::RowMajor>() <<'.format(dgdofs(d),g*g))
    print('\t',end=' ')
    for dp in range(dgdofs(d)):
        for gx in range(g):
            for gy in range(g):
                print(basisfunction(dp, gausspoints[g-1,gx], gausspoints[g-1,gy]),end='')

                if (gx<g-1 or gy<g-1 or dp<dgdofs(d)-1):
                    print(', ',end='')
                else:
                    print(').finished();')


#
# evaluate edge basis functions in the quadrature points

# d   : order of dG (0, 1, 2)
# g   : number of gauss points (1,2,3)
def edge_basisfunctions_in_gausspoints(d, g):

    # print header
    print('static const Eigen::Matrix<double, {0}, {1}, Eigen::RowMajor> BiGe{2}{3} ='.format(d+1,g,d,g))
    print('\t(Eigen::Matrix<double, {0}, {1}, Eigen::RowMajor>() <<'.format(d+1,g))
    print('\t',end=' ')
    for dp in range(d+1):
        for gp in range(g):
            print(edgebasisfunction(dp, gausspoints[g-1,gp]),end='')
            if (dp<d or gp<g-1):
                print(', ',end='')
            else:
                print(').finished();')





### Main

# make sure that Guass quadrature is correct
sanitycheck_gauss()

# Some output
print('#ifndef __BASISFUNCTIONSGUASSPOINTS_HPP')
print('#define __BASISFUNCTIONSGUASSPOINTS_HPP')
print('\n')
print('// Automatically generated by codegeneration/basisfunctions_gausspoints.py')
print('//')
print('// Generates the vectors gauss_points[gq] and gauss_weights[gq]')
print('// - stores the points and weights of the gq-point Guass rule')
print('// - the integration is scaled to [0,1]')
print('')
print('// Generates the matrices BiG[dg][gq]_[e]')
print('// - dg is the degree of the dG space')
print('// - gq is the number of Gauss quadrature points')
print('// - e is the edge number (0-lower, 1-right, 2-up, 3-left)')
print('//')
print('// - Each matrix BiG_e[i,j] stores the value of the j-th basis function')
print('//   in the i-th Gauss quadrature point alongt the edge e, weighted')
print('//   with corresponding Gauss weight and the inverse of the mass matrix')
print('//   inversemass(j) * phi_j( gauss_point(i) ) * gauss_weight(i)')
print('')
print('// Generates the matrices BiGe[dg][gq]')
print('// - stores the value of the basis functions on the edge in ')
print('//   the Guass points along the edge')
print('// - dg is the dG degree and gq the number of Gauss points')
print('// - BiGe[i,j] is simply phi_j( gauss_point(i) )')
print('')
print('// Generates the matrices BiG[dg][gq]')
print('// - stores the value of the basis functions on the cell in ')
print('//   the Guass points')
print('// - dg is the dG degree and gq the number of Gauss points in each direction')
print('// - BiG[i,j] is phi_j( gauss_point(i) )')
print('')


# print out guass points and weights
print('\n\n//------------------------------ Gauss Quadrature\n')
for gp in [1,2,3]:
    print('constexpr double gauss_points{0}[{0}] = {{'.format(gp),end='')
    for q in range(gp):
        print(gausspoints[gp-1,q],end='')
        if q<gp-1:
            print(',',end='')
    print('};')
    print('constexpr double gauss_weights{0}[{0}] = {{'.format(gp),end='')
    for q in range(gp):
        print(gaussweights[gp-1,q],end='')
        if q<gp-1:
            print(',',end='')
    print('};')


print('\n\n//------------------------------ Basis Functions in Gauss Points (edge)\n')
# generate arrays that evaluate basis functions in the gauss points on the edges
for dg in [1,2]:
    for e in [0,1,2,3]:
        basisfunctions_in_gausspoints(e, dg,dg+1)
        print('')

for dg in [1]:
    for e in [0,1,2,3]:
        basisfunctions_in_gausspoints(e, dg,dg+2)
        print('')

print('\n\n//------------------------------ Basis Functions in Gauss Points (cell)\n')

for dg in [1,2]:
    basisfunctions_in_gausspoints_cell(dg,2)
    basisfunctions_in_gausspoints_cell(dg,3)


for dg in [1,2]:
    integration_basisfunctions_in_gausspoints_cell(dg,2)
    integration_basisfunctions_in_gausspoints_cell(dg,3)

    
print('\n\n//------------------------------ Edge Basis Functions in Gauss Points\n')
# generate arrays that evaluate basis functions in the gauss points on the edges
for dg in [1,2]:
    edge_basisfunctions_in_gausspoints(dg,dg+1)
    print('')

for dg in [1]:
    edge_basisfunctions_in_gausspoints(dg,dg+2)
    print('')


# Some output
print('#endif /* __BASISFUNCTIONSGUASSPOINTS_HPP */')
