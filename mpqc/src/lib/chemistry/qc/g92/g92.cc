
#ifdef __GNUC__
#pragma implementation
#endif

#include "g92.h"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
};

#include <math/scmat/local.h>

////////////////////////////////////////////////////////////////////////////

static int
find_line(FILE *g92log, char *line, int len, const char *string)
{
  while(fgets(line,len,g92log)) {
    if (strstr(line,string))
      break;
  }

  if (feof(g92log)) {
    fprintf(stderr,"Gaussian92::find_line: hit end of file\n");
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////

#define CLASSNAME Gaussian92
#define PARENTS public OneBodyWavefunction
#include <util/state/statei.h>
#include <util/class/classia.h>
void *
Gaussian92::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = OneBodyWavefunction::_castdown(cd);
  return do_castdowns(casts,cd);
}

Gaussian92::Gaussian92(const RefKeyVal&keyval):
  OneBodyWavefunction(keyval),
  _eigenvectors(this)
{
  // override the default thresholds
  if (!keyval->exists("value_accuracy")) {
    set_desired_value_accuracy(1.0e-10);
  }
  if (!keyval->exists("gradient_accuracy")) {
    set_desired_gradient_accuracy(1.0e-9);
  }
  if (!keyval->exists("hessian_accuracy")) {
    set_desired_hessian_accuracy(1.0e-8);
  }

  // read input, and initialize various structs
  multiplicity_ = keyval->intvalue("multiplicity");
  if (keyval->error() != KeyVal::OK) multiplicity_ = 1;
  
  charge_ = keyval->intvalue("charge");

  memory_ = keyval->intvalue("memory");
  if (keyval->error() != KeyVal::OK) memory_ = 4000000;

  use_ckpt_ = keyval->booleanvalue("use_checkpoint_guess");
  if (keyval->error() != KeyVal::OK) use_ckpt_ = 1;

  name_ = keyval->pcharvalue("filename");
  if (keyval->error() != KeyVal::OK) {
    name_ = new char[8];
    strcpy(name_,"g92file");
  }

  scr_dir_ = keyval->pcharvalue("scratch_dir");
  g92_dir_ = keyval->pcharvalue("g92_dir");
  if (keyval->error() != KeyVal::OK) {
    fprintf(stderr,"Gaussian92: g92_dir is not set\n");
    abort();
  }
  basis_ = keyval->pcharvalue("g92basis");
}

Gaussian92::~Gaussian92()
{
  if (name_) delete[] name_;
  if (scr_dir_) delete[] scr_dir_;
  if (g92_dir_) delete[] g92_dir_;
  if (basis_) delete[] basis_;
  name_ = scr_dir_ = g92_dir_ = basis_ =0;
}

Gaussian92::Gaussian92(StateIn&s):
  OneBodyWavefunction(s),
  _eigenvectors(this)
  maybe_SavableState(s)
{
  s.get(charge_);
  s.get(multiplicity_);
  s.get(memory_);
  s.get(use_ckpt_);
  s.getstring(name_);
  s.getstring(scr_dir_);
  s.getstring(g92_dir_);
  s.getstring(basis_);
}

void
Gaussian92::save_data_state(StateOut&s)
{
  OneBodyWavefunction::save_data_state(s);
  s.put(charge_);
  s.put(multiplicity_);
  s.put(memory_);
  s.put(use_ckpt_);
  s.putstring(name_);
  s.putstring(scr_dir_);
  s.putstring(g92_dir_);
  s.putstring(basis_);
}

////////////////////////////////////////////////////////////////////////////

void
Gaussian92::compute()
{
  int i,j;

  // print the geometry every iteration
  printf("\n  molecular geometry in Gaussian92::compute()\n");
  fflush(stdout);
  _mol->print();

  // Adjust the value accuracy if gradients are needed and set up
  // minimal accuracies.
  if (desired_value_accuracy() > 1.0e-4)
    set_desired_value_accuracy(1.0e-4);

  if (_gradient.compute()) {
    if (desired_gradient_accuracy() > 1.0e-4)
      set_desired_gradient_accuracy(1.0e-4);
    if (desired_value_accuracy() > 0.1 * desired_gradient_accuracy()) {
      set_desired_value_accuracy(0.1 * desired_gradient_accuracy());
    }
  }

  if (_hessian.compute()) {
    if (desired_hessian_accuracy() > 1.0e-4)
      set_desired_hessian_accuracy(1.0e-4);
    
    if (desired_value_accuracy() > 0.01 * desired_hessian_accuracy()) {
      set_desired_value_accuracy(0.01 * desired_hessian_accuracy());
    }
  }

  if (_hessian.needed()) {
    if (run_hessian() < 0) {
      fprintf(stderr,"Gaussian92::compute: run_hessian did not succeed\n");
      abort();
    }
  } else if (_gradient.needed()) {
    if (run_gradient() < 0) {
      fprintf(stderr,"Gaussian92::compute: run_gradient did not succeed\n");
      abort();
    }
  } else if (_energy.needed()) {
    if (run_energy() < 0) {
      fprintf(stderr,"Gaussian92::compute: run_energy did not succeed\n");
      abort();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

int
Gaussian92::run_energy()
{
  if (run_g92(emethod()) < 0) {
    fprintf(stderr,"Gaussian92::run_energy: run_g92 did not succeed\n");
    return -1;
  }
  
  return parse_g92_energy();
}

int
Gaussian92::parse_g92_energy()
{
  FILE *g92log = fopen("Test.FChk","r");
  if (!g92log) {
    fprintf(stderr,"Gaussian92::parse_g92_energy: could not open log file\n");
    return -1;
  }
  
  char line[122];
  if (find_line(g92log,line,120,"Total Energy") < 0) {
    fprintf(stderr,"Gaussian92::parse_g92_energy:"
            " Could not find energy in G92 output\n");
    fclose(g92log);
    return -1;
  }

  fclose(g92log);

  double energy;
  char *tok = strtok(line,"R");
  tok = strtok(0," ");
  if (!tok) {
    fprintf(stderr,
            "Gaussian92::parse_g92_energy: trouble scanning energy line\n");
    return -1;
  }

  energy = atof(tok);
  printf("\n  Gaussian92 energy = %20.10f\n",energy);
  fflush(stdout);
  
  set_energy(energy);
  _energy.set_actual_accuracy(_energy.desired_accuracy());
  
  return 0;
}

//////////////////////////////////////////////////////////////////////////

int
Gaussian92::run_gradient()
{
  if (run_g92(gmethod()) < 0) {
    fprintf(stderr,"Gaussian92::run_gradient: run_g92 did not succeed\n");
    return -1;
  }
  
  return parse_g92_gradient();
}

int
Gaussian92::parse_g92_gradient()
{
  if (parse_g92_energy() < 0) {
    fprintf(stderr,"Gaussian92::parse_g92_gradient:"
            " Could not parse energy\n");
    return -1;
  }

  FILE *g92log = fopen("Test.FChk","r");
  if (!g92log) {
    fprintf(stderr,
            "Gaussian92::parse_g92_gradient: Could not open log file\n");
    return -1;
  }

  // find the forces in the log file
  char line[122];
  
  if (find_line(g92log,line,120,"Cartesian Forces") < 0) {
    fprintf(stderr,"Gaussian92::parse_g92_gradient:"
            " Could not find gradient in G92 output\n");
    fclose(g92log);
    return -1;
  }

  RefSCVector gradient(_mol->dim_natom3());
  
  // and now read in the forces
  double x,y,z;
  
  printf("\n  Gaussian92 gradient:\n");
  
  for (int i=0; i < _mol->natom(); i++) {
    fscanf(g92log,"%lf %lf %lf",&x,&y,&z);
    gradient.set_element(i*3,x);
    gradient.set_element(i*3+1,y);
    gradient.set_element(i*3+2,z);
    printf("%5d %14.10f %14.10f %14.10f\n",i+1,x,y,z);
  }
  fflush(stdout);

  fclose(g92log);
  set_gradient(gradient);
  _gradient.set_actual_accuracy(desired_gradient_accuracy());

  return 0;
}

//////////////////////////////////////////////////////////////////////////

int
Gaussian92::run_hessian()
{
  if (run_g92(hmethod()) < 0) {
    fprintf(stderr,"Gaussian92::run_hessian: run_g92 did not succeed\n");
    return -1;
  }
  
  return parse_g92_hessian();
}
  
int
Gaussian92::parse_g92_hessian()
{
  if (parse_g92_gradient() < 0) {
    fprintf(stderr,"Gaussian92::parse_g92_hessian:"
            " Could not parse gradient\n");
    return -1;
  }

  FILE *g92log = fopen("Test.FChk","r");
  if (!g92log) {
    fprintf(stderr,"Gaussian92::parse_g92_hessian: Could not open log file\n");
    return -1;
  }

  // find the force constants in the log file
  char line[122];
  
  if (find_line(g92log,line,120,"Cartesian Force Constants") < 0) {
    fprintf(stderr,"Gaussian92::parse_g92_hessian:"
            " Could not find hessian in G92 output\n");
    fclose(g92log);
    return -1;
  }

  RefSymmSCMatrix hessian(_mol->dim_natom3());
  
  // read in force constants
  for (int i=0; i < hessian->n(); i++) {
    for (int j=0; j <= i; j++) {
      double x;
      fscanf(g92log,"%lf",&x);
      hessian.set_element(i,j,x);
    }
  }

  fclose(g92log);

  hessian.print("  G92 force constants");
  fflush(stdout);
  cout.flush();
  set_hessian(hessian);
  _hessian.set_actual_accuracy(desired_hessian_accuracy());
  
  return 0;
}

////////////////////////////////////////////////////////////////////////////

int
Gaussian92::run_g92(const char *method)
{
  char *comfile = new char[strlen(name_)+5];
  sprintf(comfile,"%s.com",name_);

  char *logfile = new char[strlen(name_)+9];
  sprintf(logfile,"%s.g92.out",name_);

  // open the G92 input file
  FILE *g92com = fopen(comfile,"w");
  if (!g92com) {
    fprintf(stderr,"run_g92: could not open comfile %s\n",comfile);
    return -1;
  }
  
  // Write out required headers
  fprintf(g92com,"%%chk=%s\n",name_);
  fprintf(g92com,"%%mem=%d\n",memory_);
  fprintf(g92com,"#p FChk=all units=au nosymm %s\n",method);
  
  if (basis_)
    fprintf(g92com,"%s\n",basis_);

  // Request to use guess from checkpoint file
  if (use_ckpt_) {
    char *tmp = new char[5+strlen(name_)];
    sprintf(tmp,"%s.chk",name_);
    
    struct stat sb;
    if (stat(tmp,&sb)==0 && sb.st_size)
      fprintf(g92com,"guess=check\n");
    
    delete[] tmp;
  }

  fprintf(g92com,"\nG92 input generated by Gaussian92:run_g92()\n");
  fprintf(g92com,"\n %d %d\n", charge_, multiplicity_);
  for (int i=0; i < _mol->natom(); i++) {
    fprintf(g92com,"%s %lf %lf %lf\n",
            _mol->atom(i).element().symbol(),
            _mol->atom(i).operator[](0), _mol->atom(i).operator[](1),
            _mol->atom(i).operator[](2));
  }
  fprintf(g92com,"\n");
  fclose(g92com);

  // Set environmental variable necessary for g92 run
  // Do this only once, or else the execution shell loses it
  static env_var_set=0;
  if (!env_var_set) {
    env_var_set=1;
    static char *g92_env_str;
    if (g92_dir_) {
      g92_env_str=(char*) malloc(sizeof(char)*(14+strlen(g92_dir_)));
      sprintf(g92_env_str,"GAUSS_EXEDIR=%s",g92_dir_);
      putenv(g92_env_str);
    }
    
    static char *g92_scr_str;
    if (scr_dir_) {
      g92_scr_str=(char*) malloc(sizeof(char)*(16+strlen(scr_dir_)));
      sprintf(g92_scr_str,"GAUSS_SCRDIR=%s",scr_dir_);
    } else {
      g92_scr_str="GAUSS_SCRDIR=./";
    }
    putenv(g92_scr_str);
  }

  // assemble and execute g92 command
  char *commandstr =
    new char[strlen(comfile)+strlen(g92_dir_)+strlen(logfile)+10];
  sprintf(commandstr,"%sg92 < %s > %s",g92_dir_, comfile, logfile);

  int ret = system(commandstr);
    
  // Free arrays
  delete[] commandstr;
  delete[] comfile;
  delete[] logfile;

  return ret;
}

///////////////////////////////////////////////////////////////////////////

int
Gaussian92::do_eigenvectors(int f)
{
  int old = _eigenvectors.compute();
  _eigenvectors.compute() = f;
  return old;
}

RefSCMatrix
Gaussian92::eigenvectors()
{
  if (!_energy.computed()) {
    run_energy();
  }
  return _eigenvectors;
}

void
Gaussian92::print(SCostream&o)
{
  o.flush();
  OneBodyWavefunction::print(o);
  o.flush();
}

////////////////////////////////////////////////////////////////////////////

static FILE *
open_log(const char *name_)
{
  char * logfile = new char[strlen(name_)+9];
  sprintf(logfile,"%s.g92.out",name_);
  
  FILE *g92log = fopen(logfile,"r");
  if (!g92log)
    fprintf(stderr,"open_log: could not open output file %s\n",logfile);

  delete[] logfile;
  return g92log;
}

RefSCMatrix
Gaussian92::normal_modes()
{
  if (!_hessian.computed())
    run_hessian();

  RefSCDimension nnorm = new LocalSCDimension(_moldim.n()-6);
  RefSCMatrix normalmodes(_moldim,nnorm);
  normalmodes.assign(0.0);

  FILE *g92log = open_log(name_);
  if (!g92log) {
    fprintf(stderr,"Gaussian92::normal_modes: Could not open log file\n");
    return normalmodes;
  }

  // find the frequencies in the log file
  char line[122];
  char *fstring = "Depolarizations ---";

  int npara = nnorm.n()/5;
  if (nnorm.n()%5) npara++;

  int j=0, jj=0;
  for (int np=0; np < npara; np++) {
    if (find_line(g92log,line,120,fstring) < 0) {
      fprintf(stderr,"Gaussian92::normal_modes:"
              " Could not find frequencies in G92 output\n");
      fclose(g92log);
      return normalmodes;
    }
    // eat the next line
    fgets(line,120,g92log);

    for (int i=0; i < _moldim.n(); i++) {
      fgets(line,120,g92log);
      char *tok = strtok(line," "); // coord
      tok = strtok(0," ");          // atom
      tok = strtok(0," ");          // element

      j=jj;
      while (tok=strtok(0," ")) {
        double f=atof(tok);
        normalmodes.set_element(i,j,f);
        j++;
      }
    }
    jj=j;
  }

  fclose(g92log);

  return normalmodes;
}

RefSCVector
Gaussian92::frequencies()
{
  if (!_hessian.computed())
    run_hessian();

  RefSCDimension nfreq = new LocalSCDimension(_moldim.n()-6);
  RefSCVector freq(nfreq);
  freq.assign(0.0);

  FILE *g92log = open_log(name_);
  if (!g92log) {
    fprintf(stderr,"Gaussian92::frequencies: Could not open log file\n");
    return freq;
  }

  // find the frequencies in the log file
  char line[122];
  char *fstring = "Frequencies ---";

  int npara = nfreq.n()/5;
  if (nfreq.n()%5) npara++;

  int i=0;
  for (int np=0; np < npara; np++) {
    if (find_line(g92log,line,120,fstring) < 0) {
      fprintf(stderr,"Gaussian92::frequencies:"
              " Could not find frequencies in G92 output\n");
      fclose(g92log);
      return freq;
    }
    char *tok = strtok(line,"-");
    strtok(0," "); // eat rest of -'s
    while (tok=strtok(0," ")) {
      double f=atof(tok);
      freq.set_element(i,f);
      i++;
    }
  }

  fclose(g92log);
  
  return freq;
}
