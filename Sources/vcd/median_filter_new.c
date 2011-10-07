// adaptions for photivo
// width -> m_Width
// height -> m_Height
// image -> m_Image
// ushort -> uint16_t
// lines with 'verbose' -> TRACEKEYVALS("3x3 differential median filter","%s","");

/* 
   differential median filter 
*/
#define PIX_SORT(a,b) { if ((a)>(b)) {temp=(a);(a)=(b);(b)=temp;} }
void CLASS median_filter_new()
{
  int (*mf)[3], (*pc)[3], p[9], indx, c, d, temp, v0, pass;
  int p11, p12, p13, p31, p32, p33;
  p11 = -m_Width-1;
  p12 = p11+1;
  p13 = p12+1;
  p31 =  m_Width-1;
  p32 = p31+1;
  p33 = p32+1;
  /* Allocate buffer for 3x3 median filter */
  mf = (int (*)[3]) calloc(m_Width*m_Height, sizeof *mf);
  for (pass=1; pass <= m_UserSetting_MedianPasses; pass++) {
    TRACEKEYVALS("3x3 differential median filter","%s","");
    for (c=0; c < 3; c+=2) {
      /* Compute median(R-G) and median(B-G) */ 
      for (indx=0; indx < m_Height*m_Width; indx++)
				mf[indx][c] = m_Image[indx][c] - m_Image[indx][1];
      /* Apply 3x3 median fileter */
#pragma omp parallel for schedule(static) private(p, temp, pc)			
      for (uint16_t row=1; row < m_Height-1; row++)
				for (uint16_t col=1; col < m_Width-1; col++) {
					pc = mf + row*m_Width+col;
					/* Assign 3x3 differential color values */
					p[0] = pc[p11][c]; p[1] = pc[p12][c]; p[2] = pc[p13][c];
					p[3] = pc[ -1][c]; p[4] = pc[  0][c]; p[5] = pc[  1][c];
					p[6] = pc[p31][c]; p[7] = pc[p32][c]; p[8] = pc[p33][c];
					/* Sort for median of 9 values */
					PIX_SORT(p[1],p[2]); PIX_SORT(p[4], p[5]); PIX_SORT(p[7],p[8]);
					PIX_SORT(p[0],p[1]); PIX_SORT(p[3], p[4]); PIX_SORT(p[6],p[7]);
					PIX_SORT(p[1],p[2]); PIX_SORT(p[4], p[5]); PIX_SORT(p[7],p[8]);
					PIX_SORT(p[0],p[3]); PIX_SORT(p[5], p[8]); PIX_SORT(p[4],p[7]);
					PIX_SORT(p[3],p[6]); PIX_SORT(p[1], p[4]); PIX_SORT(p[2],p[5]);
					PIX_SORT(p[4],p[7]); PIX_SORT(p[4], p[2]); PIX_SORT(p[6],p[4]);
					PIX_SORT(p[4],p[2]);
					pc[0][1] = p[4];
				}
#pragma omp parallel for schedule(static) private(pc)				
      for (uint16_t row=1; row < m_Height-1; row++)
				for (uint16_t col=1; col < m_Width-1; col++) {
					pc = mf + row*m_Width+col;
					pc[0][c] = pc[0][1]; 
				}
    }

    /* red/blue at GREEN pixel locations */
#pragma omp parallel for schedule(static) private(c, indx, v0)		
    for (uint16_t row=1; row < m_Height-1; row++)
      for (uint16_t col=1+(FC(row,2) & 1), c=FC(row,col+1); col < m_Width-1; col+=2) {
				indx = row*m_Width+col;
				for (int i=0; i < 2; c=2-c, i++) {
					v0 = m_Image[indx][1]+mf[indx][c];
					m_Image[indx][c] = CLIP(v0);
				}
      }

    /* red/blue at BLUE/RED pixel locations */
#pragma omp parallel for schedule(static) private(c, indx, v0)		
    for (uint16_t row=2; row < m_Height-2; row++)
      for (uint16_t col=2+(FC(row,2) & 1), c=2-FC(row,col); col < m_Width-2; col+=2) {
				indx = row*m_Width+col;
				v0 = m_Image[indx][1]+mf[indx][c];
				m_Image[indx][c] = CLIP(v0);
      }

    /* green at RED/BLUE location */
#pragma omp parallel for schedule(static) private(c, d, indx, v0)		
    for (uint16_t row=1; row < m_Height-1; row++)
      for (uint16_t col=1+(FC(row,1) & 1), c=FC(row,col); col < m_Width-3; col+=2) {
				indx = row*m_Width+col;
				d = 2 - c;
				v0 = (m_Image[indx][c]-mf[indx][c]+m_Image[indx][d]-mf[indx][d]+1) >> 1;
				m_Image[indx][1] = CLIP(v0);
      }
  }

  /* Free buffer */
  free(mf);
}
#undef PIX_SORT
