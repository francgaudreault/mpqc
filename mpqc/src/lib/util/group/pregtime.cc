//
// pregtime.cc
//
// Copyright (C) 1996 Limit Point Systems, Inc.
//
// Author: Curtis Janssen <cljanss@limitpt.com>
// Maintainer: LPS
//
// This file is part of the SC Toolkit.
//
// The SC Toolkit is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The SC Toolkit is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the SC Toolkit; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
// The U.S. Government is granted a limited license as per AL 91-7.
//

#ifdef __GNUC__
#pragma implementation
#endif

#ifdef HAVE_CONFIG_H
#  include <scconfig.h>
#endif

#include <iostream>
#include <iomanip>

#include <util/misc/formio.h>
#include <util/group/pregtime.h>

using namespace std;

#define CLASSNAME ParallelRegionTimer
#define PARENTS public RegionTimer
#define HAVE_KEYVAL_CTOR
#include <util/class/classi.h>
void *
ParallelRegionTimer::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] =  RegionTimer::_castdown(cd);
  return do_castdowns(casts,cd);
}

ParallelRegionTimer::ParallelRegionTimer(const RefKeyVal &keyval):
  RegionTimer(keyval)
{
  msg_ = MessageGrp::get_default_messagegrp();
}

ParallelRegionTimer::ParallelRegionTimer(const RefMessageGrp&msg,
                                         const char *topname,
                                         int cpu_time, int wall_time):
  RegionTimer(topname, cpu_time, wall_time),
  msg_(msg)
{
}

ParallelRegionTimer::~ParallelRegionTimer()
{
}

void
ParallelRegionTimer::print(ostream &o) const
{
  int i,j;

  if (msg_->n() == 1) {
      RegionTimer::print(o);
      return;
    }

  update_top();

  int n = nregion();

  int minn = n;
  int maxn = n;
  msg_->max(maxn);
  msg_->min(minn);

  if (maxn != minn) {
      ExEnv::err() << node0
           << "ParallelRegionTimer::print: differing number of regions"
           << endl;
      abort();
    }

  double *cpu_time = 0;
  double *wall_time = 0;
  double *flops = 0;
  double *min_cpu_time = 0;
  double *min_wall_time = 0;
  double *min_flops = 0;
  double *max_cpu_time = 0;
  double *max_wall_time = 0;
  double *max_flops = 0;
  double *avg_cpu_time = 0;
  double *avg_wall_time = 0;
  double *avg_flops = 0;
  if (cpu_time_) {
      cpu_time = new double[n];
      get_cpu_times(cpu_time);
      min_cpu_time = new double[n];
      get_cpu_times(min_cpu_time);
      max_cpu_time = new double[n];
      get_cpu_times(max_cpu_time);
      avg_cpu_time = new double[n];
      get_cpu_times(avg_cpu_time);
      msg_->max(max_cpu_time,n);
      msg_->min(min_cpu_time,n);
      msg_->sum(avg_cpu_time,n);
      for (i=0; i<n; i++) {
          avg_cpu_time[i] /= msg_->n();
        }
    }
  if (wall_time_) {
      wall_time = new double[n];
      get_wall_times(wall_time);
      min_wall_time = new double[n];
      get_wall_times(min_wall_time);
      max_wall_time = new double[n];
      get_wall_times(max_wall_time);
      avg_wall_time = new double[n];
      get_wall_times(avg_wall_time);
      msg_->max(max_wall_time,n);
      msg_->min(min_wall_time,n);
      msg_->sum(avg_wall_time,n);
      for (i=0; i<n; i++) {
          avg_wall_time[i] /= msg_->n();
        }
    }
  const char *flops_name = 0;
  if (flops_) {
      flops= new double[n];
      get_flops(flops);
      if (cpu_time_) {
        for (i=0; i<n; i++) {
          if (fabs(cpu_time[i]) > 1.0e-10) flops[i] /= cpu_time[i]*1000000.;
          else flops[i] = 0.0;
          }
        flops_name = "MFLOP/S";
        }
      else if (wall_time_) {
        for (i=0; i<n; i++) {
          if (fabs(wall_time[i]) > 1.0e-10) flops[i] /= wall_time[i]*1000000.;
          else flops[i] = 0.0;
          }
        flops_name = "MFLOP/WS";
        }
      else {
        for (i=0; i<n; i++) {
          flops[i] /= 1000000.;
          }
        flops_name = "mflops";
        }
      min_flops= new double[n];
      memcpy(min_flops, flops, sizeof(double)*n);
      max_flops= new double[n];
      memcpy(max_flops, flops, sizeof(double)*n);
      avg_flops= new double[n];
      memcpy(avg_flops, flops, sizeof(double)*n);
      msg_->max(max_flops,n);
      msg_->min(min_flops,n);
      msg_->sum(avg_flops,n);
      for (i=0; i<n; i++) {
          avg_flops[i] /= msg_->n();
        }
    }

  if (msg_->me() == 0) {
      const char **names = new const char*[n];
      get_region_names(names);
      int *depth = new int[n];
      get_depth(depth);

      int maxwidth = 0;
      double maxtime = 0.0;
      for (i=0; i<n; i++) {
          int width = strlen(names[i]) + 2 * depth[i] + 2;
          if (width > maxwidth) maxwidth = width;
          if (cpu_time_ && max_cpu_time[i] > maxtime)
              maxtime = max_cpu_time[i];
          if (wall_time_ && max_wall_time[i] > maxtime)
              maxtime = max_wall_time[i];
          if (flops_ && max_flops[i] > maxtime)
              maxtime = max_flops[i];
        }

      int maxtimewidth = 4;
      while (maxtime >= 10.0) { maxtime/=10.0; maxtimewidth++; }

      o.setf(ios::right);

      for (i=0; i<maxwidth; i++) o << " ";
      if (cpu_time_) {
          o << setw(maxtimewidth+1) << " ";
          o << setw(maxtimewidth+1) << " CPU";
          o << setw(maxtimewidth+1) << " ";
        }
      if (wall_time_) {
          o << setw(maxtimewidth+1) << " ";
          o << setw(maxtimewidth+1) << " Wall";
          o << setw(maxtimewidth+1) << " ";
        }
      if (flops_) {
          o << setw(maxtimewidth+1) << " ";
          o << " " << setw(maxtimewidth+1) << flops_name;
          o << setw(maxtimewidth+1) << " ";
        }
      o << endl;

      for (i=0; i<maxwidth; i++) o << " ";
      if (cpu_time_) {
          o << setw(maxtimewidth+1) << " min";
          o << setw(maxtimewidth+1) << " max";
          o << setw(maxtimewidth+1) << " avg";
        }
      if (wall_time_) {
          o << setw(maxtimewidth+1) << " min";
          o << setw(maxtimewidth+1) << " max";
          o << setw(maxtimewidth+1) << " avg";
        }
      if (flops_) {
          o << setw(maxtimewidth+1) << " min";
          o << setw(maxtimewidth+1) << " max";
          o << setw(maxtimewidth+1) << " avg";
        }
      o << endl;

      o.setf(ios::fixed);
      o.precision(2);
      for (i=0; i<n; i++) {
          int width = strlen(names[i]) + 2 * depth[i] + 2;
          for (j=0; j<depth[i]; j++) o << "  ";
          o << names[i] << ": ";
          for (j=width; j<maxwidth; j++) o << " ";
          if (cpu_time_) {
              o << " " << setw(maxtimewidth) << min_cpu_time[i];
              o << " " << setw(maxtimewidth) << max_cpu_time[i];
              o << " " << setw(maxtimewidth) << avg_cpu_time[i];
            }                    
          if (wall_time_) {
              o << " " << setw(maxtimewidth) << min_wall_time[i];
              o << " " << setw(maxtimewidth) << max_wall_time[i];
              o << " " << setw(maxtimewidth) << avg_wall_time[i];
            }
          if (flops_) {
              o << " " << setw(maxtimewidth) << min_flops[i];
              o << " " << setw(maxtimewidth) << max_flops[i];
              o << " " << setw(maxtimewidth) << avg_flops[i];
            }
          o << endl;
        }

      delete[] names;
      delete[] depth;
    }

  delete[] cpu_time;
  delete[] min_cpu_time;
  delete[] max_cpu_time;
  delete[] avg_cpu_time;
  delete[] wall_time;
  delete[] min_wall_time;
  delete[] max_wall_time;
  delete[] avg_wall_time;
  delete[] flops;
  delete[] min_flops;
  delete[] max_flops;
  delete[] avg_flops;
}

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:
