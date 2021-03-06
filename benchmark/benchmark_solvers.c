/*
Original author: https://github.com/urbach/schwinger
Modified on 9/28/2018 by Yin Lin to compare performance to the 
benchmark_solvers.jl
*/

#include <complex.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#ifndef M_PI
#define M_PI    3.14159265358979323846f
#endif

/* From Julia microbench mark */
/* https://github.com/JuliaLang/Microbenchmarks/blob/master/perf.c */ 
double clock_now()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (double)now.tv_sec + (double)now.tv_usec/1.0e6;
}

/* Random number between 0 and 1 using stdlib */
/* For benchmark purpose only! */
double rand01(){
    return (double)rand() / (double)RAND_MAX;
}

/* Gaussian random number with zero mean and unit variance */
/* implemented with Box-Muller algorithm */
double gauss(){
    return (sqrt(-2.0*log(rand01()))*cos(2.0*M_PI*rand01()));
}

/* routine to initialise the geometry index routines */
/* takes care of the periodic boundary conditions    */
/* Also randomize the initial starting values */
int init_lattice_hot(const int NX, const int NY, 
                 int *left1, int *right1, int *left2, int *right2,
                 double *gauge1, double *gauge2, 
                 complex double *link1, complex double *link2) {
    int i;
    div_t dg;
    int N = NX*NY;

    left1[0]=NX-1;
    right1[0]=1;
    left2[0]=N-1;
    right2[0]=NX;

    for(i=0; i<N; i++) {
        /* div from stdlib for integer division                                  */
        /* div_t has two members, quot for the quotient and rem for the reminder */
        dg=div(i, NX);
        if(dg.rem==0) {
            left1[i]=i+NX-1;
            right1[i]=i+1;
        }
        else {
            if(dg.rem==(NX-1)) {
        left1[i]=i-1;
        right1[i]=i-NX+1;
            }
            else {
        left1[i]=i-1;
        right1[i]=i+1;
            }
        } 

        left2[i]=i-NX;
        if(left2[i]<0) {
            left2[i] += N;
        }

        right2[i]=i+NX;
        if(right2[i]>=N) {
            right2[i] -= N;
        }
    }
    /* Generate random starting lattice */
    for(i=0; i<N; i++){
        gauge1[i] = 2 * M_PI * rand01();
        gauge2[i] = 2 * M_PI * rand01();
        link1[i] = cos(gauge1[i]) + I*sin(gauge1[i]);
        link2[i] = cos(gauge2[i]) + I*sin(gauge2[i]);
    }
    return(0);
}

typedef struct spinor {
 complex double s1, s2;
} spinor;


void gamma5_Dslash_wilson(spinor *out, spinor *in, int NTOT, double mass,
                          int *left1, int *right1, int *left2, int *right2,
                          complex double *link1, complex double *link2) {
  int i;
  double g_R = 1;
  double factor = (2*g_R + mass);
  for(i=0; i<NTOT; i++) {
    complex double link1_i = link1[i];
    spinor in_i = in[i];
    int left1_i = left1[i];
    int left2_i = left2[i];
    int right1_i = right1[i];
    int right2_i = right2[i];
    spinor in_right1_i = in[right1_i];
    spinor in_right2_i = in[right2_i];
    spinor in_left1_i = in[left1_i];
    spinor in_left2_i = in[left2_i];
    complex double cconj_link1_left1_i = carg(link1[left1_i]);
    complex double cconj_link2_left2_i = carg(link2[left2_i]);
    out[i].s1 = factor * in_i.s1 - 
    0.5*(link1_i*(g_R*in_right1_i.s1 - in_right1_i.s2) +
         cconj_link1_left1_i * ( g_R*in_left1_i.s1  +   in_left1_i.s2)  +
         link2[i] * ( g_R*in_right2_i.s1 + I*in_right2_i.s2) +
         cconj_link2_left2_i * ( g_R*in_left2_i.s1  - I*in_left2_i.s2));
    
    out[i].s2 = - factor * in_i.s2 -
    0.5*(link1_i * (   in_right1_i.s1 - g_R*in_right1_i.s2) -
         cconj_link1_left1_i * (in_left1_i.s1  + g_R*in_left1_i.s2)  +
         link2[i] * ( I*in_right2_i.s1 - g_R*in_right2_i.s2) -
         cconj_link2_left2_i * (I*in_left2_i.s1  + g_R*in_left2_i.s2));
    /*printf("output (site %d) = (%.2f + i(%.2f), %.2f + i(%.2f))\n", i, creal(out[i].s1), 
        cimag(out[i].s1), creal(out[i].s2), cimag(out[i].s2));*/
  }
  return;
}

void set_zero(spinor *P, int NTOT)
{
    int i;
    for(i=0; i<NTOT; i++){
        P[i].s1 = 0.0 + I*0.0;
        P[i].s2 = 0.0 + I*0.0;
    };
}
void assign(spinor *R, spinor *S, int NTOT)
{
    int ix;
    spinor *r,*s;

    for (ix=0;ix<NTOT;ix++){
    r=(spinor *) R + ix;
    s=(spinor *) S + ix;

    (*r).s1=(*s).s1;

    (*r).s2=(*s).s2;
  }
}
double square_norm(spinor *S, int NTOT)
{
  int ix;
  static double ds;
  spinor *s;
  ds=0.0;
  for (ix=0;ix<NTOT;ix++)
  {
    s=(spinor *)S + ix;
    ds+=creal(conj((*s).s1)*(*s).s1+conj((*s).s2)*(*s).s2);
  }
  return ds;
}
double scalar_prod_r(spinor *S, spinor *R, int NTOT)
{
  int ix;
  static double ds;
  spinor *s,*r;
  
  /* Real Part */

  ds=0.0;
  
  for (ix=0;ix<NTOT;ix++){
    s=(spinor *) S + ix;
    r=(spinor *) R + ix;
    
    //ds+=conj((*s).s1)*(*r).s1+conj((*s).s2)*(*r).s2;
    ds+=creal(conj((*s).s1)*(*r).s1+conj((*s).s2)*(*r).s2);
  }

  return(ds);
}
void assign_add_mul_r(spinor *P, spinor *Q, double c, int NTOT)
{
  int ix;
  static double fact;

  fact=c;
   
  for (ix=0;ix<NTOT;ix++){

    P[ix].s1+=fact*Q[ix].s1;
    P[ix].s2+=fact*Q[ix].s2;
  }
}
void assign_mul_add_r(spinor *R, spinor *S, double c, int NTOT)
{
  int ix;
  static double fact;
  spinor *r,*s;
  
  fact=c;
  
  for (ix=0;ix<NTOT;ix++){
    r=(spinor *) R + ix;
    s=(spinor *) S + ix;
    
    (*r).s1=fact*(*r).s1+(*s).s1;
    (*r).s2=fact*(*r).s2+(*s).s2;
  }
}
#if 0
/* Solves the equation f*P = Q where f = gamma5_Dslash_wilson here */
int cg_Q(spinor *P, spinor *Q, int max_iter, double eps_sq, int NTOT, 
         double mass, int *left1, int *right1, int *left2, int *right2,
         complex double *link1, complex double *link2){

    double normsq, pro, err, alpha_cg, beta_cg;
    int iteration;
    spinor r[NTOT], p[NTOT];
    spinor x[NTOT], q2p[NTOT];
    spinor tmp1[NTOT];

    /* Initial guess for the solution - zero works well here */ 
    set_zero(x, NTOT);

    /* initialize residue r and search vector p */
    assign(r, Q, NTOT); /* r = Q - f*x, x=0 */
    assign(p, r, NTOT);
    normsq = square_norm(r, NTOT);
    
    double max_rel_err = eps_sq * square_norm(Q, NTOT);

    /* main loop */
    printf("\n\n Starting CG iterations...\n");
    for(iteration=0; iteration<max_iter; iteration++){
        gamma5_Dslash_wilson(q2p, p, NTOT, mass, left1, right1, left2,
                             right2, link1, link2);
        pro = scalar_prod_r(p, q2p, NTOT);
        /*  Compute alpha_cg(i+1)   */
        alpha_cg = normsq/pro;
        /*  Compute x_(i+1) = x_i + alpha_cg(i+1) p_i    */
        assign_add_mul_r(x, p,  alpha_cg, NTOT);
        /*  Compute r_(i+1) = r_i - alpha_cg(i+1) Q2p_i   */
        assign_add_mul_r(r, q2p, -alpha_cg, NTOT);
        /* Check whether the precision is reached ... */
        err=square_norm(r, NTOT);
        printf("\t CG iteration %i, |r|^2 = %2.2E\n", iteration, err);
        if(err <= max_rel_err)
        {
        printf("Required precision reached, stopping CG iterations...\n\n");
        assign(P, x, NTOT);
        return(iteration);
    };
        
    /* Compute beta_cg(i+1) */
    beta_cg = err/normsq;
    /* Compute p_(i+1) = r_i+1 + beta_(i+1) p_i     */
    assign_mul_add_r(p, r, beta_cg, NTOT);
    normsq = err;
    }
    fprintf(stderr, "WARNING: CG didn't converge after %d iterations!\n", max_iter);
    return (-1);
}
#endif

void print_sep(){
    printf("----------------------------------\n");
}
void print_lattice(int NX, int NT, double mass){
    print_sep();
    printf("Lattice Info:\n");
    printf("nx:         %d\n", NX);
    printf("nt:         %d\n", NT);
    printf("mass:       %.4f\n", mass);
    print_sep();
}


int main(){
    const int NX = 32;
    const int NT = 32;
    const double kappa = 0.22;
    const double mass = (pow(kappa, -1) - 4)/2;
    const int NTOT = NX * NT;
    const int bmiter = 10000; // Number of testing iterations

    /* Initilize lattice */
    int *leftx = (int *)malloc(NTOT*sizeof(int));
    int *rightx = (int *)malloc(NTOT*sizeof(int));
    int *downt = (int *)malloc(NTOT*sizeof(int));
    int *upt = (int *)malloc(NTOT*sizeof(int));

    double anglex[NTOT], anglet[NTOT];
    complex double linkx[NTOT], linkt[NTOT];
    init_lattice_hot(NX, NT, leftx, downt, rightx, upt,
                     anglex, anglet, linkx, linkt);

    /* Generate some random spinors */
    int i;
    spinor field_in[NTOT];
    spinor field_out[NTOT];
    for(i=0; i<NTOT; i++){
        field_in[i].s1 = gauss() + I*gauss();
        field_in[i].s2 = gauss() + I*gauss();
        field_out[i].s1 = gauss() + I*gauss();
        field_out[i].s2 = gauss() + I*gauss();
    }
    /* Main testing loop */
    print_lattice(NX, NT, mass);
    printf("Benchmark: gamma5_Dslash_wilson\n");
    double t;
    t = clock_now();
    for(i=0; i<bmiter; i++){
        gamma5_Dslash_wilson(field_out, field_in, NTOT, mass, 
                             leftx, rightx, downt, upt,
                             linkx, linkt);
    }
    t = clock_now() - t;
    printf("--> Total excution time (%d iterations): %.8f seconds\n", bmiter, t);
    print_sep();
    printf("Benchmark: cg_Q\n");
    int max_iter = 1e5;
    double eps_sq = 1e-16;

    /* Make some source at t0 = 0 */
    spinor wallsource[NTOT];
    int t0 = 0;
    for(i=0; i<NTOT; i++){
        wallsource[i].s1;
    }
    /* not yet implemented
    cg_Q(P, Q, max_iter, eps_sq, NTOT, 
         mass, leftx, rightx, downt, upt,
         linkx, linkt);
    */
}