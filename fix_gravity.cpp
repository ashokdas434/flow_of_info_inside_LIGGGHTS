/* ----------------------------------------------------------------------
    This is the

    ██╗     ██╗ ██████╗  ██████╗  ██████╗ ██╗  ██╗████████╗███████╗
    ██║     ██║██╔════╝ ██╔════╝ ██╔════╝ ██║  ██║╚══██╔══╝██╔════╝
    ██║     ██║██║  ███╗██║  ███╗██║  ███╗███████║   ██║   ███████╗
    ██║     ██║██║   ██║██║   ██║██║   ██║██╔══██║   ██║   ╚════██║
    ███████╗██║╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║   ██║   ███████║
    ╚══════╝╚═╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚══════╝®

    DEM simulation engine, released by
    DCS Computing Gmbh, Linz, Austria
    http://www.dcs-computing.com, office@dcs-computing.com

    LIGGGHTS® is part of CFDEM®project:
    http://www.liggghts.com | http://www.cfdem.com

    Core developer and main author:
    Christoph Kloss, christoph.kloss@dcs-computing.com

    LIGGGHTS® is open-source, distributed under the terms of the GNU Public
    License, version 2 or later. It is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. You should have
    received a copy of the GNU General Public License along with LIGGGHTS®.
    If not, see http://www.gnu.org/licenses . See also top-level README
    and LICENSE files.

    LIGGGHTS® and CFDEM® are registered trade marks of DCS Computing GmbH,
    the producer of the LIGGGHTS® software and the CFDEM®coupling software
    See http://www.cfdem.com/terms-trademark-policy for details.

-------------------------------------------------------------------------
    Contributing author and copyright for this file:
    This file is from LAMMPS, but has been modified. Copyright for
    modification:

    Copyright 2012-     DCS Computing GmbH, Linz
    Copyright 2009-2012 JKU Linz

    Copyright of original file:
    LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
    http://lammps.sandia.gov, Sandia National Laboratories
    Steve Plimpton, sjplimp@sandia.gov

    Copyright (2003) Sandia Corporation.  Under the terms of Contract
    DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
    certain rights in this software.  This software is distributed under
    the GNU General Public License.
------------------------------------------------------------------------- */

#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fix_gravity.h"
#include "atom.h"
#include "update.h"
#include "domain.h"
#include "respa.h"
#include "modify.h"
#include "input.h"
#include "variable.h"
#include "math_const.h"
#include "fix_multisphere.h"  
#include "fix_relax_contacts.h"  
#include "error.h"
#include "force.h"

using namespace LAMMPS_NS;
using namespace FixConst;
using namespace MathConst;

enum{CHUTE,SPHERICAL,VECTOR};
enum{CONSTANT,EQUAL};

/* ---------------------------------------------------------------------- */

FixGravity::FixGravity(LAMMPS *lmp, int narg, char **arg) :
  Fix(lmp, narg, arg)
{
  if (narg < 5) error->all(FLERR,"Illegal fix gravity command");

  scalar_flag = 1;
  global_freq = 1;
  extscalar = 1;

  mstr = vstr = pstr = tstr = xstr = ystr = zstr = NULL;
  mstyle = vstyle = pstyle = tstyle = xstyle = ystyle = zstyle = CONSTANT;

  if (strstr(arg[3],"v_") == arg[3]) {
    int n = strlen(&arg[3][2]) + 1;
    mstr = new char[n];
    strcpy(mstr,&arg[3][2]);
    mstyle = EQUAL;
  } else {
    magnitude = force->numeric(FLERR,arg[3]);
    mstyle = CONSTANT;
  }

  if (strcmp(arg[4],"chute") == 0) {
    if (narg != 6) error->all(FLERR,"Illegal fix gravity command");
    style = CHUTE;
    if (strstr(arg[5],"v_") == arg[5]) {
      int n = strlen(&arg[5][2]) + 1;
      vstr = new char[n];
      strcpy(vstr,&arg[5][2]);
      vstyle = EQUAL;
    } else {
      vert = force->numeric(FLERR,arg[5]);
      vstyle = CONSTANT;
    }

  } else if (strcmp(arg[4],"spherical") == 0) {
    if (narg != 7) error->all(FLERR,"Illegal fix gravity command");
    style = SPHERICAL;
    if (strstr(arg[5],"v_") == arg[5]) {
      int n = strlen(&arg[5][2]) + 1;
      pstr = new char[n];
      strcpy(pstr,&arg[5][2]);
      pstyle = EQUAL;
    } else {
      phi = force->numeric(FLERR,arg[5]);
      pstyle = CONSTANT;
    }
    if (strstr(arg[6],"v_") == arg[6]) {
      int n = strlen(&arg[6][2]) + 1;
      tstr = new char[n];
      strcpy(tstr,&arg[6][2]);
      tstyle = EQUAL;
    } else {
      theta = force->numeric(FLERR,arg[6]);
      tstyle = CONSTANT;
    }

  } else if (strcmp(arg[4],"vector") == 0) {
    if (narg != 8) error->all(FLERR,"Illegal fix gravity command");
    style = VECTOR;
    if (strstr(arg[5],"v_") == arg[5]) {
      int n = strlen(&arg[5][2]) + 1;
      xstr = new char[n];
      strcpy(xstr,&arg[5][2]);
      xstyle = EQUAL;
    } else {
      xdir = force->numeric(FLERR,arg[5]);
      xstyle = CONSTANT;
    }
    if (strstr(arg[6],"v_") == arg[6]) {
      int n = strlen(&arg[6][2]) + 1;
      ystr = new char[n];
      strcpy(ystr,&arg[6][2]);
      ystyle = EQUAL;
    } else {
      ydir = force->numeric(FLERR,arg[6]);
      ystyle = CONSTANT;
    }
    if (strstr(arg[7],"v_") == arg[7]) {
      int n = strlen(&arg[7][2]) + 1;
      zstr = new char[n];
      strcpy(zstr,&arg[7][2]);
      zstyle = EQUAL;
    } else {
      zdir = force->numeric(FLERR,arg[7]);
      zstyle = CONSTANT;
    }

  } else error->all(FLERR,"Illegal fix gravity command");

  degree2rad = MY_PI/180.0;
  time_origin = update->ntimestep;

  eflag = 0;
  egrav = 0.0;

  fm = NULL; 
fprintf(screen,"Fix_gravity is being used \n");
}

/* ---------------------------------------------------------------------- */

FixGravity::~FixGravity()
{
  delete [] mstr;
  delete [] vstr;
  delete [] pstr;
  delete [] tstr;
  delete [] xstr;
  delete [] ystr;
  delete [] zstr;
}

/* ---------------------------------------------------------------------- */

int FixGravity::setmask()
{
printf("================= JCPRINT: FixGravity::setmask is being called ======\n");
  int mask = 0;
  mask |= POST_FORCE;
  mask |= THERMO_ENERGY;
  mask |= POST_FORCE_RESPA;
  return mask;
}

/* ---------------------------------------------------------------------- */

void FixGravity::init()
{
printf("================= JCPRINT: FixGravity::init is being called ======\n");
  if (strstr(update->integrate_style,"respa"))
    nlevels_respa = ((Respa *) update->integrate)->nlevels;

  // check variables

  if (mstr) {
    mvar = input->variable->find(mstr);
    if (mvar < 0)
      error->all(FLERR,"Variable name for fix gravity does not exist");
    if (!input->variable->equalstyle(mvar))
      error->all(FLERR,"Variable for fix gravity is invalid style");
  }
  if (vstr) {
    vvar = input->variable->find(vstr);
    if (vvar < 0)
      error->all(FLERR,"Variable name for fix gravity does not exist");
    if (!input->variable->equalstyle(vvar))
      error->all(FLERR,"Variable for fix gravity is invalid style");
  }
  if (pstr) {
    pvar = input->variable->find(pstr);
    if (pvar < 0)
      error->all(FLERR,"Variable name for fix gravity does not exist");
    if (!input->variable->equalstyle(pvar))
      error->all(FLERR,"Variable for fix gravity is invalid style");
  }
  if (tstr) {
    tvar = input->variable->find(tstr);
    if (tvar < 0)
      error->all(FLERR,"Variable name for fix gravity does not exist");
    if (!input->variable->equalstyle(tvar))
      error->all(FLERR,"Variable for fix gravity is invalid style");
  }
  if (xstr) {
    xvar = input->variable->find(xstr);
    if (xvar < 0)
      error->all(FLERR,"Variable name for fix gravity does not exist");
    if (!input->variable->equalstyle(xvar))
      error->all(FLERR,"Variable for fix gravity is invalid style");
  }
  if (ystr) {
    yvar = input->variable->find(ystr);
    if (yvar < 0)
      error->all(FLERR,"Variable name for fix gravity does not exist");
    if (!input->variable->equalstyle(yvar))
      error->all(FLERR,"Variable for fix gravity is invalid style");
  }
  if (zstr) {
    zvar = input->variable->find(zstr);
    if (zvar < 0)
      error->all(FLERR,"Variable name for fix gravity does not exist");
    if (!input->variable->equalstyle(zvar))
      error->all(FLERR,"Variable for fix gravity is invalid style");
  }

  varflag = CONSTANT;
  if (mstyle != CONSTANT || vstyle != CONSTANT || pstyle != CONSTANT ||
      tstyle != CONSTANT || xstyle != CONSTANT || ystyle != CONSTANT ||
      zstyle != CONSTANT) varflag = EQUAL;

  // set gravity components once and for all

  if (varflag == CONSTANT) set_acceleration();

  fm = NULL;
  int nms = modify->n_fixes_style("multisphere");
  if(nms > 1)
    error->fix_error(FLERR,this,"support for more than one fix multisphere not implemented");
  if(nms)
    fm = static_cast<FixMultisphere*>(modify->find_fix_style("multisphere",0));

  int n_relax = modify->n_fixes_style("relax");
  if(n_relax > 1)
        error->fix_error(FLERR,this,"does not work with more than 1 fix relax");
  else if(1 == n_relax)
        fix_relax = static_cast<FixRelaxContacts*>(modify->find_fix_style("relax",0));
  else
        fix_relax = 0;
}

/* ---------------------------------------------------------------------- */

void FixGravity::setup(int vflag)
{
printf("================= JCPRINT: FixGravity::setup is being called ======\n");
  if (strstr(update->integrate_style,"verlet"))
    post_force(vflag);
  else {
    ((Respa *) update->integrate)->copy_flevel_f(nlevels_respa-1);
    post_force_respa(vflag,nlevels_respa-1,0);
    ((Respa *) update->integrate)->copy_f_flevel(nlevels_respa-1);
  }
}

/* ---------------------------------------------------------------------- */

void FixGravity::post_force(int vflag)
{
printf("================= JCPRINT: FixGravity::post_force is being called ======\n");
  // update gravity due to variables

  if (varflag != CONSTANT) {
    modify->clearstep_compute();
    if (mstyle == EQUAL) magnitude = input->variable->compute_equal(mvar);
    if (vstyle == EQUAL) vert = input->variable->compute_equal(vvar);
    if (pstyle == EQUAL) phi = input->variable->compute_equal(pvar);
    if (tstyle == EQUAL) theta = input->variable->compute_equal(tvar);
    if (xstyle == EQUAL) xdir = input->variable->compute_equal(xvar);
    if (ystyle == EQUAL) ydir = input->variable->compute_equal(yvar);
    if (zstyle == EQUAL) zdir = input->variable->compute_equal(zvar);
    modify->addstep_compute(update->ntimestep + 1);

    set_acceleration();
  }

  double **x = atom->x;
  double **f = atom->f;
  double *rmass = atom->rmass;
  double *mass = atom->mass;
  int *mask = atom->mask;
  int *type = atom->type;
  int nlocal = atom->nlocal;
  double massone;

  eflag = 0;
  egrav = 0.0;

  if (rmass) {
    for (int i = 0; i < nlocal; i++)
      if ((mask[i] & groupbit) && (!fm || (fm && fm->belongs_to(i) < 0))) { 
        massone = rmass[i] * (fix_relax ? (fix_relax->factor_relax(i)) : 1.);
        f[i][0] += massone*xacc;
        f[i][1] += massone*yacc;
        f[i][2] += massone*zacc;
        egrav -= massone * (xacc*x[i][0] + yacc*x[i][1] + zacc*x[i][2]);
      }
  } else {
    for (int i = 0; i < nlocal; i++)
      if ((mask[i] & groupbit) && (!fm || (fm && fm->belongs_to(i) < 0))) { 
        massone = mass[type[i]];
        f[i][0] += massone*xacc;
        f[i][1] += massone*yacc;
        f[i][2] += massone*zacc;
        egrav -= massone * (xacc*x[i][0] + yacc*x[i][1] + zacc*x[i][2]);
      }
  }
}

/* ---------------------------------------------------------------------- */

void FixGravity::post_force_respa(int vflag, int ilevel, int iloop)
{
printf("================= JCPRINT: FixGravity::post_force_respa is being called ======\n");
  if (ilevel == nlevels_respa-1) post_force(vflag);
}

/* ---------------------------------------------------------------------- */

void FixGravity::set_acceleration()
{
printf("================= JCPRINT: FixGravity::set_acceleration is being called ======\n");
  if (style == CHUTE || style == SPHERICAL) {
    if (style == CHUTE) {
      phi = 0.0;
      theta = 180.0 - vert;
    }
    if (domain->dimension == 3) {
      xgrav = sin(degree2rad * theta) * cos(degree2rad * phi);
      ygrav = sin(degree2rad * theta) * sin(degree2rad * phi);
      zgrav = cos(degree2rad * theta);
    } else {
      xgrav = sin(degree2rad * theta);
      ygrav = cos(degree2rad * theta);
      zgrav = 0.0;
    }
  } else if (style == VECTOR) {
    if (domain->dimension == 3) {
      double length = sqrt(xdir*xdir + ydir*ydir + zdir*zdir);
      if(length == 0.)
        error->one(FLERR,"Gravity direction vector = 0");
      xgrav = xdir/length;
      ygrav = ydir/length;
      zgrav = zdir/length;
    } else {
      double length = sqrt(xdir*xdir + ydir*ydir);
      if(length == 0.)
        error->one(FLERR,"Gravity direction vector = 0");
      xgrav = xdir/length;
      ygrav = ydir/length;
      zgrav = 0.0;
    }
  }

  xacc = magnitude*xgrav;
  yacc = magnitude*ygrav;
  zacc = magnitude*zgrav;
}

/* ----------------------------------------------------------------------
   potential energy in gravity field
------------------------------------------------------------------------- */

double FixGravity::compute_scalar()
{
printf("================= JCPRINT: FixGravity::compute_scalar is being called ======\n");
  // only sum across procs one time

  if (eflag == 0) {
    MPI_Allreduce(&egrav,&egrav_all,1,MPI_DOUBLE,MPI_SUM,world);
    eflag = 1;
  }
  return egrav_all;
}

/* ---------------------------------------------------------------------- */

void FixGravity::get_gravity(double *grav)
{
printf("================= JCPRINT: FixGravity::get_gravity is being called ======\n");
    grav[0] = xgrav * magnitude;
    grav[1] = ygrav * magnitude;
    grav[2] = zgrav * magnitude;
}
