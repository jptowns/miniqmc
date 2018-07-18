////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source
// License.  See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by:
//
// File created by:
////////////////////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include <Particle/ParticleIOUtility.h>

namespace qmcplusplus
{

template <typename T> T count_electrons(const ParticleSet &ions, T scale)
{
  return ions.getTotalNum() * scale * 12;
}

template <typename T>
Tensor<T, 3> tile_cell(ParticleSet &ions, const Tensor<int, 3> &tmat, T scale)
{

  Tensor<T, 3> nio_cell = {7.8811, 7.8811, 0.0, -7.8811, 7.8811,
                           0.0,    0.0,    0.0, 15.7622};
  // set PBC in x,y,z directions
  ions.Lattice.BoxBConds = 1;
  // set the lattice
  ions.Lattice.set(nio_cell); // CrystalLattice.h:321
  // create Ni and O by group
  std::vector<int> nio_group(2);
  nio_group[0] = nio_group[1] = 16;
  ions.create(32); // ParticleSet.h:176 "number of particles per group"
  // using lattice coordinates
  ions.R.InUnit = 1;

  ions.R[0]  = {0.5, 0.0, 0.25}; // O
  ions.R[1]  = {0.0, 0.0, 0.75};
  ions.R[2]  = {0.25, 0.25, 0.5};
  ions.R[3]  = {0.5, 0.5, 0.25};
  ions.R[4]  = {0.75, 0.25, 0.0};
  ions.R[5]  = {0.0, 0.5, 0.75};
  ions.R[6]  = {0.25, 0.75, 0.5};
  ions.R[7]  = {0.75, 0.75, 0.0};
  ions.R[8]  = {0.0, 0.0, 0.25};
  ions.R[9]  = {0.25, 0.25, 0.0};
  ions.R[10] = {0.5, 0.0, 0.75};
  ions.R[11] = {0.75, 0.25, 0.5};
  ions.R[12] = {0.0, 0.5, 0.25};
  ions.R[13] = {0.25, 0.75, 0.0};
  ions.R[14] = {0.5, 0.5, 0.75};
  ions.R[15] = {0.75, 0.75, 0.5};
  ions.R[16] = {0.0, 0.0, 0.0}; // Ni
  ions.R[17] = {0.0, 0.5, 0.0};
  ions.R[18] = {0.25, 0.25, 0.75};
  ions.R[19] = {0.5, 0.0, 0.5};
  ions.R[20] = {0.5, 0.5, 0.5};
  ions.R[21] = {0.75, 0.25, 0.25};
  ions.R[22] = {0.25, 0.75, 0.75};
  ions.R[23] = {0.75, 0.75, 0.25};
  ions.R[24] = {0.5, 0.0, 0.0};
  ions.R[25] = {0.0, 0.0, 0.5};
  ions.R[26] = {0.25, 0.25, 0.25};
  ions.R[27] = {0.5, 0.5, 0.0};
  ions.R[28] = {0.75, 0.25, 0.75};
  ions.R[29] = {0.0, 0.5, 0.5};
  ions.R[30] = {0.25, 0.75, 0.25};
  ions.R[31] = {0.75, 0.75, 0.75};

  SpeciesSet &species(ions.getSpeciesSet());
  species.addSpecies("O");
  species.addSpecies("Ni");

  expandSuperCell(ions, tmat);

  ions.resetGroups();

  return nio_cell;
}

template <typename J2Type> void buildJ2(J2Type &J2, double rcut)
{
  using Func     = typename J2Type::FuncType;
  using RealType = typename Func::real_type;
  const int npts = 10;
  std::string optimize("no");
  rcut = std::min(rcut, 5.5727792532);

  { // add uu/dd
    std::vector<RealType> Y = {0.3062333936,
                               0.2068358231,
                               0.1324267459,
                               0.08673342451,
                               0.05641590391,
                               0.03626533946,
                               0.02269217959,
                               0.01331043612,
                               0.006750919735,
                               0.002827476478,
                               0.0};
    std::string suu("uu");
    Func *f = new Func;
    f->setupParameters(npts, rcut, 0.0, Y);
    J2.addFunc(0, 0, f);
  }
  { // add ud/du
    std::vector<RealType> Y = {0.4397856796,
                               0.2589475889,
                               0.1516634434,
                               0.08962897215,
                               0.05676614496,
                               0.03675758961,
                               0.02294244601,
                               0.01343860721,
                               0.00681803725,
                               0.002864695148,
                               0.0};
    std::string suu("ud");
    Func *f = new Func;
    f->setupParameters(npts, rcut, 0.0, Y);
    J2.addFunc(0, 1, f);
  }
}

template <typename J1Type> void buildJ1(J1Type &J1, double rcut)
{
  using Func     = typename J1Type::FuncType;
  using RealType = typename Func::real_type;
  const int npts = 10;
  std::string optimize("no");
  rcut = std::min(rcut, 4.8261684030);

  // oxygen
  std::vector<RealType> Y = {-1.060468469,
                             -0.9741652012,
                             -0.8785027166,
                             -0.6350075366,
                             -0.3276892194,
                             -0.08646817952,
                             -0.07340983171,
                             -0.0377479213,
                             -0.01507835571,
                             -0.001101691898,
                             0.0};
  std::string suu("O");
  Func *f = new Func;
  f->setupParameters(npts, rcut, -0.25, Y);
  J1.addFunc(0, f);

  // nickel
  Y   = {-3.058259288,
       -3.077898004,
       -2.51188807,
       -1.570208063,
       -0.6204423266,
       0.06272826611,
       0.06228214399,
       0.06665099211,
       0.03230352443,
       0.009254036239,
       0.0};
  suu = "Ni";
  f   = new Func;
  f->setupParameters(npts, rcut, -0.25, Y);
  J1.addFunc(1, f);
}

template <typename JeeIType> void buildJeeI(JeeIType &JeeI, double rcut)
{
  using Func     = typename JeeIType::FuncType;
  using RealType = typename Func::real_type;
  std::string optimize("no");
  rcut = std::min(rcut, 4.8261684030);

  { // add uuNi
    std::vector<RealType> Y = {
        -0.003356164484, 0.002412623253,   0.01653623839,
        0.0008341346169, -0.002808360734,  0.000710697475,
        0.01076942152,   0.0009228283355,  0.01576022161,
        -0.003585259096, 0.003323106938,   -0.02282975998,
        -0.002246144403, -0.007196992871,  -0.00404316239,
        0.001465337212,  0.02026982926,    -0.03528735393,
        0.04594087928,   -0.008776410679,  -0.001552528476,
        -0.005554407743, 0.001858594451,   0.002001634408,
        0.0009302256139, -0.0006304447229};
    std::string suu("uuNi");
    Func *f = new Func;
    f->cutoff_radius = rcut;
    f->resize(3, 3);
    f->Parameters = Y;
    f->reset_gamma();
    JeeI.addFunc(0, 0, 0, f);
  }
  { // add udNi
    std::vector<RealType> Y = {
        -0.006861627197, 0.003278047306,   0.03324006545,
        0.003097361067,  -0.004710623571,  9.652180317e-06,
        0.02212708787,   -0.003718893286,  0.03390124932,
        -0.00710566395,  0.008807743592,   -0.04281661568,
        -0.008463011294, -0.01269994613,   -0.002005229447,
        0.002186590944,  0.03350196472,    -0.05677253817,
        0.07810604648,   -0.009629896208,  -0.006372643712,
        -0.01056861605,  0.002485188615,   0.008392442289,
        1.073423014e-05, -0.0004812466328};
    std::string suu("udNi");
    Func *f = new Func;
    f->cutoff_radius = rcut;
    f->resize(3, 3);
    f->Parameters = Y;
    f->reset_gamma();
    JeeI.addFunc(0, 0, 1, f);
  }
  { // add uuO
    std::vector<RealType> Y = {
        -0.003775082438,  -0.00169971229,   0.02162925441,
        0.005674020544,   -0.0008296047161, 0.00128057705,
        0.005487203215,   0.001637322446,   0.02976838198,
        -0.0003207100945, 0.01143855436,    -0.05336741304,
        -0.00732359381,   -0.01556942626,   0.0001149478453,
        0.001838601199,   0.02570154203,    -0.0675325214,
        0.1080671614,     -0.01258358969,   0.001839834045,
        -0.02422400426,   0.005154953014,   0.003510582598,
        0.007464427016,   -0.002454817757};
    std::string suu("uuO");
    Func *f = new Func;
    f->cutoff_radius = rcut;
    f->resize(3, 3);
    f->Parameters = Y;
    f->reset_gamma();
    JeeI.addFunc(1, 0, 0, f);
  }
  { // add udO
    std::vector<RealType> Y = {-0.009590393593, 0.002498010871, 0.04225872633,
                               0.00460311261,   -0.01071033503, 0.001253155062,
                               0.02934351285,   -0.01823794726, 0.07224890393,
                               -0.01020046849,  0.006310807929, -0.05655009412,
                               -0.0363775247,   0.002062411388, 0.02037856173,
                               0.003372676617,  0.03915277249,  -0.02680556816,
                               0.08648136635,   0.01499088063,  -0.02231984329,
                               -0.02399792096,  0.001105720128, 0.02196005181,
                               0.003162638982,  -0.00119645772};
    std::string suu("udO");
    Func *f = new Func;
    f->cutoff_radius = rcut;
    f->resize(3, 3);
    f->Parameters = Y;
    f->reset_gamma();
    JeeI.addFunc(1, 0, 1, f);
  }
}
}
