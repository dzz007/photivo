/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * 
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
This file written in standard C by ben_pcc contains utilities for working with the gradient of a 32 bit image.
Very useful paper: Gradient Domain High Dynamic Range Compression by R. Fattal et al. Not followed exactly.

All cross boundary differentiation in this file happens under the assumption of no change accross boundary.
So for example a pixel at (-1, y) is equal to a pixel at (0, y) by definition.
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>


//Stores derivative of I into (Ix, Iy). Note that this is backward difference by shifting coordinates.
void ForwardDifferenceGradient(float *Ix, float *Iy, unsigned int w, unsigned int h, float *I){
	unsigned int x, y;
	unsigned int w1 = w - 1, h1 = h - 1;

	for(y = 0; y != h1; y++){
		//Pixel rows.
		float *rI = &I[y*w];
		float *rIx = &Ix[y*w];
		float *rIy = &Iy[y*w];

		//Interior, also the top and left boundaries.
		for(x = 0; x != w1; x++){
			float c = rI[x];
			rIx[x] = rI[x + 1] - c;
			rIy[x] = rI[x + w] - c;
		}

		//Right boundary.
		rIx[x] = 0.0f;
		rIy[x] = rI[x + w] - rI[x];
	}

	//Bottom boundary.
	y = w1 + h1*w;
	for(x = h1*w; x != y; x++)
		Ix[x] = I[x + 1] - I[x];
	Ix[x] = 0.0f;
	memset(&Iy[h1*w], 0, w*sizeof(float));
}

void BackwardDifferenceDivergence(float *Div, float *Ix, float *Iy, unsigned int w, unsigned int h){
	unsigned int x, y;

	for(y = 1;y != h; y++){
		float *rDiv = &Div[y*w];
		float *rIx = &Ix[y*w];
		float *rIy = &Iy[y*w];

		//Left boundary.
		rDiv[0] = rIx[0] + rIy[0] - rIy[-w];

		//Interior.
		for(x = 1; x != w; x++)
			rDiv[x] = rIx[x] - rIx[x - 1] + rIy[x] - rIy[x - w];
	}

	//Top boundary.		
	for(x = 1; x != w; x++)
		Div[x] = Ix[x] - Ix[x - 1] + Iy[x];

	//Top left corner.
	Div[0] = Ix[0] + Iy[0];
}

//Ok if In == Out.
void GaussianBlur(float *In, float *Out, int w, int h, unsigned int BlurWidth){
	//Fix blur width if necessary.
	if((int)BlurWidth >= h) BlurWidth = h - 3;
	BlurWidth += 1 - (BlurWidth & 1);			//Ensure odd.

	if(BlurWidth == 1){
		if(In != Out) memcpy(Out, In, sizeof(float)*w*h);
		return;
	}

	int i, x, y, w2 = BlurWidth >> 1;


	//Construct a normalized row of Pascal's triangle, this is the convolution vector.
	float *k0 = (float *)malloc(sizeof(float)*BlurWidth);
	k0[0] = k0[BlurWidth - 1] = powf(2.0f, -(float)BlurWidth);
	for(i = 1; i != w2 + 1; i++)
		k0[BlurWidth - 1 - i] = k0[i] = k0[i - 1]*(((float)BlurWidth + 1.0)/((float)i) - 1.0);
	float *k = &k0[w2];

	//Convolve x.
	float *R = (float *)malloc(sizeof(float)*w*h);
	for(y = 0; y != h; y++){
		float *rI = &In[y*w];
		float *rO = &R[y*w];

		//Work first on those which can be blurred without boundary consideration.
		for(x = w2; x != w - w2; x++){
			rO[x] = 0.0f;
			for(i = -w2; i <= w2; i++) rO[x] += k[i]*rI[x + i];
		}

		//Now work on left boundary...
		for(x = 0; x != w2; x++){
			rO[x] = 0.0f;
			float ks = 0.0f;
			for(i = -x; i <= w2; i++){
				rO[x] += k[i]*rI[x + i];
				ks += k[i];
			}
			rO[x] /= ks;
		}

		//...right boundary.
		for(x = w - w2; x != w; x++){
			rO[x] = 0.0f;
			float ks = 0.0f;
			for(i = -w2; i <= w - 1 - x; i++){
				rO[x] += k[i]*rI[x + i];
				ks += k[i];
			}
			rO[x] /= ks;
		}
	}

	//Convolve y. Working on columns is so messy.
	for(x = 0; x != w; x++){
		float *cI = &R[x];
		float *cO = &Out[x];

		for(y = w2; y != h - w2; y++){
			int yw = y*w;
			cO[yw] = 0.0f;
			for(i = -w2; i <= w2; i++) cO[yw] += k[i]*cI[yw + i*w];
		}

		for(y = 0; y != w2; y++){
			int yw = y*w;
			cO[yw] = 0.0f;
			float ks = 0.0f;
			for(i = -y; i <= w2; i++){
				cO[yw] += k[i]*cI[yw + i*w];
				ks += k[i];
			}
			cO[yw] /= ks;
		}

		for(y = h - w2; y != h; y++){
			int yw = y*w;
			cO[yw] = 0.0f;
			float ks = 0.0f;
			for(i = -w2; i <= h - 1 - y; i++){
				cO[yw] += k[i]*cI[yw + i*w];
				ks += k[i];
			}
			cO[yw] /= ks;
		}
	}

	free(k0);
	free(R);
}

//Returns the magnitude of the gradient of I. 0 to 1. (Ix, Iy) should be calculated with ForwardDifferenceGradient.
//Could be MUCH shorter, but this way is most efficient.
float *MagnitudeOfGradient(float *Ix, float *Iy, int w, int h){
	float *MoG = (float *)malloc(sizeof(float)*w*h);
	unsigned int x, y;
	unsigned int w1 = w - 1, h1 = h - 1;
	float ix, iy;

	//Interior.
	for(y = 1; y != h1; y++){
		float *rMoG = &MoG[y*w];
		float *rIx = &Ix[y*w];
		float *rIy = &Iy[y*w];
		for(x = 1; x != w1; x++){
			ix = 0.5f*(fabs(rIx[x]) + fabs(rIx[x - 1]));
			iy = 0.5f*(fabs(rIy[x]) + fabs(rIy[x - w]));
			rMoG[x] = sqrtf(ix*ix + iy*iy);
		}
	}

	//Top & bottom not including corners.
	y = h1*w;
	for(x = 1; x != w1; x++){
		ix = 0.5f*(fabs(Ix[x]) + fabs(Ix[x - 1]));
		iy = fabs(Iy[x]);
		MoG[x] = sqrtf(ix*ix + iy*iy);

		ix = 0.5f*(fabs(Ix[x + y]) + fabs(Ix[x - 1 + y]));
		iy = fabs(Iy[x + y - w]);
		MoG[x + y] = sqrtf(ix*ix + iy*iy);
	}

	//Left & right not including corners.
	for(y = w; y != h1*w; y += w){
		ix = fabs(Ix[y]);
		iy = 0.5f*(fabs(Iy[y]) + fabs(Iy[y - w]));
		MoG[y] = sqrtf(ix*ix + iy*iy);

		x = y + w1;
		ix = fabs(Ix[x - 1]);
		iy = 0.5f*(fabs(Iy[x - w]) + fabs(Iy[x]));
		MoG[x] = sqrtf(ix*ix + iy*iy);
	}

	//Corners.
	ix = fabs(Ix[0]);
	iy = fabs(Iy[0]);
	MoG[0] = sqrtf(ix*ix + iy*iy);

	ix = fabs(Ix[h1*w]);
	iy = fabs(Iy[h1*w - w]);
	MoG[h1*w] = sqrtf(ix*ix + iy*iy);

	ix = fabs(Ix[h*w - 2]);
	iy = fabs(Iy[h1*w - 1]);
	MoG[h*w - 1] = sqrtf(ix*ix + iy*iy);

	ix = fabs(Ix[w1 - 1]);
	iy = fabs(Iy[w1]);
	MoG[w1] = sqrtf(ix*ix + iy*iy);

	return MoG;
}

//Returns magnitude of gradient.
float *ModifyGradient(float *Ix, float *Iy, unsigned int w, unsigned int h, float alpha, float beta){
	//Form the magnitude of gradient first.
	unsigned int i, n = w*h;
	float *MoG = MagnitudeOfGradient(Ix, Iy, w, h);

	//Replace MoG with weighted geometric mean of its blurrings. Note that the exponents sum to one. Note also recursive blurring.
	//This is done to capture "edges at multiple scales" as the paper puts it, though the execution here is very different.
	float *MoGBlur = (float *)malloc(sizeof(float)*n);
	GaussianBlur(MoG, MoG, w, h, 3);
	GaussianBlur(MoG, MoGBlur, w, h, 5);
	for(i = 0; i != n; i++) MoG[i] = 	  powf(MoG[i], 0.05f)*
									  powf(MoGBlur[i], 0.1f);
	GaussianBlur(MoGBlur, MoGBlur, w, h, 9);
	for(i = 0; i != n; i++) MoG[i] *= powf(MoGBlur[i], 0.15f);
	GaussianBlur(MoGBlur, MoGBlur, w, h, 17);
	for(i = 0; i != n; i++) MoG[i] *= powf(MoGBlur[i], 0.3f);
	GaussianBlur(MoGBlur, MoGBlur, w, h, 33);
	for(i = 0; i != n; i++) MoG[i] *= powf(MoGBlur[i], 0.4f);
	free(MoGBlur);

	/* We're applying the curve Out = In^beta to the gradient. But the situation is slightly complicated by the fact that we use
	magnitude of gradient (MoG) for in and (Ix, Iy) for out. So rearrange: Out = In In^(beta - 1) and assume we can use
	(Ix, Iy)_Out = (Ix, Iy)_In MoG^(beta - 1) which makes sense: scale gradient with a power function. */

	if(alpha <= 0.0f){	//Zero or negative indicates automatic selection.
		alpha = 0.0f;
		for(i = 0; i != n; i++) alpha += MoG[i];
		alpha *= 0.4f/n;
	}
	beta = beta - 1.0f;
	alpha = powf(1.0f/alpha, beta);

	for(i = 0; i != n; i++){
		float phi = powf(0.0001f + MoG[i], beta)*alpha;
		Ix[i] *= phi;
		Iy[i] *= phi;
	}

	return MoG;
}

/*
Solves Ax = b by the Conjugate gradient method, where instead of feeding it the matrix A you feed it a function which
calculates A x where x is some vector. Stops when error < eps or when maximum iterates is reached.
Stops at n iterates if MaximumIterates = 0 since that many iterates gives exact solution. Applicable to symmetric A only.
Parameter pass can be passed through, containing whatever info you like it to contain (matrix info?).

Warning: will exit premature if too many iterates are done due to roundoff accumulation.
Roundoff accumulation can be fixed with a non-recursive calculation of r.
*/
float *SparseConjugateGradient(void Ax(float *Product, float *x, void *Pass), float *b, unsigned int n, bool OkToModify_b, 
	float *x, float eps, unsigned int MaximumIterates, void *Pass, void Preconditioner(float *Product, float *x, void *Pass)){
	unsigned int iterate, i;

	//r.
	float *r = (float *)malloc(sizeof(float)*n);
	memcpy(r, b, sizeof(float)*n);
	if(x == NULL) x = (float *)calloc(n, sizeof(float));		//Zero initial guess.
	else{
		Ax(r, x, Pass);
		for(i = 0; i != n; i++) r[i] = b[i] - r[i];		//r = b - A x.
	}

	//d et al.
	float *s = r, *q = b, *d = (float *)malloc(sizeof(float)*n);
	if(Preconditioner != NULL){
		s = (float *)malloc(sizeof(float)*n);
		Preconditioner(d, r, Pass);
	}else memcpy(d, r, sizeof(float)*n);
	if(!OkToModify_b) q = (float *)malloc(sizeof(float)*n);


	if(MaximumIterates == 0) MaximumIterates = n;
	float delta_new = 0.0f;
	for(i = 0; i != n; i++) delta_new += r[i]*d[i];
	const float delta_0 = delta_new;
	float delta_old, alpha, beta = 0.0f;

	eps *= eps;
	for(iterate = 0; iterate != MaximumIterates; iterate++){
		alpha = 0.0f;
		Ax(q, d, Pass);				//q = A d
		for(i = 0; i != n; i++)
			alpha += d[i]*b[i];		//dT q
		alpha = delta_new/alpha;


		for(i = 0; i != n; i++){
			x[i] += alpha*d[i];
			r[i] -= alpha*q[i];		//"Fast recursive formula", use explicit r = b - Ax occasionally?
		}
		if(Preconditioner != NULL) Preconditioner(s, r, Pass);

		delta_old = delta_new;
		delta_new = 0.0f;
		for(i = 0; i != n; i++) delta_new += r[i]*s[i];
		beta = delta_new/delta_old;
		if(delta_new < eps*delta_0) break;

		for(i = 0; i != n; i++) d[i] = s[i] + beta*d[i];
	}

	if(q != b) free(q);
	if(s != r) free(s);
	free(r);
	free(d);
	return x;	//r d x.
}

//Calculates the product AI where A is the five point discretization of the Laplacian in a Neumann homogeneous bounded rectangle.
void Laplacian5ptNR(float *AI, float *I, void *wh){
	unsigned int w = ((unsigned int *)wh)[0];
	unsigned int h = ((unsigned int *)wh)[1];
	unsigned int w1 = w - 1;
	unsigned int h1w = (h - 1)*w;
	unsigned int i, iend;

	//The interior. Isn't this an awesome pair of loops?
	for(i = w; i != h1w; i++){
		iend = w1 + i++;
		while(i != iend)
			AI[i++] = I[i - w] + I[i - 1] - 4.0f*I[i] + I[i + 1] + I[i + w];
	}

	//Top row. These four edges calculations exclude corners.
	for(i = 1; i != w1; i++)
		AI[i] = I[i - 1] - 3.0f*I[i] + I[i + 1] + I[i + w];

	//Bottom row.
	iend = h1w + w1;
	for(i = h1w + 1; i != iend; i++)
		AI[i] = I[i - w] + I[i - 1] - 3.0f*I[i] + I[i + 1];

	//Left column.
	for(i = w; i != h1w; i += w)
		AI[i] = I[i - w] - 3.0f*I[i] + I[i + 1] + I[i + w];

	//Right column.
	iend = h1w + w1;
	for(i = w + w1; i != iend; i += w)
		AI[i] = I[i - w] + I[i - 1] - 3.0f*I[i] + I[i + w];

	//Corners.
	AI[0] = -2.0f*I[0] + I[1] + I[w];
	AI[w1] = I[w1 - 1] - 2.0f*I[w1] + I[w1 + w];
	AI[h1w] = I[h1w - w] - 2.0f*I[h1w] + I[h1w + 1];
	AI[h*w - 1] = I[h*w - 1 - w] + I[h*w - 1 - 1] - 2.0f*I[h*w - 1];


/*	Could use this to cover all pixels. Slower, though. Educational to take a good look at this comment.
	for(unsigned int y = 0; y != h; y++){
		for(unsigned int x = 0; x != w; x++){
			i = x + y*w;
			AI[i] = 0.0f;
			
			if(x > 0)	AI[i] += -I[i] + I[i - 1];
			if(y > 0)	AI[i] += -I[i] + I[i - w];
			if(x < w1)	AI[i] += -I[i] + I[i + 1];
			if(y < h1)	AI[i] += -I[i] + I[i + w];
		}
	} */
}

void Laplacian5ptNRPreconditioner(float *MiI, float *I, void *wh){
	unsigned int w = ((unsigned int *)wh)[0];
	unsigned int h = ((unsigned int *)wh)[1];
	unsigned int w1 = w - 1;
	unsigned int h1w = (h - 1)*w;
	unsigned int i, iend;

	for(i = w; i != h1w; i++){
		iend = w1 + i++;
		while(i != iend)
			MiI[i++] = I[i]/4.0f;
	}

	for(i = 1; i != w1; i++){
		MiI[i] = I[i]/3.0f;
		MiI[i + h1w] = I[i + h1w]/3.0f;
	}

	for(i = w; i != h1w; i += w){
		MiI[i] = I[i]/3.0f;
		MiI[i + w1] = I[i + w1]/3.0f;
	}

	MiI[0] = I[0]/2.0f;
	MiI[w1] = I[w1]/2.0f;
	MiI[h1w] = I[h1w]/2.0f;
	MiI[h*w - 1] = I[h*w - 1]/2.0f;
}


//Returns half sized image by averaging 2 x 2 squares. Output width and height are w >> 1 and h >> 1.
float *HalfSize(float *I, unsigned int w, unsigned int h){
	unsigned int ww = w >> 1, hh = h >> 1;
	float *Ih = (float *)malloc(sizeof(float)*ww*hh);

	//Note that exact x/2 will always be equal or greater than x >> 1. Output image is then, with odd dimensions, smaller then half size.
	for(unsigned int y = 0; y != hh; y++){
		float *rI0 = &I[2*y*w];
		float *rI1 = &I[(2*y + 1)*w];
		float *rIh = &Ih[y*ww];
		for(unsigned int x = 0; x != ww; x++)
			rIh[x] = 0.25f*(rI0[2*x] + rI0[2*x + 1] + rI1[2*x] + rI1[2*x + 1]);
	}

	return Ih;
}

//w and h are output width and height. Note that, for example, 1501 >> 1 is 750 but 750 << 1 is 1500.
//By explicitly specifying output size the off by one issue is handled. In is assumed w >> 1 by h >> 1.
void DoubleSize(float *Out, unsigned int w, unsigned int h, float *In){
	unsigned int ws = w >> 1;
	unsigned int hs = h >> 1;
	unsigned int we = w - (w & 1);
	unsigned int he = h - (h & 1);

	for(unsigned int y = 0; y != he; y++){
		float *rO = &Out[y*w];
		float *rI = &In[(y >> 1)*ws];
		for(unsigned int x = 0; x != we; x++)
			rO[x] = rI[x >> 1];
	}

	if(we != w)
		for(unsigned int y = 0; y != he; y++)
			Out[w - 1 + y*w] = Out[w - 2 + y*w];
	if(he != h)
		for(unsigned int x = 0; x != w; x++)
			Out[x + (h - 1)*w] = Out[x + (h - 2)*w];
}


//This function does what it's name describes using the parameter beta. beta < 1.0 corresponds to compression.
//alpha <= 0.0 makes it automatically set (which seems to work pretty good).
void CompressDynamicRange(float *I, unsigned int w, unsigned int h, float alpha, float beta, bool WorkOnLog){
	printf("Dynamic range compression: ");
	double t = (double)clock()/(double)CLOCKS_PER_SEC;
	float logadd = 0.001f;

	//Get the min and max values and optionally take the logarithm.
	unsigned int i, n = w*h;

	float IMin = FLT_MAX, IMax = -FLT_MAX;
	if(WorkOnLog)
		for(i = 0; i != n; i++){
			if(I[i] > IMax) IMax = I[i];
			if(I[i] < IMin) IMin = I[i];
			I[i] = logf(I[i] + logadd);
		}
	else
		for(i = 0; i != n; i++){
			if(I[i] > IMax) IMax = I[i];
			if(I[i] < IMin) IMin = I[i];
		}
	//By working with the logarithm, "finite difference" becomes effectively "logarithmic contrast ratio".


	/* Unfortunately this problem takes very long to solve at any sensible resolution. But the solution time is cut if a good initial
	guess is used. So, we solve at multiple resolutions: solve the problem at a small resolution, upsize that, solve, upsize, etc. */

	//So, start by building a Laplacian pyramid with N levels so that topmost is smaller then 64 x 64.
	unsigned int level = 0;
	while((h >> level) > 64) level++;
	const unsigned int N = level;

	const unsigned int W = w, H = h;		//We'll modify w and h but these explicitly won't change.
	float **L = (float **)malloc(sizeof(float *)*N);
	float *img = I;
	for(level = 0; level != N; level++){
		if(level != 0){
			//Prepare this level.
			float *img_ = HalfSize(img, w, h);
			if(img != I) free(img);
			img = img_;

			w = W >> level;
			h = H >> level;
		}
		n = w*h;

		//Take the gradient of the image & modify it.
		float *Ix = (float *)malloc(sizeof(float)*n);
		float *Iy = (float *)malloc(sizeof(float)*n);
		ForwardDifferenceGradient(Ix, Iy, w, h, img);
		L[level] = ModifyGradient(Ix, Iy, w, h, alpha, beta);	//Reuse the memory under a new name.

/*
		float *M = L[level];
		IMin = FLT_MAX, IMax = -FLT_MAX;
		for(i = 0; i != n; i++){
			if(M[i] > IMax) IMax = M[i];
			if(M[i] < IMin) IMin = M[i];
		}
		for(i = 0; i != n; i++)
			I[i] = (M[i] - IMin)/(IMax - IMin);
		return;
//*/

		//Take backward divergence of gradient, giving Laplacian.
		BackwardDifferenceDivergence(L[level], Ix, Iy, w, h);
		free(Ix);
		free(Iy);
	}

	/* Note that at this point img was not deleted, and that it contains I at the resolution of L[N - 1].
	So now we have the modified Laplacian calculated at many different scales. Mind you, calculating the Laplacian once and then
	resizing that doesn't work. It's possible to write a proper Laplacian resizer but only in 1D. */

	//Now solve smallest to largest.
	level = N;
	printf("solution done at level ");
	while(level != 0){
		level--;
		w = W >> level;
		h = H >> level;
		n = w*h;

		if(level != N - 1){
			float *img_;
			if(level != 0) img_ = (float *)malloc(sizeof(float)*n);
			else img_ = I;
			DoubleSize(img_, w, h, img);
			free(img);
			img = img_;
		}

		//Undo Laplacian (divergence of gradient) to get pixels corresponding to modified gradient.
		unsigned int wh[2] = {w, h};	//Form pass through parameter which contains w and n.
		float convergence = 0.001f*(N - 1 - level + 0.001f)/(N - 1);		//Enforce stronger convergence at lower resolutions.
		SparseConjugateGradient(Laplacian5ptNR, L[level], n, true, img,
			convergence, (level + 1)*200, (void *)wh, NULL);
		//Note: typical number of iterates is 20 - 200; more for higher levels, less for lower.

		free(L[level]);
		if(level != 0) printf("%u, ", level);
		else  printf("and %u; ", level);
	}
	free(L);


	//Restore input range (by linear mapping) and exponentiate.

	float OutIMin = FLT_MAX, OutIMax = -FLT_MAX;
	if(WorkOnLog)
		for(i = 0; i != n; i++){
			I[i] = expf(I[i]) - logadd;
			if(I[i] > OutIMax) OutIMax = I[i];
			if(I[i] < OutIMin) OutIMin = I[i];
		}
	else
		for(i = 0; i != n; i++){
			if(I[i] > OutIMax) OutIMax = I[i];
			if(I[i] < OutIMin) OutIMin = I[i];
		}

	float scale = (IMax - IMin)/(OutIMax - OutIMin);
	float shift = IMin - OutIMin*scale;
	for(i = 0; i != n; i++)
		I[i] = I[i]*scale + shift;

	printf("took %.1f seconds.\n", (double)clock()/(double)CLOCKS_PER_SEC - t);
}

