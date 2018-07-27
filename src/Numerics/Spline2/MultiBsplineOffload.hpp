////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source
// License.  See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by:
// Jeongnim Kim, jeongnim.kim@intel.com, Intel Corp.
// Amrita Mathuriya, amrita.mathuriya@intel.com, Intel Corp.
// Ye Luo, yeluo@anl.gov, Argonne National Laboratory
//
// File created by:
// Jeongnim Kim, jeongnim.kim@intel.com, Intel Corp.
////////////////////////////////////////////////////////////////////////////////
// -*- C++ -*-
/**@file MultiBsplineOffload.hpp
 *
 * Master header file to define MultiBsplineOffload
 */
#ifndef QMCPLUSPLUS_MULTIEINSPLINE_OFFLOAD_HPP
#define QMCPLUSPLUS_MULTIEINSPLINE_OFFLOAD_HPP
#include "config.h"
#include <iostream>
#include <Numerics/Spline2/bspline_allocator.hpp>
#include <Numerics/Spline2/MultiBsplineData.hpp>
#include <OMP_target_test/OMPVector.h>
#include <stdlib.h>

namespace qmcplusplus
{

PRAGMA_OMP("omp declare target")
template <typename T> struct MultiBsplineOffload
{
  /// define the einsplie object type
  using spliner_type = typename bspline_traits<T, 3>::SplineType;

  MultiBsplineOffload() {}
  MultiBsplineOffload(const MultiBsplineOffload &in) = delete;
  MultiBsplineOffload &operator=(const MultiBsplineOffload &in) = delete;

  /** compute values vals[0,num_splines)
   *
   * The base address for vals, grads and lapl are set by the callers, e.g.,
   * evaluate_vgh(r,psi,grad,hess,ip).
   */

  static void evaluate_v(const spliner_type *restrict spline_m, T x, T y, T z, T *restrict vals,
                  size_t num_splines);

  static void evaluate_vgl(const spliner_type *restrict spline_m, T x, T y, T z, T *restrict vals, T *restrict grads,
                    T *restrict lapl, size_t num_splines);

  static void evaluate_vgh(const spliner_type *restrict spline_m, T x, T y, T z, T *restrict vals, T *restrict grads,
                    T *restrict hess, size_t num_splines);

  static void evaluate_vgh_v2(const spliner_type *restrict spline_m, T x, T y, T z, T *restrict vals, T *restrict grads,
                    T *restrict hess, size_t num_splines);
};

template <typename T>
inline void MultiBsplineOffload<T>::evaluate_v(const spliner_type *restrict spline_m,
                                           T x, T y, T z, T *restrict vals,
                                           size_t num_splines)
{
  x -= spline_m->x_grid.start;
  y -= spline_m->y_grid.start;
  z -= spline_m->z_grid.start;
  T tx, ty, tz;
  int ix, iy, iz;
  SplineBound<T>::get(x * spline_m->x_grid.delta_inv, tx, ix,
                      spline_m->x_grid.num - 1);
  SplineBound<T>::get(y * spline_m->y_grid.delta_inv, ty, iy,
                      spline_m->y_grid.num - 1);
  SplineBound<T>::get(z * spline_m->z_grid.delta_inv, tz, iz,
                      spline_m->z_grid.num - 1);
  T a[4], b[4], c[4];

  MultiBsplineData<T>::compute_prefactors(a, tx);
  MultiBsplineData<T>::compute_prefactors(b, ty);
  MultiBsplineData<T>::compute_prefactors(c, tz);

  const intptr_t xs = spline_m->x_stride;
  const intptr_t ys = spline_m->y_stride;
  const intptr_t zs = spline_m->z_stride;

  constexpr T zero(0);
  OMPstd::fill_n(vals, num_splines, zero);

  for (size_t i = 0; i < 4; i++)
    for (size_t j = 0; j < 4; j++)
    {
      const T pre00 = a[i] * b[j];
      const T *restrict coefs =
          spline_m->coefs + (ix + i) * xs + (iy + j) * ys + iz * zs;
      for (size_t n = 0; n < num_splines; n++)
        vals[n] +=
            pre00 * (c[0] * coefs[n] + c[1] * coefs[n + zs] +
                     c[2] * coefs[n + 2 * zs] + c[3] * coefs[n + 3 * zs]);
    }
}

template <typename T>
inline void
MultiBsplineOffload<T>::evaluate_vgl(const spliner_type *restrict spline_m,
                                 T x, T y, T z, T *restrict vals,
                                 T *restrict grads, T *restrict lapl,
                                 size_t num_splines)
{
  x -= spline_m->x_grid.start;
  y -= spline_m->y_grid.start;
  z -= spline_m->z_grid.start;
  T tx, ty, tz;
  int ix, iy, iz;
  SplineBound<T>::get(x * spline_m->x_grid.delta_inv, tx, ix,
                      spline_m->x_grid.num - 1);
  SplineBound<T>::get(y * spline_m->y_grid.delta_inv, ty, iy,
                      spline_m->y_grid.num - 1);
  SplineBound<T>::get(z * spline_m->z_grid.delta_inv, tz, iz,
                      spline_m->z_grid.num - 1);

  T a[4], b[4], c[4], da[4], db[4], dc[4], d2a[4], d2b[4], d2c[4];

  MultiBsplineData<T>::compute_prefactors(a, da, d2a, tx);
  MultiBsplineData<T>::compute_prefactors(b, db, d2b, ty);
  MultiBsplineData<T>::compute_prefactors(c, dc, d2c, tz);

  const intptr_t xs = spline_m->x_stride;
  const intptr_t ys = spline_m->y_stride;
  const intptr_t zs = spline_m->z_stride;

  const size_t out_offset = spline_m->num_splines;

  T *restrict gx = grads;
  T *restrict gy = grads + out_offset;
  T *restrict gz = grads + 2 * out_offset;
  T *restrict lx = lapl;
  T *restrict ly = lapl + out_offset;
  T *restrict lz = lapl + 2 * out_offset;

  OMPstd::fill_n(vals,  out_offset  , T());
  OMPstd::fill_n(grads, out_offset*3, T());
  OMPstd::fill_n(lapl,  out_offset*3, T());

  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
    {

      const T pre20 = d2a[i] * b[j];
      const T pre10 = da[i] * b[j];
      const T pre00 = a[i] * b[j];
      const T pre11 = da[i] * db[j];
      const T pre01 = a[i] * db[j];
      const T pre02 = a[i] * d2b[j];

      const T *restrict coefs =
          spline_m->coefs + (ix + i) * xs + (iy + j) * ys + iz * zs;
      const T *restrict coefszs  = coefs + zs;
      const T *restrict coefs2zs = coefs + 2 * zs;
      const T *restrict coefs3zs = coefs + 3 * zs;

      for (int n = 0; n < num_splines; n++)
      {
        const T coefsv    = coefs[n];
        const T coefsvzs  = coefszs[n];
        const T coefsv2zs = coefs2zs[n];
        const T coefsv3zs = coefs3zs[n];

        T sum0 = c[0] * coefsv + c[1] * coefsvzs + c[2] * coefsv2zs +
                 c[3] * coefsv3zs;
        T sum1 = dc[0] * coefsv + dc[1] * coefsvzs + dc[2] * coefsv2zs +
                 dc[3] * coefsv3zs;
        T sum2 = d2c[0] * coefsv + d2c[1] * coefsvzs + d2c[2] * coefsv2zs +
                 d2c[3] * coefsv3zs;
        gx[n] += pre10 * sum0;
        gy[n] += pre01 * sum0;
        gz[n] += pre00 * sum1;
        lx[n] += pre20 * sum0;
        ly[n] += pre02 * sum0;
        lz[n] += pre00 * sum2;
        vals[n] += pre00 * sum0;
      }
    }

  const T dxInv = spline_m->x_grid.delta_inv;
  const T dyInv = spline_m->y_grid.delta_inv;
  const T dzInv = spline_m->z_grid.delta_inv;

  const T dxInv2 = dxInv * dxInv;
  const T dyInv2 = dyInv * dyInv;
  const T dzInv2 = dzInv * dzInv;

  for (int n = 0; n < num_splines; n++)
  {
    gx[n] *= dxInv;
    gy[n] *= dyInv;
    gz[n] *= dzInv;
    lx[n] = lx[n] * dxInv2 + ly[n] * dyInv2 + lz[n] * dzInv2;
  }
}

template <typename T>
inline void
MultiBsplineOffload<T>::evaluate_vgh(const spliner_type *restrict spline_m,
                                 T x, T y, T z, T *restrict vals,
                                 T *restrict grads, T *restrict hess,
                                 size_t num_splines)
{

  int ix, iy, iz;
  T tx, ty, tz;
  T a[4], b[4], c[4], da[4], db[4], dc[4], d2a[4], d2b[4], d2c[4];

  x -= spline_m->x_grid.start;
  y -= spline_m->y_grid.start;
  z -= spline_m->z_grid.start;
  SplineBound<T>::get(x * spline_m->x_grid.delta_inv, tx, ix,
                      spline_m->x_grid.num - 1);
  SplineBound<T>::get(y * spline_m->y_grid.delta_inv, ty, iy,
                      spline_m->y_grid.num - 1);
  SplineBound<T>::get(z * spline_m->z_grid.delta_inv, tz, iz,
                      spline_m->z_grid.num - 1);

  MultiBsplineData<T>::compute_prefactors(a, da, d2a, tx);
  MultiBsplineData<T>::compute_prefactors(b, db, d2b, ty);
  MultiBsplineData<T>::compute_prefactors(c, dc, d2c, tz);

  const intptr_t xs = spline_m->x_stride;
  const intptr_t ys = spline_m->y_stride;
  const intptr_t zs = spline_m->z_stride;

  const size_t out_offset = spline_m->num_splines;

  T *restrict gx = grads;
  T *restrict gy = grads + out_offset;
  T *restrict gz = grads + 2 * out_offset;

  T *restrict hxx = hess;
  T *restrict hxy = hess + out_offset;
  T *restrict hxz = hess + 2 * out_offset;
  T *restrict hyy = hess + 3 * out_offset;
  T *restrict hyz = hess + 4 * out_offset;
  T *restrict hzz = hess + 5 * out_offset;

  OMPstd::fill_n(vals,  out_offset  , T());
  OMPstd::fill_n(grads, out_offset*3, T());
  OMPstd::fill_n(hess,  out_offset*6, T());

  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
    {
      const T *restrict coefs =
          spline_m->coefs + (ix + i) * xs + (iy + j) * ys + iz * zs;
      const T *restrict coefszs  = coefs + zs;
      const T *restrict coefs2zs = coefs + 2 * zs;
      const T *restrict coefs3zs = coefs + 3 * zs;

      const T pre20 = d2a[i] * b[j];
      const T pre10 = da[i] * b[j];
      const T pre00 = a[i] * b[j];
      const T pre11 = da[i] * db[j];
      const T pre01 = a[i] * db[j];
      const T pre02 = a[i] * d2b[j];

#ifdef ENABLE_OFFLOAD
      #pragma omp for nowait
#else
      #pragma omp simd aligned(coefs,coefszs,coefs2zs,coefs3zs,vals,gx,gy,gz,hxx,hyy,hzz,hxy,hxz,hyz)
#endif
      for (int n = 0; n < num_splines; n++)
      {

        T coefsv    = coefs[n];
        T coefsvzs  = coefszs[n];
        T coefsv2zs = coefs2zs[n];
        T coefsv3zs = coefs3zs[n];

        T sum0 = c[0] * coefsv + c[1] * coefsvzs + c[2] * coefsv2zs +
                 c[3] * coefsv3zs;
        T sum1 = dc[0] * coefsv + dc[1] * coefsvzs + dc[2] * coefsv2zs +
                 dc[3] * coefsv3zs;
        T sum2 = d2c[0] * coefsv + d2c[1] * coefsvzs + d2c[2] * coefsv2zs +
                 d2c[3] * coefsv3zs;

        hxx[n] += pre20 * sum0;
        hxy[n] += pre11 * sum0;
        hxz[n] += pre10 * sum1;
        hyy[n] += pre02 * sum0;
        hyz[n] += pre01 * sum1;
        hzz[n] += pre00 * sum2;
        gx[n] += pre10 * sum0;
        gy[n] += pre01 * sum0;
        gz[n] += pre00 * sum1;
        vals[n] += pre00 * sum0;
      }
    }

  const T dxInv = spline_m->x_grid.delta_inv;
  const T dyInv = spline_m->y_grid.delta_inv;
  const T dzInv = spline_m->z_grid.delta_inv;
  const T dxx   = dxInv * dxInv;
  const T dyy   = dyInv * dyInv;
  const T dzz   = dzInv * dzInv;
  const T dxy   = dxInv * dyInv;
  const T dxz   = dxInv * dzInv;
  const T dyz   = dyInv * dzInv;

#ifdef ENABLE_OFFLOAD
  #pragma omp for nowait
#else
  #pragma omp simd aligned(gx,gy,gz,hxx,hyy,hzz,hxy,hxz,hyz)
#endif
  for (int n = 0; n < num_splines; n++)
  {
    gx[n] *= dxInv;
    gy[n] *= dyInv;
    gz[n] *= dzInv;
    hxx[n] *= dxx;
    hyy[n] *= dyy;
    hzz[n] *= dzz;
    hxy[n] *= dxy;
    hxz[n] *= dxz;
    hyz[n] *= dyz;
  }
}

template <typename T>
inline void
MultiBsplineOffload<T>::evaluate_vgh_v2(const spliner_type *restrict spline_m,
                                 T x, T y, T z, T *restrict vals,
                                 T *restrict grads, T *restrict hess,
                                 size_t num_splines)
{

  int ix, iy, iz;
  T tx, ty, tz;
  T a[4], b[4], c[4], da[4], db[4], dc[4], d2a[4], d2b[4], d2c[4];

  x -= spline_m->x_grid.start;
  y -= spline_m->y_grid.start;
  z -= spline_m->z_grid.start;
  SplineBound<T>::get(x * spline_m->x_grid.delta_inv, tx, ix,
                      spline_m->x_grid.num - 1);
  SplineBound<T>::get(y * spline_m->y_grid.delta_inv, ty, iy,
                      spline_m->y_grid.num - 1);
  SplineBound<T>::get(z * spline_m->z_grid.delta_inv, tz, iz,
                      spline_m->z_grid.num - 1);

  MultiBsplineData<T>::compute_prefactors(a, da, d2a, tx);
  MultiBsplineData<T>::compute_prefactors(b, db, d2b, ty);
  MultiBsplineData<T>::compute_prefactors(c, dc, d2c, tz);

  const intptr_t xs = spline_m->x_stride;
  const intptr_t ys = spline_m->y_stride;
  const intptr_t zs = spline_m->z_stride;

  const size_t out_offset = spline_m->num_splines;

  T *restrict gxs = grads;
  T *restrict gys = grads + out_offset;
  T *restrict gzs = grads + 2 * out_offset;

  T *restrict hxxs = hess;
  T *restrict hxys = hess + out_offset;
  T *restrict hxzs = hess + 2 * out_offset;
  T *restrict hyys = hess + 3 * out_offset;
  T *restrict hyzs = hess + 4 * out_offset;
  T *restrict hzzs = hess + 5 * out_offset;

  const T dxInv = spline_m->x_grid.delta_inv;
  const T dyInv = spline_m->y_grid.delta_inv;
  const T dzInv = spline_m->z_grid.delta_inv;
  const T dxx   = dxInv * dxInv;
  const T dyy   = dyInv * dyInv;
  const T dzz   = dzInv * dzInv;
  const T dxy   = dxInv * dyInv;
  const T dxz   = dxInv * dzInv;
  const T dyz   = dyInv * dzInv;

#ifdef ENABLE_OFFLOAD
  #pragma omp for nowait
#else
  #pragma omp simd aligned(vals,gxs,gys,gzs,hxxs,hyys,hzzs,hxys,hxzs,hyzs)
#endif
  for (int n = 0; n < num_splines; n++)
  {
    T val = T();
    T  gx = T();
    T  gy = T();
    T  gz = T();
    T hxx = T();
    T hxy = T();
    T hxz = T();
    T hyy = T();
    T hyz = T();
    T hzz = T();

    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
      {
        const T *restrict coefs =
            spline_m->coefs + (ix + i) * xs + (iy + j) * ys + iz * zs;
        const T *restrict coefszs  = coefs + zs;
        const T *restrict coefs2zs = coefs + 2 * zs;
        const T *restrict coefs3zs = coefs + 3 * zs;

        const T pre20 = d2a[i] * b[j];
        const T pre10 = da[i] * b[j];
        const T pre00 = a[i] * b[j];
        const T pre11 = da[i] * db[j];
        const T pre01 = a[i] * db[j];
        const T pre02 = a[i] * d2b[j];

        T coefsv    = coefs[n];
        T coefsvzs  = coefszs[n];
        T coefsv2zs = coefs2zs[n];
        T coefsv3zs = coefs3zs[n];

        T sum0 = c[0] * coefsv + c[1] * coefsvzs + c[2] * coefsv2zs +
                 c[3] * coefsv3zs;
        T sum1 = dc[0] * coefsv + dc[1] * coefsvzs + dc[2] * coefsv2zs +
                 dc[3] * coefsv3zs;
        T sum2 = d2c[0] * coefsv + d2c[1] * coefsvzs + d2c[2] * coefsv2zs +
                 d2c[3] * coefsv3zs;

        hxx += pre20 * sum0;
        hxy += pre11 * sum0;
        hxz += pre10 * sum1;
        hyy += pre02 * sum0;
        hyz += pre01 * sum1;
        hzz += pre00 * sum2;
        gx  += pre10 * sum0;
        gy  += pre01 * sum0;
        gz  += pre00 * sum1;
        val += pre00 * sum0;
      }
    vals[n] = val;
    gxs[n]   = gx * dxInv;
    gys[n]   = gy * dyInv;
    gzs[n]   = gz * dzInv;
    hxxs[n]  = hxx * dxx;
    hxys[n]  = hxy * dxy;
    hxzs[n]  = hxz * dxz;
    hyys[n]  = hyy * dyy;
    hyzs[n]  = hyz * dyz;
    hzzs[n]  = hzz * dzz;
  }
}
PRAGMA_OMP("omp end declare target")

} /** qmcplusplus namespace */
#endif
