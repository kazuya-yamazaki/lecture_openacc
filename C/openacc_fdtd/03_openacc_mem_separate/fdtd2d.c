/**
 * @file fdtd2d.c
 * @brief (File brief)
 *
 * (File explanation)
 *
 * @author Takashi Shimokawabe
 * @date 2017/11/10 Created
 * @version 0.1.0
 *
 * $Header$ 
 */

#include "fdtd2d.h"

void calc_ex_ey(const struct Range *whole, const struct Range *inside,
                const FLOAT *hz, const FLOAT *cexly, const FLOAT *ceylx, FLOAT *ex, FLOAT *ey)
{
    const int nx    = inside->length[0];
    const int ny    = inside->length[1];
    const int mgn[] = { inside->begin[0] - whole->begin[0],
                        inside->begin[1] - whole->begin[1] };

#pragma acc kernels present(ex, cexly, hz)
#pragma acc loop independent
    for (int j=0; j<ny+1; j++) {
#pragma acc loop independent
        for (int i=0; i<nx; i++) {
            const int ix = (j+mgn[1])*whole->length[0] + i+mgn[0];
            const int jm = ix - whole->length[0];
            ex[ix] += cexly[ix]*(hz[ix]-hz[jm]);
        }
    }
    
#pragma acc kernels present(ey, ceylx, hz)
#pragma acc loop independent
    for (int j=0; j<ny; j++) {
#pragma acc loop independent
        for (int i=0; i<nx+1; i++) {
            const int ix = (j+mgn[1])*whole->length[0] + i+mgn[0];
            const int im = ix - 1;
            ey[ix] += - ceylx[ix]*(hz[ix]-hz[im]);
        }
    }
}

void calc_hz(const struct Range *whole, const struct Range *inside,
             const FLOAT *ey, const FLOAT *ex, const FLOAT *chzlx, const FLOAT *chzly, FLOAT *hz)
{
    const int nx    = inside->length[0];
    const int ny    = inside->length[1];
    const int mgn[] = { inside->begin[0] - whole->begin[0],
                        inside->begin[1] - whole->begin[1] };

#pragma acc kernels present(hz, chzlx, ey, chzly, ex)
#pragma acc loop independent
    for (int j=0; j<ny; j++) {
#pragma acc loop independent
        for (int i=0; i<nx; i++) {
            const int ix = (j+mgn[1])*whole->length[0] + i+mgn[0];
            const int ip = ix + 1;
            const int jp = ix + whole->length[0];
            hz[ix] += - chzlx[ix]*(ey[ip]-ey[ix]) + chzly[ix]*(ex[jp]-ex[ix]);
        }
    }
}

void pml_boundary_ex(const struct Range *whole, const struct Range *inside,
                     const FLOAT *hz, const FLOAT *cexy, const FLOAT *cexyl, const FLOAT *rer_ex,
                     FLOAT *ex, FLOAT *exy)
{
    const int bi[] = { inside->begin[0], inside->begin[1] };
    const int ei[] = { inside->begin[0] + inside->length[0], inside->begin[1] + inside->length[1] };
    const int bw[] = { whole->begin[0], whole->begin[1] };
    const int ew[] = { whole->begin[0] + whole->length[0], whole->begin[1] + whole->length[1] };

    const int r[4][4] = { { bw[0], ew[0], bw[1]+1, bi[1] },
                          { bw[0], ew[0], ei[1]+1, ew[1] },
                          { bw[0], bi[0], bi[1]  , ei[1]+1},
                          { ei[0], ew[0], bi[1]  , ei[1]+1} };
    
#pragma acc parallel present(exy, cexy, exy, rer_ex, cexyl, hz, ex)
#pragma acc loop seq
    for (int l=0; l<4; l++) {
#pragma acc loop independent
        for (int j=r[l][2]; j<r[l][3]; j++) {
#pragma acc loop independent
            for (int i=r[l][0]; i<r[l][1]; i++) {

                const int jj = j - bw[1];
                const int ii = i - bw[0];
                
                const int ix  = jj*whole->length[0] + ii;
                const int jm  = ix - whole->length[0];
                exy[ix] = cexy[jj]*exy[ix] + rer_ex[ix]*cexyl[jj]*(hz[ix] - hz[jm]);
                ex [ix]  = exy[ix];
            }
        }
        
    }
    
}


void pml_boundary_ey(const struct Range *whole, const struct Range *inside,
                     const FLOAT *hz, const FLOAT *ceyx, const FLOAT *ceyxl, const FLOAT *rer_ey,
                     FLOAT *ey, FLOAT *eyx)
{
    const int bi[] = { inside->begin[0], inside->begin[1] };
    const int ei[] = { inside->begin[0] + inside->length[0], inside->begin[1] + inside->length[1] };
    const int bw[] = { whole->begin[0], whole->begin[1] };
    const int ew[] = { whole->begin[0] + whole->length[0], whole->begin[1] + whole->length[1] };

    const int r[4][4] = { { bw[0]+1, ew[0], bw[1], bi[1] },
                          { bw[0]+1, ew[0], ei[1], ew[1] },
                          { bw[0]+1, bi[0], bi[1], ei[1] },
                          { ei[0]+1, ew[0], bi[1], ei[1] } };
    
#pragma acc parallel present(eyx, ceyx, rer_ey, ceyxl, hz, ey)
#pragma acc loop seq
    for (int l=0; l<4; l++) {
#pragma acc loop independent
        for (int j=r[l][2]; j<r[l][3]; j++) {
#pragma acc loop independent
            for (int i=r[l][0]; i<r[l][1]; i++) {

                const int jj = j - bw[1];
                const int ii = i - bw[0];
                
                const int ix  = jj*whole->length[0] + ii;
                const int im  = ix - 1;
                eyx[ix] = ceyx[ii]*eyx[ix] - rer_ey[ix]*ceyxl[ii]*(hz[ix]-hz[im]);
                ey [ix]  = eyx[ix];
            }
        }
        
    }    
    
}


void pml_boundary_hz(const struct Range *whole, const struct Range *inside,
                     const FLOAT *ey, const FLOAT *ex,
                     const FLOAT *chzx, const FLOAT *chzxl, const FLOAT *chzy, const FLOAT *chzyl,
                     FLOAT *hz, FLOAT *hzx, FLOAT *hzy)
{
    const int bi[] = { inside->begin[0], inside->begin[1] };
    const int ei[] = { inside->begin[0] + inside->length[0], inside->begin[1] + inside->length[1] };
    const int bw[] = { whole->begin[0], whole->begin[1] };
    const int ew[] = { whole->begin[0] + whole->length[0], whole->begin[1] + whole->length[1] };

    const int r[4][4] = { { bw[0], ew[0]-1, bw[1], bi[1] },
                          { bw[0], ew[0]-1, ei[1], ew[1]-1 },
                          { bw[0], bi[0]  , bi[1], ei[1] },
                          { ei[0], ew[0]-1, bi[1], ei[1] } };
    
#pragma acc parallel present(hzx, chzx, hzx, chzxl, ey, hzy, chzy, hzy, chzyl, ex, hz)
#pragma acc loop seq
    for (int l=0; l<4; l++) {
#pragma acc loop independent
        for (int j=r[l][2]; j<r[l][3]; j++) {
#pragma acc loop independent
            for (int i=r[l][0]; i<r[l][1]; i++) {
                
                const int jj = j - bw[1];
                const int ii = i - bw[0];
                
                const int ix  = jj*whole->length[0] + ii;
                const int ip  = ix + 1;
                const int jp  = ix + whole->length[0];
                hzx[ix] = chzx[ii]*hzx[ix] - chzxl[ii]*(ey[ip]-ey[ix]);
                hzy[ix] = chzy[jj]*hzy[ix] + chzyl[jj]*(ex[jp]-ex[ix]);
                hz [ix]  = hzx[ix] + hzy[ix];
            }
        }
        
    }
    
}
