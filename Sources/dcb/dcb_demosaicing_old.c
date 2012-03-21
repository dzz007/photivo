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


/* This is DCB interpolation
 * author: Jacek Gozdz (cuniek@kft.umcs.lublin.pl)
 * the implementation is not speed optimised
 * the code is open source (BSD licence)
*/

// If you wan to use the code, please drop me a line about it

// adaptions for photivo
// width -> m_Width
// height -> m_Height
// image -> m_Image
// ushort -> uint16_t
// lines with 'verbose' -> TRACEKEYVALS("DCB sharp demosaicing","%s","");


// red and blue smoothing
void CLASS dcb_pp_old()
{
  int g1, r1, b1, u=m_Width, indx, row, col;
  uint16_t (*pix)[4];

  for (row=2; row < m_Height-2; row++) {
    for (col=2; col < m_Width-2; col++) {
      indx=row*m_Width+col;
      pix=m_Image+indx;

      r1 = ( pix[-1][0] + pix[1][0] + pix[-u][0] + pix[u][0] + pix[-u-1][0] + pix[+u+1][0] + pix[-u+1][0] + pix[+u-1][0])/8.0;
      g1 = ( pix[-1][1] + pix[1][1] + pix[-u][1] + pix[u][1] + pix[-u-1][1] + pix[+u+1][1] + pix[-u+1][1] + pix[+u-1][1])/8.0;
      b1 = ( pix[-1][2] + pix[1][2] + pix[-u][2] + pix[u][2] + pix[-u-1][2] + pix[+u+1][2] + pix[-u+1][2] + pix[+u-1][2])/8.0;

      m_Image[indx][0] = CLIP(r1 + ( pix[0][1] - g1 ));
      m_Image[indx][2] = CLIP(b1 + ( pix[0][1] - g1 ));
    }
  }
}



// saves red and blue
void CLASS copy_to_buffer_old(float (*m_Image2)[3])
{
  int indx;
#pragma omp parallel for schedule(static)
  for (indx=0; indx < m_Height*m_Width; indx++) {
    m_Image2[indx][0]=m_Image[indx][0]; //R
    m_Image2[indx][2]=m_Image[indx][2]; //B
  }
}



// restores red and blue
void CLASS restore_from_buffer_old(float (*m_Image2)[3])
{
  int indx;
#pragma omp parallel for schedule(static)
  for (indx=0; indx < m_Height*m_Width; indx++) {
    m_Image[indx][0]=m_Image2[indx][0]; //R
    m_Image[indx][2]=m_Image2[indx][2]; //B
  }

}



// fast green interpolation
void CLASS hid_old()
{
  int row, col, c, u=m_Width, v=2*u;
  uint16_t (*pix)[4];

  for (row=2; row < m_Height-2; row++) {
    for (col=2; col < m_Width-2; col++) {

      c =  fc(row,col);
      if(c != 1)
      {
        pix=m_Image+row*u+col;
        m_Image[row*u+col][1] = CLIP((pix[u][1] + pix[-u][1] + pix[-1][1] + pix[1][1])/4.0 +
                 (pix[0][c] - ( pix[v][c] + pix[-v][c] + pix[-2][c] + pix[2][c])/4.0)/2.0);
      }

    }
  }

}



// green correction
void CLASS hid2_old()
{
  int row, col, c, u=m_Width, v=2*u;
  uint16_t (*pix)[4];
// parallelizing is not completely rigorous here, but should be ok
#pragma omp parallel for private(col,c,pix) schedule(static)
  for (row=2; row < m_Height-2; row++) {
  for (col=2; col < m_Width-2; col++) {

    c =  fc(row,col);

    if (c != 1)
    {
      pix=m_Image+row*u+col;
      m_Image[row*u+col][1] = CLIP((pix[v][1] + pix[-v][1] + pix[-2][1] + pix[2][1])/4.0 +
                pix[0][c] - ( pix[v][c] + pix[-v][c] + pix[-2][c] + pix[2][c])/4.0);
      }

  }
  }
}



// missing colors are interpolated
void CLASS dcb_color_old()
{
  int row, col, c, d, u=m_Width;
  uint16_t (*pix)[4];

  for (row=1; row < m_Height-1; row++)
    for (col=1+(FC(row,1) & 1), c=2-FC(row,col),d=2-c; col < u-1; col+=2) {
      pix=m_Image+row*u+col;

      m_Image[row*u+col][c] = CLIP((
      4*pix[0][1]
      - pix[+u+1][1] - pix[+u-1][1] - pix[-u+1][1] - pix[-u-1][1]
      + pix[+u+1][c] + pix[+u-1][c] + pix[-u+1][c] + pix[-u-1][c] )/4.0);
    }

  for (row=1; row<m_Height-1; row++)
    for (col=1+(FC(row,2)&1),c=FC(row,col+1),d=2-c; col<m_Width-1; col+=2) {
      pix=m_Image+row*u+col;

      pix[0][c] = CLIP((2*pix[0][1] - pix[1][1] - pix[-1][1] + pix[1][c] + pix[-1][c])/2.0);
      pix[0][d] = CLIP((2*pix[0][1] - pix[u][1] - pix[-u][1] + pix[u][d] + pix[-u][d])/2.0);
    }
}



// green is used to create
// an interpolation direction map
// 1 = vertical
// 0 = horizontal
// saved in m_Image[][3]
void CLASS dcb_map_old()
{
  int row, col, u=m_Width, indx;
  uint16_t (*pix)[4];
#pragma omp parallel for private(col,indx,pix) schedule(static)
  for (row=2; row < m_Height-2; row++) {
  for (col=2; col < m_Width-2; col++) {

    indx=row*m_Width+col;
    pix=m_Image+indx;


    if (pix[0][1] > ( pix[-1][1] + pix[1][1] + pix[-u][1] + pix[u][1])/4.0)
      pix[0][3] = ((MIN( pix[-1][1], pix[1][1]) + pix[-1][1] + pix[1][1] ) < (MIN( pix[-u][1], pix[u][1]) + pix[-u][1] + pix[u][1]));
    else
      pix[0][3] = ((MAX( pix[-1][1], pix[1][1]) + pix[-1][1] + pix[1][1] ) > (MAX( pix[-u][1], pix[u][1]) + pix[-u][1] + pix[u][1])) ;
  }
  }
}



// all colors are used to create
// an interpolation direction map
// 1 = vertical
// 0 = horizontal
// saved in m_Image[][3]
void CLASS dcb_map2_old()
{
  int row, col, u=m_Width, indx;
  uint16_t (*pix)[4];
#pragma omp parallel for private(col,indx,pix) schedule(static)
  for (row=2; row < m_Height-2; row++) {
  for (col=2; col < m_Width-2; col++) {

    indx=row*m_Width+col;
    pix=m_Image+indx;


    if (pix[0][1] > ( pix[-1][1] + pix[1][1] + pix[-u][1] + pix[u][1])/4.0)
      pix[0][3] = ((MIN( pix[-1][1], pix[1][1]) + pix[-1][1] + pix[1][1] +
              MIN( pix[-1][0], pix[1][0]) + pix[-1][0] + pix[1][0] +
              MIN( pix[-1][2], pix[1][2]) + pix[-1][2] + pix[1][2]) <
             (MIN( pix[-u][1], pix[u][1]) + pix[-u][1] + pix[u][1] +
              MIN( pix[-u][0], pix[u][0]) + pix[-u][0] + pix[u][0] +
              MIN( pix[-u][2], pix[u][2]) + pix[-u][2] + pix[u][2]));
    else
      pix[0][3] = ((MAX( pix[-1][1], pix[1][1]) + pix[-1][1] + pix[1][1] +
              MAX( pix[-1][0], pix[1][0]) + pix[-1][0] + pix[1][0] +
              MAX( pix[-1][2], pix[1][2]) + pix[-1][2] + pix[1][2]) >
             (MAX( pix[-u][1], pix[u][1]) + pix[-u][1] + pix[u][1] +
              MAX( pix[-u][0], pix[u][0]) + pix[-u][0] + pix[u][0] +
              MAX( pix[-u][2], pix[u][2]) + pix[-u][2] + pix[u][2]));
  }
  }
}



// interpolate green according to the map
void CLASS dcb_correction_old()
{
  int current, row, col, c, u=m_Width, v=2*u, indx;
  uint16_t (*pix)[4];

  for (row=4; row < m_Height-4; row++) {
  for (col=4; col < m_Width-4; col++) {

// interpolated green pixels are corrected using the map

    c =  FC(row,col);

    if (c != 1)
    {

    indx=row*m_Width+col;
    pix=m_Image+indx;

      current = 4*pix[0][3] +
              2*(pix[u][3] + pix[-u][3] + pix[1][3] + pix[-1][3]) +
              pix[v][3] + pix[-v][3] + pix[2][3] + pix[-2][3];

      pix[0][1] = ((16-current)*(pix[-1][1] + pix[1][1])/2.0 + current*(pix[-u][1] + pix[u][1])/2.0)/16.0;
    }

  }
  }

}



// interpolate green according to the map
// with correction
void CLASS dcb_correction2_old()
{
  int current, row, col, c, u=m_Width, v=2*u, indx;
  uint16_t (*pix)[4];

  for (row=4; row < m_Height-4; row++) {
  for (col=4; col < m_Width-4; col++) {

// interpolated green pixels are corrected using the map

    c =  FC(row,col);

    if (c != 1)
    {

    indx=row*m_Width+col;
    pix=m_Image+indx;

      current = 4*pix[0][3] +
              2*(pix[u][3] + pix[-u][3] + pix[1][3] + pix[-1][3]) +
              pix[v][3] + pix[-v][3] + pix[2][3] + pix[-2][3];

      pix[0][1] = CLIP(((16-current)*((pix[-1][1] + pix[1][1])/2.0 + pix[0][c] - (pix[2][c] + pix[-2][c])/2.0) + current*((pix[-u][1] + pix[u][1])/2.0 + pix[0][c] - (pix[v][c] + pix[-v][c])/2.0))/16.0);
    }

  }
  }

}



// green is interpolated according to map
// with correction based on red and blue
void CLASS dcb_correction3_old()
{
  int current, row, col, c, u=m_Width, v=2*u, indx;
//warning: variable 'pix' set but not used [-Wunused-but-set-variable]
//  uint16_t (*pix)[4];

  for (row=4; row < m_Height-4; row++) {
  for (col=4; col < m_Width-4; col++) {

    indx=row*m_Width+col;
//    pix=m_Image+indx;
    c =  FC(row,col);

    if (c != 1)
    {

      current = 3*m_Image[row*m_Width+col][3] +
                2*(m_Image[(row+1)*m_Width+col][3] + m_Image[(row-1)*m_Width+col][3] + m_Image[row*m_Width+col+1][3] + m_Image[row*m_Width+col-1][3]) +
            m_Image[(row+2)*m_Width+col][3] + m_Image[(row-2)*m_Width+col][3] + m_Image[row*m_Width+col+2][3] + m_Image[row*m_Width+col-2][3];

      if (current > 7)
        m_Image[indx][1] = CLIP(((16-current)*(m_Image[row*m_Width+col-1][1] + m_Image[row*m_Width+col+1][1])/2.0 +
                       current*(m_Image[(row-1)*m_Width+col][1] + m_Image[(row+1)*m_Width+col][1])/2.0)/16.0 +
                       LIM((m_Image[indx][c] - (m_Image[indx-v][c] + m_Image[indx+v][c])/2.0)/2.0,
                       -MIN(ABS(m_Image[indx-u][1] - m_Image[indx+u][1]), ABS(m_Image[indx-u][c] + m_Image[indx+u][c])),
                      MIN(ABS(m_Image[indx-u][1] - m_Image[indx+u][1]), ABS(m_Image[indx-u][c] + m_Image[indx+u][c])) ));
      else
        m_Image[indx][1] = CLIP(((16-current)*(m_Image[row*m_Width+col-1][1] + m_Image[row*m_Width+col+1][1])/2.0 +
                       current*(m_Image[(row-1)*m_Width+col][1] + m_Image[(row+1)*m_Width+col][1])/2.0)/16.0 +
                       LIM((m_Image[indx][c] - (m_Image[indx-2][c] + m_Image[indx+2][c])/2.0)/2.0,
                      -MIN(ABS(m_Image[indx-1][1] - m_Image[indx+1][1]), ABS(m_Image[indx-1][c] + m_Image[indx+1][c])),
                       MIN(ABS(m_Image[indx-1][1] - m_Image[indx+1][1]), ABS(m_Image[indx-1][c] + m_Image[indx+1][c])) ));
      }
  }
  }

}



void CLASS dcb_interpolate_soft_old(int iterations, int dcb_smoothing)
{
  int i=1;
  float (*m_Image2)[3];
  m_Image2 = (float (*)[3]) calloc(m_Width*m_Height, sizeof *m_Image2);


  TRACEKEYVALS("DCB soft demosaicing","%s","");

    border_interpolate(2);

  copy_to_buffer_old(m_Image2);

hid_old();
dcb_color_old();

  while (i<=iterations)
  {
  fprintf (stderr,_("DCB correction pass %d...\n"), i);
  hid2_old();
  hid2_old();
  hid2_old();
  dcb_map_old();
  dcb_correction_old();
  i++;
  }

dcb_color_old();
dcb_pp_old();

hid2_old();
hid2_old();
hid2_old();

dcb_map_old();
dcb_correction2_old();

restore_from_buffer_old(m_Image2);

dcb_map_old();
dcb_correction_old();
dcb_color_old();
dcb_pp_old();
dcb_map_old();
dcb_correction_old();
dcb_map_old();
dcb_correction_old();

restore_from_buffer_old(m_Image2);

dcb_color_old();

  if (dcb_smoothing)
  {
    fprintf (stderr,_("DCB color smoothing...\n"));
    dcb_pp_old();
  }



free(m_Image2);

}



void CLASS dcb_interpolate_sharp_old(int iterations, int dcb_smoothing)
{
  int i=1;
  float (*m_Image2)[3];
  m_Image2 = (float (*)[3]) calloc(m_Width*m_Height, sizeof *m_Image2);


  TRACEKEYVALS("DCB sharp demosaicing","%s","");

  border_interpolate(4);

  copy_to_buffer_old(m_Image2);

hid_old();
dcb_color_old();

  while (i<=iterations)
  {
    fprintf (stderr,_("DCB correction pass %d...\n"), i);
    hid2_old();
    hid2_old();
    hid2_old();
    dcb_map_old();
    dcb_correction_old();
    i++;
  }

dcb_map_old();
dcb_correction_old();
dcb_map_old();
dcb_correction_old();
dcb_pp_old();

dcb_map_old();
dcb_correction_old();
dcb_map_old();
dcb_correction_old();

restore_from_buffer_old(m_Image2);

dcb_color_old();
dcb_map2_old();
dcb_correction_old();
dcb_map_old();
dcb_correction_old();
dcb_map_old();
dcb_correction_old();
dcb_map_old();
dcb_correction3_old();
dcb_color_old();

  if (dcb_smoothing)
  {
    fprintf (stderr,_("DCB color smoothing...\n"));
    dcb_pp_old();
  }



free(m_Image2);

}

