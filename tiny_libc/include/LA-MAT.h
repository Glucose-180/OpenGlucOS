#ifndef __LA_MAT_H__
#define __LA_MAT_H__
	struct RN {
		long p;
		long q;
	};
	struct RN Mult(struct RN, struct RN);
	struct RN Reci(struct RN);
	struct RN Divd(struct RN, struct RN);
	struct RN Plus(struct RN, struct RN);
	struct RN Minu(struct RN, struct RN);
	struct RN Power(struct RN, long);
	//double Ftod(struct RN);
	//struct RN Dtof(double, unsigned);
	struct RN Frac(long, long);
	void Form(struct RN*);
	unsigned long gcd(long, long);
	unsigned long lcm(long, long);
	int IsPr(long);
	void DisplayRN(struct RN);
	struct RN ator(char str[]);

typedef struct matrix {
	unsigned int row;
	unsigned int col;
	struct RN** dat;
}matrix;
extern const matrix MAT_NULL;
matrix string_mat(char* str);
void mat_print(matrix mat);
matrix mat_create(unsigned int m, unsigned int n);
void mat_delete(matrix* mat);
void mat_copy(matrix* mt, matrix* mf);
void mat_init(matrix* mat);
matrix mat_tran(matrix mat);
matrix mat_plus(matrix mat1, matrix mat2);
matrix mat_minu(matrix mat1, matrix mat2);
matrix mat_mult(matrix mat1, matrix mat2);
matrix mat_times(matrix mat, struct RN k);
unsigned int mat_gauss_elim(matrix* mat, char crd);
struct RN mat_det(matrix mat);
unsigned int mat_rank(matrix mat);
matrix mat_inv(matrix mat);

#endif
