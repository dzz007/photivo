// adaptions for photivo
// width -> m_Width
// height -> m_Height
// image -> m_Image
// ushort -> uint16_t
// lines with 'verbose' -> TRACEKEYVALS("Post-demosaic refinement","%s","");

/*
   Refinement based on EECI demosaicing algorithm by L. Chang and Y.P. Tan
*/
void CLASS refinement()
{
  uint16_t (*pix)[4];
  int row, col, indx, c, i, d, w1, w2, v0;
  double dL, dR, dU, dD; //dt;
  clock_t t1; //t2;

  TRACEKEYVALS("Post-demosaic refinement","%s","");
  t1 = clock();
  w1 = m_Width;
  w2 = 2*w1;

  /* Reinforce interpolated green pixels on RED/BLUE pixel locations */
  for (row=2; row < m_Height-2; row++)
    for (col=2+(FC(row,2) & 1), c=FC(row,col); col < m_Width-2; col+=2) {
      indx = row*m_Width+col;
      pix = m_Image + indx;
      dL = 1.0/(1.0+ABS(pix[ -2][c]-pix[0][c])+ABS(pix[ 1][1]-pix[ -1][1]));
      dR = 1.0/(1.0+ABS(pix[  2][c]-pix[0][c])+ABS(pix[ 1][1]-pix[ -1][1]));
      dU = 1.0/(1.0+ABS(pix[-w2][c]-pix[0][c])+ABS(pix[w1][1]-pix[-w1][1]));
      dD = 1.0/(1.0+ABS(pix[ w2][c]-pix[0][c])+ABS(pix[w1][1]-pix[-w1][1]));
      v0 = (int)((double)pix[0][c] + 0.5 +
     ((double)(pix[ -1][1]-pix[ -1][c])*dL +
      (double)(pix[  1][1]-pix[  1][c])*dR +
      (double)(pix[-w1][1]-pix[-w1][c])*dU +
      (double)(pix[ w1][1]-pix[ w1][c])*dD ) / (dL+dR+dU+dD));
      pix[0][1] = CLIP(v0);
    }

  /* Reinforce interpolated red/blue pixels on GREEN pixel locations */
  for (row=2; row < m_Height-2; row++)
    for (col=2+(FC(row,3) & 1), c=FC(row,col+1); col < m_Width-2; col+=2) {
      indx = row*m_Width+col;
      pix = m_Image + indx;
      for (i=0; i < 2; c=2-c, i++) {
  dL = 1.0/(1.0+ABS(pix[ -2][1]-pix[0][1])+ABS(pix[ 1][c]-pix[ -1][c]));
  dR = 1.0/(1.0+ABS(pix[  2][1]-pix[0][1])+ABS(pix[ 1][c]-pix[ -1][c]));
  dU = 1.0/(1.0+ABS(pix[-w2][1]-pix[0][1])+ABS(pix[w1][c]-pix[-w1][c]));
  dD = 1.0/(1.0+ABS(pix[ w2][1]-pix[0][1])+ABS(pix[w1][c]-pix[-w1][c]));
  v0 = (int)((double)pix[0][1] + 0.5 -
       ((double)(pix[ -1][1]-pix[ -1][c])*dL +
        (double)(pix[  1][1]-pix[  1][c])*dR +
        (double)(pix[-w1][1]-pix[-w1][c])*dU +
        (double)(pix[ w1][1]-pix[ w1][c])*dD ) / (dL+dR+dU+dD));
  pix[0][c] = CLIP(v0);
      }
    }

  /* Reinforce integrated red/blue pixels on BLUE/RED pixel locations */
  for (row=2; row < m_Height-2; row++)
    for (col=2+(FC(row,2) & 1), c=2-FC(row,col); col < m_Width-2; col+=2) {
      indx = row*m_Width+col;
      pix = m_Image + indx;
      d = 2 - c;
      dL = 1.0/(1.0+ABS(pix[ -2][d]-pix[0][d])+ABS(pix[ 1][1]-pix[ -1][1]));
      dR = 1.0/(1.0+ABS(pix[  2][d]-pix[0][d])+ABS(pix[ 1][1]-pix[ -1][1]));
      dU = 1.0/(1.0+ABS(pix[-w2][d]-pix[0][d])+ABS(pix[w1][1]-pix[-w1][1]));
      dD = 1.0/(1.0+ABS(pix[ w2][d]-pix[0][d])+ABS(pix[w1][1]-pix[-w1][1]));
      v0 = (int)((double)pix[0][1] + 0.5 -
     ((double)(pix[ -1][1]-pix[ -1][c])*dL +
      (double)(pix[  1][1]-pix[  1][c])*dR +
      (double)(pix[-w1][1]-pix[-w1][c])*dU +
      (double)(pix[ w1][1]-pix[ w1][c])*dD ) / (dL+dR+dU+dD));
      pix[0][c] = CLIP(v0);
    }

  /* Done */
  //~ t2 = clock();
  //~ dt = ((double)(t2-t1)) / CLOCKS_PER_SEC;
  //~ if (verbose) fprintf(stderr,_("\telapsed time     = %5.3fs\n"),dt);
}
