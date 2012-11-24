/*
 *      Redistribution and use in source and binary forms, with or without
 *      modification, are permitted provided that the following conditions are
 *      met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *      * Neither the name of the author nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// If you want to use the code, you need to display name of the original authors in
// your software!



/* DCB demosaicing by Jacek Gozdz (cuniek@kft.umcs.lublin.pl)
 * the implementation is not speed optimised
 * the code is open source (BSD licence)
*/

/* FBDD denoising by Jacek Gozdz (cuniek@kft.umcs.lublin.pl) and Luis Sanz Rodríguez (luis.sanz.rodriguez@gmail.com)
 * the implementation is not speed optimised
 * the code is open source (BSD licence)
*/

// adaptions for photivo
// width -> m_Width
// height -> m_Height
// image -> m_Image
// ushort -> uint16_t
// lines with 'verbose' -> TRACEKEYVALS("DCB sharp demosaicing","%s","");


// R and B smoothing using green contrast, all pixels except 2 pixel wide border
void CLASS dcb_pp()
{
  int g1, r1, b1, u=m_Width, indx, row, col;

#pragma omp parallel for schedule(static) private(row, col, indx, g1, r1, b1)
  for (row=2; row < m_Height-2; row++)
    for (col=2, indx=row*u+col; col < m_Width-2; col++, indx++) {

      r1 = ( m_Image[indx-1][0] + m_Image[indx+1][0] + m_Image[indx-u][0] + m_Image[indx+u][0] + m_Image[indx-u-1][0] + m_Image[indx+u+1][0] + m_Image[indx-u+1][0] + m_Image[indx+u-1][0])/8.0;
      g1 = ( m_Image[indx-1][1] + m_Image[indx+1][1] + m_Image[indx-u][1] + m_Image[indx+u][1] + m_Image[indx-u-1][1] + m_Image[indx+u+1][1] + m_Image[indx-u+1][1] + m_Image[indx+u-1][1])/8.0;
      b1 = ( m_Image[indx-1][2] + m_Image[indx+1][2] + m_Image[indx-u][2] + m_Image[indx+u][2] + m_Image[indx-u-1][2] + m_Image[indx+u+1][2] + m_Image[indx-u+1][2] + m_Image[indx+u-1][2])/8.0;

      m_Image[indx][0] = CLIP(r1 + ( m_Image[indx][1] - g1 ));
      m_Image[indx][2] = CLIP(b1 + ( m_Image[indx][1] - g1 ));

    }
}

// saves red and blue
void CLASS copy_to_buffer(float (*m_Image2)[3])
{
  int indx;
#pragma omp parallel for schedule(static) private(indx)
  for (indx=0; indx < m_Height*m_Width; indx++) {
    m_Image2[indx][0]=m_Image[indx][0]; //R
    m_Image2[indx][2]=m_Image[indx][2]; //B
  }
}

// restores red and blue
void CLASS restore_from_buffer(float (*m_Image2)[3])
{
  int indx;
#pragma omp parallel for schedule(static) private(indx)
  for (indx=0; indx < m_Height*m_Width; indx++) {
    m_Image[indx][0]=m_Image2[indx][0]; //R
    m_Image[indx][2]=m_Image2[indx][2]; //B
  }
}

// fast green interpolation
void CLASS hid()
{
  int row, col, c, u=m_Width, v=2*u, indx;
#pragma omp parallel for schedule(static) private(row, col, c, indx)
  for (row=2; row < m_Height-2; row++) {
    for (col=2, indx=row*m_Width+col; col < m_Width-2; col++, indx++) {

      c =  fcol(row,col);
      if(c != 1) {
        m_Image[indx][1] = CLIP((int32_t)((m_Image[indx+u][1] + m_Image[indx-u][1] + m_Image[indx-1][1] + m_Image[indx+1][1])/4.0f +
                 (m_Image[indx][c] - ( m_Image[indx+v][c] + m_Image[indx-v][c] + m_Image[indx-2][c] + m_Image[indx+2][c])/4.0f)/2.f));
      }
    }
  }
}

// green correction
void CLASS hid2()
{
  int row, col, c, u=m_Width, v=2*u, indx;

#pragma omp parallel for schedule(static) private(row, col, c, indx)
  for (row=4; row < m_Height-4; row++) {
    for (col=4, indx=row*m_Width+col; col < m_Width-4; col++, indx++) {

      c =  fcol(row,col);

      if (c != 1) {
        m_Image[indx][1] = CLIP((int32_t)((m_Image[indx+v][1] + m_Image[indx-v][1] + m_Image[indx-2][1] + m_Image[indx+2][1])/4.0f +
                  m_Image[indx][c] - ( m_Image[indx+v][c] + m_Image[indx-v][c] + m_Image[indx-2][c] + m_Image[indx+2][c])/4.0f));
      }
    }
  }
}

// missing colors are interpolated
void CLASS dcb_color()
{
  int row, col, c, d, u=m_Width, indx;
#pragma omp parallel
{
#pragma omp for schedule(static) private(row, col, c, indx)
  for (row=1; row < m_Height-1; row++)
    for (col=1+(FC(row,1) & 1), indx=row*m_Width+col, c=2-FC(row,col); col < u-1; col+=2, indx+=2) {
      m_Image[indx][c] = CLIP((int32_t)((
                              4*m_Image[indx][1]
                              - m_Image[indx+u+1][1] - m_Image[indx+u-1][1] - m_Image[indx-u+1][1] - m_Image[indx-u-1][1]
                              + m_Image[indx+u+1][c] + m_Image[indx+u-1][c] + m_Image[indx-u+1][c] + m_Image[indx-u-1][c] )/4.0f));
    }
#pragma omp for schedule(static) private(row, col, c, d, indx)
  for (row=1; row<m_Height-1; row++)
    for (col=1+(FC(row,2)&1), indx=row*m_Width+col,c=FC(row,col+1),d=2-c; col<m_Width-1; col+=2, indx+=2) {
      m_Image[indx][c] = CLIP((int32_t)((2*m_Image[indx][1] - m_Image[indx+1][1] - m_Image[indx-1][1] + m_Image[indx+1][c] + m_Image[indx-1][c])/2.0f));
      m_Image[indx][d] = CLIP((int32_t)((2*m_Image[indx][1] - m_Image[indx+u][1] - m_Image[indx-u][1] + m_Image[indx+u][d] + m_Image[indx-u][d])/2.0f));
    }
} // end of parallel
}

// missing colors are interpolated using high quality algorithm by Luis Sanz Rodríguez
void CLASS dcb_color_full()
{
  int row,col,c,d,u=m_Width,w=3*u,indx;
  float f[4],g[4],(*chroma)[2];

  chroma = (float (*)[2]) calloc(m_Width*m_Height,sizeof *chroma); merror (chroma, "dcb_color_full()");
#pragma omp parallel
{
#pragma omp for schedule(static) private(row, col, c, indx, d)
  for (row=1; row < m_Height-1; row++)
    for (col=1+(FC(row,1)&1),indx=row*m_Width+col,c=FC(row,col),d=c/2; col < u-1; col+=2,indx+=2)
      chroma[indx][d]=m_Image[indx][c]-m_Image[indx][1];
#pragma omp for schedule(static) private(row, col, c, d, indx, f, g)
  for (row=3; row<m_Height-3; row++)
    for (col=3+(FC(row,1)&1),indx=row*m_Width+col,c=1-FC(row,col)/2,d=1-c; col<u-3; col+=2,indx+=2) {
      f[0]=1.0/(float)(1.0+fabs(chroma[indx-u-1][c]-chroma[indx+u+1][c])+fabs(chroma[indx-u-1][c]-chroma[indx-w-3][c])+fabs(chroma[indx+u+1][c]-chroma[indx-w-3][c]));
      f[1]=1.0/(float)(1.0+fabs(chroma[indx-u+1][c]-chroma[indx+u-1][c])+fabs(chroma[indx-u+1][c]-chroma[indx-w+3][c])+fabs(chroma[indx+u-1][c]-chroma[indx-w+3][c]));
      f[2]=1.0/(float)(1.0+fabs(chroma[indx+u-1][c]-chroma[indx-u+1][c])+fabs(chroma[indx+u-1][c]-chroma[indx+w+3][c])+fabs(chroma[indx-u+1][c]-chroma[indx+w-3][c]));
      f[3]=1.0/(float)(1.0+fabs(chroma[indx+u+1][c]-chroma[indx-u-1][c])+fabs(chroma[indx+u+1][c]-chroma[indx+w-3][c])+fabs(chroma[indx-u-1][c]-chroma[indx+w+3][c]));
      g[0]=1.325*chroma[indx-u-1][c]-0.175*chroma[indx-w-3][c]-0.075*chroma[indx-w-1][c]-0.075*chroma[indx-u-3][c];
      g[1]=1.325*chroma[indx-u+1][c]-0.175*chroma[indx-w+3][c]-0.075*chroma[indx-w+1][c]-0.075*chroma[indx-u+3][c];
      g[2]=1.325*chroma[indx+u-1][c]-0.175*chroma[indx+w-3][c]-0.075*chroma[indx+w-1][c]-0.075*chroma[indx+u-3][c];
      g[3]=1.325*chroma[indx+u+1][c]-0.175*chroma[indx+w+3][c]-0.075*chroma[indx+w+1][c]-0.075*chroma[indx+u+3][c];
      chroma[indx][c]=(f[0]*g[0]+f[1]*g[1]+f[2]*g[2]+f[3]*g[3])/(f[0]+f[1]+f[2]+f[3]);
    }
#pragma omp for schedule(static) private(row, col, c, d, indx, f, g)
  for (row=3; row<m_Height-3; row++)
    for (col=3+(FC(row,2)&1),indx=row*m_Width+col,c=FC(row,col+1)/2; col<u-3; col+=2,indx+=2)
      for(d=0;d<=1;c=1-c,d++){
        f[0]=1.0/(float)(1.0+fabs(chroma[indx-u][c]-chroma[indx+u][c])+fabs(chroma[indx-u][c]-chroma[indx-w][c])+fabs(chroma[indx+u][c]-chroma[indx-w][c]));
        f[1]=1.0/(float)(1.0+fabs(chroma[indx+1][c]-chroma[indx-1][c])+fabs(chroma[indx+1][c]-chroma[indx+3][c])+fabs(chroma[indx-1][c]-chroma[indx+3][c]));
        f[2]=1.0/(float)(1.0+fabs(chroma[indx-1][c]-chroma[indx+1][c])+fabs(chroma[indx-1][c]-chroma[indx-3][c])+fabs(chroma[indx+1][c]-chroma[indx-3][c]));
        f[3]=1.0/(float)(1.0+fabs(chroma[indx+u][c]-chroma[indx-u][c])+fabs(chroma[indx+u][c]-chroma[indx+w][c])+fabs(chroma[indx-u][c]-chroma[indx+w][c]));

        g[0]=0.875*chroma[indx-u][c]+0.125*chroma[indx-w][c];
        g[1]=0.875*chroma[indx+1][c]+0.125*chroma[indx+3][c];
        g[2]=0.875*chroma[indx-1][c]+0.125*chroma[indx-3][c];
        g[3]=0.875*chroma[indx+u][c]+0.125*chroma[indx+w][c];

        chroma[indx][c]=(f[0]*g[0]+f[1]*g[1]+f[2]*g[2]+f[3]*g[3])/(f[0]+f[1]+f[2]+f[3]);
      }
#pragma omp for schedule(static) private(row, col, indx)
  for(row=3; row<m_Height-3; row++)
    for(col=3,indx=row*m_Width+col; col<m_Width-3; col++,indx++){
      m_Image[indx][0]=CLIP((int32_t)(chroma[indx][0]+m_Image[indx][1]));
      m_Image[indx][2]=CLIP((int32_t)(chroma[indx][1]+m_Image[indx][1]));
    }
} // end of parallel
  free(chroma);
}

// green is used to create
// an interpolation direction map
// 1 = vertical
// 0 = horizontal
// saved in m_Image[][3]
void CLASS dcb_map()
{
  int row, col, u=m_Width, indx;
#pragma omp parallel for schedule(static) private(row, col, indx)
  for (row=2; row < m_Height-2; row++) {
    for (col=2, indx=row*m_Width+col; col < m_Width-2; col++, indx++) {

      if (m_Image[indx][1] > ( m_Image[indx-1][1] + m_Image[indx+1][1] + m_Image[indx-u][1] + m_Image[indx+u][1])/4.0)
        m_Image[indx][3] = ((MIN( m_Image[indx-1][1], m_Image[indx+1][1]) + m_Image[indx-1][1] + m_Image[indx+1][1] ) < (MIN( m_Image[indx-u][1], m_Image[indx+u][1]) + m_Image[indx-u][1] + m_Image[indx+u][1]));
      else
        m_Image[indx][3] = ((MAX( m_Image[indx-1][1], m_Image[indx+1][1]) + m_Image[indx-1][1] + m_Image[indx+1][1] ) > (MAX( m_Image[indx-u][1], m_Image[indx+u][1]) + m_Image[indx-u][1] + m_Image[indx+u][1])) ;
    }
  }
}

// interpolated green pixels are corrected using the map
void CLASS dcb_correction()
{
  int current, row, col, c, u=m_Width, v=2*u, indx;
#pragma omp parallel for schedule(static) private(row, col, c, indx, current)
  for (row=4; row < m_Height-4; row++) {
    for (col=4, indx=row*m_Width+col; col < m_Width-4; col++, indx++) {

      c =  FC(row,col);

      if (c != 1) {
        current = 4*m_Image[indx][3] +
                2*(m_Image[indx+u][3] + m_Image[indx-u][3] + m_Image[indx+1][3] + m_Image[indx-1][3]) +
                m_Image[indx+v][3] + m_Image[indx-v][3] + m_Image[indx+2][3] + m_Image[indx-2][3];

        m_Image[indx][1] = ((16-current)*(m_Image[indx-1][1] + m_Image[indx+1][1])/2.0 + current*(m_Image[indx-u][1] + m_Image[indx+u][1])/2.0)/16.0;
      }

    }
  }

}

// interpolated green pixels are corrected using the map
// with correction
void CLASS dcb_correction2()
{
  int current, row, col, c, u=m_Width, v=2*u, indx;
#pragma omp parallel for schedule(static) private(row, col, c, indx, current)
  for (row=4; row < m_Height-4; row++) {
    for (col=4, indx=row*m_Width+col; col < m_Width-4; col++, indx++) {

      c =  FC(row,col);

      if (c != 1) {
        current = 4*m_Image[indx][3] +
                2*(m_Image[indx+u][3] + m_Image[indx-u][3] + m_Image[indx+1][3] + m_Image[indx-1][3]) +
                m_Image[indx+v][3] + m_Image[indx-v][3] + m_Image[indx+2][3] + m_Image[indx-2][3];

        m_Image[indx][1] = CLIP((int32_t)(((16-current)*((m_Image[indx-1][1] + m_Image[indx+1][1])/2.0f + m_Image[indx][c] - (m_Image[indx+2][c] + m_Image[indx-2][c])/2.0f) + current*((m_Image[indx-u][1] + m_Image[indx+u][1])/2.0f + m_Image[indx][c] - (m_Image[indx+v][c] + m_Image[indx-v][c])/2.0f))/16.0f));
      }
    }
  }
}

// m_Image refinement
void CLASS dcb_refinement()
{
  int row, col, c, u=m_Width, v=2*u, indx, max, min;
  float f[4], g[4];
#pragma omp parallel for schedule(static) private(row, col, c, indx, min, max, f, g)
  for (row=5; row < m_Height-5; row++)
    for (col=5+(FC(row,1)&1),indx=row*m_Width+col,c=FC(row,col); col < u-5; col+=2,indx+=2) {

      // Cubic Spline Interpolation by Li and Randhawa, modified by Jacek Gozdz and Luis Sanz Rodríguez
      f[0]=1.0/(1.0+abs(m_Image[indx-u][c]-m_Image[indx][c])+abs(m_Image[indx-u][1]-m_Image[indx][1]));
      f[1]=1.0/(1.0+abs(m_Image[indx+1][c]-m_Image[indx][c])+abs(m_Image[indx+1][1]-m_Image[indx][1]));
      f[2]=1.0/(1.0+abs(m_Image[indx-1][c]-m_Image[indx][c])+abs(m_Image[indx-1][1]-m_Image[indx][1]));
      f[3]=1.0/(1.0+abs(m_Image[indx+u][c]-m_Image[indx][c])+abs(m_Image[indx+u][1]-m_Image[indx][1]));

      g[0]=CLIP((int32_t)(m_Image[indx-u][1]+0.5f*(m_Image[indx][c]-m_Image[indx-u][c]) + 0.25f*(m_Image[indx][c]-m_Image[indx-v][c])));
      g[1]=CLIP((int32_t)(m_Image[indx+1][1]+0.5f*(m_Image[indx][c]-m_Image[indx+1][c]) + 0.25f*(m_Image[indx][c]-m_Image[indx+2][c])));
      g[2]=CLIP((int32_t)(m_Image[indx-1][1]+0.5f*(m_Image[indx][c]-m_Image[indx-1][c]) + 0.25f*(m_Image[indx][c]-m_Image[indx-2][c])));
      g[3]=CLIP((int32_t)(m_Image[indx+u][1]+0.5f*(m_Image[indx][c]-m_Image[indx+u][c]) + 0.25f*(m_Image[indx][c]-m_Image[indx+v][c])));

      m_Image[indx][1]=CLIP((int32_t)(((f[0]*g[0]+f[1]*g[1]+f[2]*g[2]+f[3]*g[3])/(f[0]+f[1]+f[2]+f[3]) )));

      // get rid of the overshooted pixels
      min = MIN(m_Image[indx+1+u][1], MIN(m_Image[indx+1-u][1], MIN(m_Image[indx-1+u][1], MIN(m_Image[indx-1-u][1], MIN(m_Image[indx-1][1], MIN(m_Image[indx+1][1], MIN(m_Image[indx-u][1], m_Image[indx+u][1])))))));

      max = MAX(m_Image[indx+1+u][1], MAX(m_Image[indx+1-u][1], MAX(m_Image[indx-1+u][1], MAX(m_Image[indx-1-u][1], MAX(m_Image[indx-1][1], MAX(m_Image[indx+1][1], MAX(m_Image[indx-u][1], m_Image[indx+u][1])))))));

      m_Image[indx][1] =  ULIM((int32_t)(m_Image[indx][1]), max, min);
    }
}

/*
m_Image[indx][0] = CLIP(65536.0*(1.0 - (1.0-m_Image[indx][0]/65536.0)*(1.0-m_Image[indx][0]/65536.0)));
m_Image[indx][1] = CLIP(65536.0*(1.0 - (1.0-m_Image[indx][1]/65536.0)*(1.0-m_Image[indx][1]/65536.0)));
m_Image[indx][2] = CLIP(65536.0*(1.0 - (1.0-m_Image[indx][2]/65536.0)*(1.0-m_Image[indx][2]/65536.0)));
*/

// converts RGB to LCH colorspace and saves it to m_Image3
void CLASS rgb_to_lch(double (*m_Image3)[3])
{
  int indx;
#pragma omp parallel for schedule(static) private(indx)
  for (indx=0; indx < m_Height*m_Width; indx++) {
    m_Image3[indx][0] = m_Image[indx][0] + m_Image[indx][1] + m_Image[indx][2];     // L
    m_Image3[indx][1] = 1.732050808 *(m_Image[indx][0] - m_Image[indx][1]);     // C
    m_Image3[indx][2] = 2.0*m_Image[indx][2] - m_Image[indx][0] - m_Image[indx][1];   // H
  }
}

// converts LCH to RGB colorspace and saves it back to m_Image
void CLASS lch_to_rgb(double (*m_Image3)[3])
{
  int indx;
#pragma omp parallel for schedule(static) private(indx)
  for (indx=0; indx < m_Height*m_Width; indx++) {
    m_Image[indx][0] = CLIP((int32_t)(m_Image3[indx][0] / 3.0f - m_Image3[indx][2] / 6.0f + m_Image3[indx][1] / 3.464101615));
    m_Image[indx][1] = CLIP((int32_t)(m_Image3[indx][0] / 3.0f - m_Image3[indx][2] / 6.0f - m_Image3[indx][1] / 3.464101615));
    m_Image[indx][2] = CLIP((int32_t)(m_Image3[indx][0] / 3.0f + m_Image3[indx][2] / 3.0f));
  }
}

// fast green interpolation
void CLASS fbdd_green2()
{
  int row, col, c, u=m_Width, v=2*u, w=3*u, indx, current, min, max, g1, g2;
#pragma omp parallel for schedule(static) private(row, col, c, indx, current, min, max, g1, g2)
  for (row=4; row < m_Height-4; row++) {
    for (col=4, indx=row*m_Width+col; col < m_Width-4; col++, indx++) {
      c =  fcol(row,col);
      if(c != 1) {
        current = m_Image[indx][c] - (m_Image[indx+v][c] + m_Image[indx-v][c] + m_Image[indx-2][c] + m_Image[indx+2][c])/4.0;

        g2 = (m_Image[indx+u][1] + m_Image[indx-u][1] + m_Image[indx-1][1] + m_Image[indx+1][1])/4.0;
        g1 = (m_Image[indx+w][1] + m_Image[indx-w][1] + m_Image[indx-3][1] + m_Image[indx+3][1])/4.0;

        m_Image[indx][1] = CLIP((int32_t)((g2+g1)/2.0f + current));

        min = MIN(m_Image[indx-1][1], MIN(m_Image[indx+1][1], MIN(m_Image[indx-u][1], m_Image[indx+u][1])));

        max = MAX(m_Image[indx-1][1], MAX(m_Image[indx+1][1], MAX(m_Image[indx-u][1], m_Image[indx+u][1])));

        m_Image[indx][1] =  ULIM((int32_t)(m_Image[indx][1]), max, min);
      }
    }
  }
}

// denoising using interpolated neighbours
void CLASS fbdd_correction()
{
  int row, col, c, u=m_Width, indx;
#pragma omp parallel for schedule(static) private(row, col, c, indx)
  for (row=2; row < m_Height-2; row++) {
    for (col=2, indx=row*m_Width+col; col < m_Width-2; col++, indx++) {
      c = fcol(row,col);
      m_Image[indx][c] = ULIM(m_Image[indx][c],
        MAX(m_Image[indx-1][c], MAX(m_Image[indx+1][c], MAX(m_Image[indx-u][c], m_Image[indx+u][c]))),
        MIN(m_Image[indx-1][c], MIN(m_Image[indx+1][c], MIN(m_Image[indx-u][c], m_Image[indx+u][c]))));
    }
  }
}

// corrects chroma noise
void CLASS fbdd_correction2(double (*m_Image3)[3])
{
  int indx, v=2*m_Width;
  double Co, Ho, ratio;
#pragma omp parallel for schedule(static) private(indx, Co, Ho, ratio)
  for (indx=2+v; indx < m_Height*m_Width-(2+v); indx++) {
    if ( m_Image3[indx][1]*m_Image3[indx][2] != 0 ) {
      Co = (m_Image3[indx+v][1] + m_Image3[indx-v][1] + m_Image3[indx-2][1] + m_Image3[indx+2][1] -
        MAX(m_Image3[indx-2][1], MAX(m_Image3[indx+2][1], MAX(m_Image3[indx-v][1], m_Image3[indx+v][1]))) -
        MIN(m_Image3[indx-2][1], MIN(m_Image3[indx+2][1], MIN(m_Image3[indx-v][1], m_Image3[indx+v][1]))))/2.0;
      Ho = (m_Image3[indx+v][2] + m_Image3[indx-v][2] + m_Image3[indx-2][2] + m_Image3[indx+2][2] -
        MAX(m_Image3[indx-2][2], MAX(m_Image3[indx+2][2], MAX(m_Image3[indx-v][2], m_Image3[indx+v][2]))) -
        MIN(m_Image3[indx-2][2], MIN(m_Image3[indx+2][2], MIN(m_Image3[indx-v][2], m_Image3[indx+v][2]))))/2.0;
      ratio = sqrt ((Co*Co+Ho*Ho) / (m_Image3[indx][1]*m_Image3[indx][1] + m_Image3[indx][2]*m_Image3[indx][2]));
      if (ratio < 0.85){
        m_Image3[indx][1] = Co;
        m_Image3[indx][2] = Ho;
      }
    }
  }
}

// Cubic Spline Interpolation by Li and Randhawa, modified by Jacek Gozdz and Luis Sanz Rodríguez
void CLASS fbdd_green()
{
  int row, col, c, u=m_Width, v=2*u, w=3*u, x=4*u, y=5*u, indx, min, max;
  float f[4], g[4];
#pragma omp parallel for schedule(static) private(row, col, c, indx, min, max, f, g)
  for (row=5; row < m_Height-5; row++)
    for (col=5+(FC(row,1)&1),indx=row*m_Width+col,c=FC(row,col); col < u-5; col+=2,indx+=2) {
      f[0]=1.0f/(1.0f+abs(m_Image[indx-u][1]-m_Image[indx-w][1])+abs(m_Image[indx-w][1]-m_Image[indx+y][1]));
      f[1]=1.0f/(1.0f+abs(m_Image[indx+1][1]-m_Image[indx+3][1])+abs(m_Image[indx+3][1]-m_Image[indx-5][1]));
      f[2]=1.0f/(1.0f+abs(m_Image[indx-1][1]-m_Image[indx-3][1])+abs(m_Image[indx-3][1]-m_Image[indx+5][1]));
      f[3]=1.0f/(1.0f+abs(m_Image[indx+u][1]-m_Image[indx+w][1])+abs(m_Image[indx+w][1]-m_Image[indx-y][1]));

      g[0]=CLIP((int32_t)((23*m_Image[indx-u][1]+23*m_Image[indx-w][1]+2*m_Image[indx-y][1]+8*(m_Image[indx-v][c]-m_Image[indx-x][c])+40*(m_Image[indx][c]-m_Image[indx-v][c]))/48.0f));
      g[1]=CLIP((int32_t)((23*m_Image[indx+1][1]+23*m_Image[indx+3][1]+2*m_Image[indx+5][1]+8*(m_Image[indx+2][c]-m_Image[indx+4][c])+40*(m_Image[indx][c]-m_Image[indx+2][c]))/48.0f));
      g[2]=CLIP((int32_t)((23*m_Image[indx-1][1]+23*m_Image[indx-3][1]+2*m_Image[indx-5][1]+8*(m_Image[indx-2][c]-m_Image[indx-4][c])+40*(m_Image[indx][c]-m_Image[indx-2][c]))/48.0f));
      g[3]=CLIP((int32_t)((23*m_Image[indx+u][1]+23*m_Image[indx+w][1]+2*m_Image[indx+y][1]+8*(m_Image[indx+v][c]-m_Image[indx+x][c])+40*(m_Image[indx][c]-m_Image[indx+v][c]))/48.0f));

      m_Image[indx][1]=CLIP((int32_t)((f[0]*g[0]+f[1]*g[1]+f[2]*g[2]+f[3]*g[3])/(f[0]+f[1]+f[2]+f[3])));

      min = MIN(m_Image[indx+1+u][1], MIN(m_Image[indx+1-u][1], MIN(m_Image[indx-1+u][1], MIN(m_Image[indx-1-u][1], MIN(m_Image[indx-1][1], MIN(m_Image[indx+1][1], MIN(m_Image[indx-u][1], m_Image[indx+u][1])))))));

      max = MAX(m_Image[indx+1+u][1], MAX(m_Image[indx+1-u][1], MAX(m_Image[indx-1+u][1], MAX(m_Image[indx-1-u][1], MAX(m_Image[indx-1][1], MAX(m_Image[indx+1][1], MAX(m_Image[indx-u][1], m_Image[indx+u][1])))))));

      m_Image[indx][1] = ULIM((int32_t)(m_Image[indx][1]), max, min);
    }
}

// red and blue interpolation by Luis Sanz Rodríguez
void CLASS fbdd_color()
{
  int row,col,c,d,u=m_Width,w=3*u,indx,(*chroma)[2];
  float f[4];
  chroma = (int (*)[2]) calloc(m_Width*m_Height,sizeof *chroma); merror (chroma, "fbdd_color2()");
#pragma omp parallel
{
#pragma omp for schedule(static) private(row, col, c, d, indx)
  for (row=2; row < m_Height-2; row++)
    for (col=2+(FC(row,2)&1),indx=row*m_Width+col,c=FC(row,col),d=c/2; col<u-2; col+=2,indx+=2)
      chroma[indx][d]=m_Image[indx][c]-m_Image[indx][1];
#pragma omp for schedule(static) private(row, col, c, d, indx, f)
  for (row=3; row<m_Height-3; row++)
    for (col=3+(FC(row,1)&1),indx=row*m_Width+col,d=1-FC(row,col)/2,c=2*d; col<u-3; col+=2,indx+=2) {
      f[0]=1.0f/(1.0f+abs(chroma[indx-u-1][d]-chroma[indx+u+1][d])+abs(chroma[indx-u-1][d]-chroma[indx-w-3][d])+abs(chroma[indx+u+1][d]-chroma[indx-w-3][d]));
      f[1]=1.0f/(1.0f+abs(chroma[indx-u+1][d]-chroma[indx+u-1][d])+abs(chroma[indx-u+1][d]-chroma[indx-w+3][d])+abs(chroma[indx+u-1][d]-chroma[indx-w+3][d]));
      f[2]=1.0f/(1.0f+abs(chroma[indx+u-1][d]-chroma[indx-u+1][d])+abs(chroma[indx+u-1][d]-chroma[indx+w+3][d])+abs(chroma[indx-u+1][d]-chroma[indx+w-3][d]));
      f[3]=1.0f/(1.0f+abs(chroma[indx+u+1][d]-chroma[indx-u-1][d])+abs(chroma[indx+u+1][d]-chroma[indx+w-3][d])+abs(chroma[indx-u-1][d]-chroma[indx+w+3][d]));
      chroma[indx][d]=(f[0]*chroma[indx-u-1][d]+f[1]*chroma[indx-u+1][d]+f[2]*chroma[indx+u-1][d]+f[3]*chroma[indx+u+1][d])/(f[0]+f[1]+f[2]+f[3]);
      m_Image[indx][c]=CLIP((int32_t)(chroma[indx][d]+m_Image[indx][1]));
    }
#pragma omp for schedule(static) private(row, col, c, d, indx, f)
  for (row=3; row<m_Height-3; row++)
    for (col=3+(FC(row,2)&1),indx=row*m_Width+col; col<u-3; col+=2,indx+=2)
      for(c=d=0;d<=1;c+=2,d++){
        f[0]=1.0f/(1.0f+abs(chroma[indx-u][d]-chroma[indx+u][d])+abs(chroma[indx-u][d]-chroma[indx-w][d])+abs(chroma[indx+u][d]-chroma[indx-w][d]));
        f[1]=1.0f/(1.0f+abs(chroma[indx+1][d]-chroma[indx-1][d])+abs(chroma[indx+1][d]-chroma[indx+3][d])+abs(chroma[indx-1][d]-chroma[indx+3][d]));
        f[2]=1.0f/(1.0f+abs(chroma[indx-1][d]-chroma[indx+1][d])+abs(chroma[indx-1][d]-chroma[indx-3][d])+abs(chroma[indx+1][d]-chroma[indx-3][d]));
        f[3]=1.0f/(1.0f+abs(chroma[indx+u][d]-chroma[indx-u][d])+abs(chroma[indx+u][d]-chroma[indx+w][d])+abs(chroma[indx-u][d]-chroma[indx+w][d]));
        m_Image[indx][c]=CLIP((int32_t)((f[0]*chroma[indx-u][d]+f[1]*chroma[indx+1][d]+f[2]*chroma[indx-1][d]+f[3]*chroma[indx+u][d])/(f[0]+f[1]+f[2]+f[3])+m_Image[indx][1]));
      }
} // end of parallel
  free(chroma);
}

// FBDD (Fake Before Demosaicing Denoising)
void CLASS fbdd(int noiserd)
{
  double (*m_Image3)[3];
  m_Image3 = (double (*)[3]) calloc(m_Width*m_Height, sizeof *m_Image3);

  border_interpolate(4);

if (noiserd>0)
{
  TRACEKEYVALS("FBDD full noise reduction...","%s","");

  fbdd_green();
  fbdd_color();
  fbdd_correction();

  dcb_color();
  rgb_to_lch(m_Image3);
  fbdd_correction2(m_Image3);
  fbdd_correction2(m_Image3);
  lch_to_rgb(m_Image3);

  fbdd_green();
  fbdd_color();
  fbdd_correction();
}
else
{
  TRACEKEYVALS("FBDD noise reduction...","%s","");
  fbdd_green();
  fbdd_color();
  fbdd_correction();
}
  free(m_Image3);
}

// DCB demosaicing main routine (sharp version)
void CLASS dcb(int iterations, int dcb_enhance)
{


  int i=1;
  float (*m_Image2)[3];
  m_Image2 = (float (*)[3]) calloc(m_Width*m_Height, sizeof *m_Image2);

  TRACEKEYVALS("DCB demosaicing","%s","");

  border_interpolate(2);
  copy_to_buffer(m_Image2);

  hid();
  dcb_color();

    while (i<=iterations)
    {
      TRACEKEYVALS("DCB correction pass","%s","");
      hid2();
      hid2();
      hid2();
      dcb_map();
      dcb_correction();
      i++;
    }

    dcb_color();
    dcb_pp();
    hid2();
    hid2();
    hid2();

  TRACEKEYVALS("finishing DCB","%s","");

    dcb_map();
    dcb_correction2();

    restore_from_buffer(m_Image2);

    dcb_map();
    dcb_correction();

    dcb_color();
    dcb_pp();
    dcb_map();
    dcb_correction();

    dcb_map();
    dcb_correction();

    restore_from_buffer(m_Image2);
    dcb_color();

  if (dcb_enhance)
  {
    TRACEKEYVALS("optional DCB refinement","%s","");
    dcb_refinement();
    dcb_color_full();
  }


  free(m_Image2);

}

