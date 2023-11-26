
#include <stdio.h>
#include <stdlib.h>
#include "LA-MAT.h"
#define BUFMAX 15

static inline long labs_g(long x)
{
	if (x < 0)
		return -x;
	else
		return x;
}

static inline long lpow(long base, long n)
{
	long rt = 1L;
	while (n-- > 0L)
		rt *= base;
	return rt;
}

struct RN Mult(struct RN R1, struct RN R2)  // R1 * R2
{
	struct RN rR;
	rR.p = R1.p * R2.p;
	rR.q = R1.q * R2.q;
	Form(&rR);
	return rR;
}

struct RN Reci(struct RN R)  // 1 / R
{
	struct RN rR;
	Form(&R);
	if (R.p == 0)
	{
		rR.p = 0;
		rR.q = 1;
	}
	else
	{
		rR.p = R.q * (R.p < 0 ? -1 : 1);
		rR.q = labs_g(R.p);
	}
	return rR;
}

struct RN Divd(struct RN R1, struct RN R2)  // R1 / R2
{
	return Mult(R1, Reci(R2));
}

struct RN Plus(struct RN R1, struct RN R2)  //  R1 + R2
{
	struct RN rR;
	Form(&R1);
	Form(&R2);
	rR.q = lcm(R1.q, R2.q);
	rR.p = R1.p * (rR.q / R1.q) + R2.p * (rR.q / R2.q);
	Form(&rR);
	return rR;
}

struct RN Minu(struct RN R1, struct RN R2)  //  R1 - R2
{
	return Plus(R1, Mult(Frac(-1, 1), R2));
}

struct RN Power(struct RN R, long n)  // R ^ n
{
	struct RN rR;
	if (n!=0)
	{
		R = (n > 0 ? R : Reci(R));
		rR.p = lpow(R.p, n);
		rR.q = lpow(R.q, n);
	}
	else
		rR.p = rR.q = 1;
	return rR;
}

void Form(struct RN* R)
{
	if (R->p == 0)
		R->q = 1;
	else
	{
		long sgn = 1, rp, rq;
		unsigned long g;
		rp = R->p;
		rq = R->q;
		sgn = ((rp > 0) != (rq > 0) ? -1 : 1);
		g = gcd(rp, rq);
		R->p = sgn * labs_g(rp / (long)g);
		R->q = labs_g(rq / (long)g);
	}
}

struct RN Frac(long ip, long iq)  //  ip / iq
{
	struct RN rR;
	rR.p = ip;
	rR.q = iq;
	return rR;
}

unsigned long gcd(long im, long in)
{
	unsigned long r1, r2, r;
	r1 = labs_g(im);
	r2 = labs_g(in);
	r = r1 % r2;
	while (r != 0)
	{
		r1 = r2;
		r2 = r;
		r = r1 % r2;
	}
	return r2;
}

unsigned long lcm(long im, long in)
{
	return labs_g(im * in / gcd(im, in));
}

int IsPr(long in)  //  Is in a prime number?
{
	long i = 2;
	in = labs_g(in);
	if (in <= 1)
		return 0;
	for (i = 2; i < in; i++)
	{
		if (!(in % i))
			return 0;
	}
	return 1;
}

void DisplayRN(struct RN R)
{
	Form(&R);
	if (R.q != 1)
		printf("%ld/%ld", R.p, R.q);
	else
		printf("%ld", R.p);
}

struct RN ator(char str[])
{
	unsigned i, fl;
	char buf1[BUFMAX] = { '\0' };
	char buf2[BUFMAX] = { '\0' };
	struct RN rR;
	for (fl = 0; str[fl] && str[fl] != '/'; fl++);
	if (!str[fl])  // No frac line
	{
		rR = Frac(atoi(str), 1);
	}
	else
	{
		for (i = 0; (buf1[i] = str[i]) != '/'; i++);
		buf1[i] = 0;
		for (i = 1; i < BUFMAX && (buf2[i - 1] = str[fl + i]); i++);
		//buf[i - 1] = '\0';
		rR = Frac(atoi(buf1), atoi(buf2));
	}
	Form(&rR);
	return rR;
}


const matrix MAT_NULL = { 0,0,NULL }; /*Null matrix*/

/*String to matrix
Format:
[_,_,_;_,_,_;...]*/
matrix string_mat(char* str)
{
	char* str0 = str;  /* mark the next of [ */
	unsigned int i = 0, j = 0, l = 0;
	unsigned int m = 1, n = 1;
	char buf[BUFMAX] = { '\0' };
	matrix rm = MAT_NULL;
	while (*str != '[' && *str != '\0')
		str++;
	if (*str == '\0')
		return MAT_NULL;  /* [ not found */
	str0 = ++str;
	for (; *str != '\0' && *str != ']'; str++)
	{
		if (*str == ',')
			n++;
		else if (*str == ';')
		{
			m++;
			break;  /*only n=1 has been known*/
		}
	}
	if (*str == '\0')
		return MAT_NULL;  /* ] not found */
	else if (*str == ';')
	{
		for (str++; *str != '\0' && *str != ']'; str++)
		{
			if (*str == ';')
				m++;
		}
		if (*str == '\0')
			return MAT_NULL;  /* ] not found */
	}
	/* now both m and n has been known */
	rm = mat_create(m, n);
	for (i = j = 0, str = str0; *(str - 1) != ']'; str++)
	{
		switch (*str)
		{
		case ',':
			if (j + 1 >= n)
				return MAT_NULL;
			buf[l] = '\0';
			l = 0;
			rm.dat[i][j++] = ator(buf);
			break;
		case';':
		case']':
			buf[l] = '\0';
			l = 0;
			rm.dat[i++][j] = ator(buf);
			j = 0;
			break;
		default:
			if (l + 1 < BUFMAX)
				buf[l++] = *str;
			else
				buf[BUFMAX - 1] = '\0';
			break;
		}
	}
	return rm;
}

/* print a matrix */
void mat_print(matrix mat)
{
	unsigned int i = 0, j = 0;
	for (; i < mat.row; i++)
	{
		for (j = 0; j < mat.col; j++)
		{
			printf("       ");
			DisplayRN(mat.dat[i][j]);
		}
		printf("\n");
	}
}

/* Create an m*n matrix */
matrix mat_create(unsigned int m, unsigned int n)
{
	matrix rm = { m,n,NULL };
	unsigned int i, j;
	rm.dat = (struct RN**)malloc(m * sizeof(struct RN*));
	for (i = 0; i < m; i++)
	{
		rm.dat[i] = (struct RN*)malloc(n * sizeof(struct RN));
		for (j = 0; j < n; j++)
			rm.dat[i][j] = Frac(0, 0);
	}
	return rm;
}

/*Delete a matrix*/
void mat_delete(matrix* mat)
{
	unsigned int i;
	for (i = 0; i < mat->row; i++)
		free(mat->dat[i]);
	free(mat->dat);
	*mat = MAT_NULL;
}

/*Copy a matrix from mf to mt*/
void mat_copy(matrix* mt, matrix* mf)
{
	unsigned int i, j;
	mt->row = mf->row;
	mt->col = mf->col;
	for (i = 0; i < mt->row; i++)
		for (j = 0; j < mt->col; j++)
			mt->dat[i][j] = mf->dat[i][j];
}

/*Set a matrix to ZERO*/
void mat_init(matrix* mat)
{
	unsigned int i, j;
	for (i = 0; i < mat->row; i++)
		for (j = 0; j < mat->col; j++)
			mat->dat[i][j] = Frac(0, 0);
}

/*Matrix transposition*/
matrix mat_tran(matrix mat)
{
	unsigned int i, j;
	matrix rm = mat_create(mat.col, mat.row);
	for (i = 0; i < mat.col; i++)
		for (j = 0; j < mat.row; j++)
			rm.dat[i][j] = mat.dat[j][i];
	return rm;
}

/* add two matrixes */
matrix mat_plus(matrix mat1, matrix mat2)
{
	unsigned int i = mat1.row, j = mat1.col;
	matrix rm = MAT_NULL;
	if (i != mat2.row || j != mat2.col)
		return MAT_NULL;  /* row, col are not the same */
	rm = mat_create(i, j);
	while (i--)
	{
		j = mat1.col;
		while (j--)
			rm.dat[i][j] = Plus(mat1.dat[i][j], mat2.dat[i][j]);
	}
	return rm;
}

/* minus two matrixes*/
matrix mat_minu(matrix mat1, matrix mat2)
{
	unsigned int i = mat1.row, j = mat1.col;
	matrix rm = MAT_NULL;
	if (i != mat2.row || j != mat2.col)
		return MAT_NULL;  /* row, col are not the same */
	rm = mat_create(i, j);
	while (i--)
	{
		j = mat1.col;
		while (j--)
			rm.dat[i][j] = Minu(mat1.dat[i][j], mat2.dat[i][j]);
	}
	return rm;
}

/* multiple two matrixes */
/* mat1: r*m; mat2: m*n */
matrix mat_mult(matrix mat1, matrix mat2)
{
	unsigned int r = mat1.row, m = mat1.col, n = mat2.col;
	unsigned int k = 0;
	matrix rm = MAT_NULL;
	if (m != mat2.row)
		return MAT_NULL;  /* mat1.col != mat2.row */
	rm = mat_create(r, n);
	while (r--)
	{
		n = mat2.col;
		while (n--)
			for (k = 0; k < m; k++)
				rm.dat[r][n] = Plus(rm.dat[r][n], Mult(mat1.dat[r][k], mat2.dat[k][n]));
	}
	return rm;
}

/* mult a matrix with a number */
matrix mat_times(matrix mat, struct RN k)
{
	matrix rm = mat_create(mat.row, mat.col);
	unsigned int i, j;
	for (i = 0; i < mat.row; i++)
		for (j = 0; j < mat.col; j++)
			rm.dat[i][j] = Mult(mat.dat[i][j], k);
	return rm;
}

/* Gaussian Elimination */
unsigned int mat_gauss_elim(matrix* mat, char crd) /* crd: 'd'-det, 'r'-rank */
{
	unsigned int sct = 0; /* swap count */
	unsigned int r = 0, c = 0; /* row, col */
	unsigned int i = 0, j = 0;
	char flag = 0;
	struct RN* temp;
	struct RN k;

	for (; (c = r) < mat->row - 1 && r < mat->col; r++)
	{
		if (mat->dat[r][c].p == 0)
		{
			for (flag = 0; c < mat->col && flag == 0; c++)
			{
				for (i = r + 1; i < mat->row; i++)
					if (mat->dat[i][c].p != 0)
					{
						temp = mat->dat[i];
						mat->dat[i] = mat->dat[r];
						mat->dat[r] = temp;
						sct++;
						flag = 1; /* swap has finished */
						break;
					}
			}
			c--;
		}
		if (mat->dat[r][c].p != 0)
			for (i = r + 1; i < mat->row; i++)
			{
				k = Divd(mat->dat[i][c], mat->dat[r][c]);
				for (j = c; j < mat->col; j++)
				{
					mat->dat[i][j] = Minu(mat->dat[i][j], Mult(k, mat->dat[r][j]));
				}
			}
	}
	if (crd == 'd') /* you want the det */
		return sct;
	/* now get the rank */
	for (i = mat->row; i > 0; i--)
	{
		for (j = 0; j < mat->col; j++)
		{
			if (mat->dat[i - 1][j].p != 0)
				return i;
		}
	}
	return 0;
}

/* calc the determinate of a matrix */
struct RN mat_det(matrix mat)
{
	matrix mtp = MAT_NULL;
	struct RN rR = Frac(1, 1);
	unsigned int i = mat.row;
	if (mat.col != mat.row || mat.col == 0 || mat.row == 0)
		return Frac(0, 0);  /* not a square */
	switch (mat.row)
	{
	case 1:
		return mat.dat[0][0];
		break;
	case 2:
		return Minu(Mult(mat.dat[0][0], mat.dat[1][1]), Mult(mat.dat[1][0], mat.dat[0][1]));
		break;
	default:
		mtp = mat_create(mat.row, mat.col);
		mat_copy(&mtp, &mat);
		if (mat_gauss_elim(&mtp, 'd') % 2 != 0)
			rR = Frac(-1, 1);
		while (i--)
			rR = Mult(rR, mtp.dat[i][i]);
		mat_delete(&mtp);
		return rR;
		break;
	}
}

/* calc the rank of a matrix */
unsigned int mat_rank(matrix mat)
{
	matrix mtp = mat_create(mat.row, mat.col);
	unsigned int rk;
	mat_copy(&mtp, &mat);
	rk = mat_gauss_elim(&mtp, 'r');
	mat_delete(&mtp);
	return rk;
}

/* get the inverse of a matrix */
matrix mat_inv(matrix mat)
{
	matrix mtp = MAT_NULL, rm = MAT_NULL;
	unsigned int n = mat.row;
	unsigned int r, i, j;
	struct RN* temp;
	struct RN k;
	if (n != mat.col)
		return MAT_NULL;
	mtp = mat_create(n, n);
	mat_copy(&mtp, &mat);
	rm = mat_create(n, n);
	for (r = 0; r < n; r++)
		rm.dat[r][r] = Frac(1, 1); /* create I_n */
	/* gauss_elim: up to down */
	for (r = 0; r < n; r++)
	{
		if (mtp.dat[r][r].p == 0)
		{
			for (i = r + 1; i < n; i++)
			{
				if (mtp.dat[i][r].p != 0)
				{
					/* swap mtp and rm */
					temp = mtp.dat[i];
					mtp.dat[i] = mtp.dat[r];
					mtp.dat[r] = temp;
					temp = rm.dat[i];
					rm.dat[i] = rm.dat[r];
					rm.dat[r] = temp;
					break;
				}
			}
		}
		if (mtp.dat[r][r].p == 0)  /* no inverse */
		{
			mat_delete(&mtp);
			mat_delete(&rm);
			return MAT_NULL;
		}
		for (i = r + 1; i < n; i++)
		{
			k = Divd(mtp.dat[i][r], mtp.dat[r][r]);
			for (j = r; j < n; j++)
				mtp.dat[i][j] = Minu(mtp.dat[i][j], Mult(k, mtp.dat[r][j]));
			for (j = 0; j < n; j++)
				rm.dat[i][j] = Minu(rm.dat[i][j], Mult(k, rm.dat[r][j]));
		}
	}
	/* gauss_elim: down to up */
	for (r = n - 1; r > 0; r--)
	{
		for (i = 1; i <= r; i++)
		{
			k = Divd(mtp.dat[r - i][r], mtp.dat[r][r]);
			mtp.dat[r - i][r] = Minu(mtp.dat[r - i][r], Mult(k, mtp.dat[r][r]));
			for (j = 0; j < n; j++)
				rm.dat[r - i][j] = Minu(rm.dat[r - i][j], Mult(k, rm.dat[r][j]));
		}
	}
	/* times every row */
	for (r = 0; r < n; r++)
	{
		if (mtp.dat[r][r].p != 1 || mtp.dat[r][r].q != 1)
		{
			k = Reci(mtp.dat[r][r]);
			for (j = 0; j < n; j++)
				rm.dat[r][j] = Mult(k, rm.dat[r][j]);
		}
	}
	mat_delete(&mtp);
	return rm;
}
