/*
	common.h

	this file is part of gf_mds

	author: Alessio Ribeca <a.ribeca@unitus.it>
	owner: DIBAF - University of Tuscia, Viterbo, Italy

	scientific contact: Dario Papale <darpap@unitus.it>
*/

/*
Copyright 2014-2019 DIBAF - University of Tuscia, Viterbo, Italy

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef COMMON_H
#define COMMON_H

/* c++ handling */
#ifdef __cplusplus
	extern "C" {
#endif

/* includes */
#include <math.h>

#ifndef isnan
	# define isnan(x) \
	  (sizeof (x) == sizeof (long double) ? isnan_ld (x) \
	   : sizeof (x) == sizeof (double) ? isnan_d (x) \
	   : isnan_f (x))
#endif

#ifndef isinf
	# define isinf(x) \
	  (sizeof (x) == sizeof (long double) ? isinf_ld (x) \
	   : sizeof (x) == sizeof (double) ? isinf_d (x) \
	   : isinf_f (x))
#endif

/* defines */
#define IS_LEAP_YEAR(x)					(((x) % 4 == 0 && (x) % 100 != 0) || (x) % 400 == 0)
#define SIZEOF_ARRAY(a)					sizeof((a))/sizeof((a)[0])
#define IS_INVALID_VALUE(value)			((INVALID_VALUE==(int)(value)))
#define IS_MISSING_VALUE(value)			((MISSING_VALUE==(int)(value)))
#define IS_VALID_VALUE(value)			(((value)==(value))&&(!isinf((value))))
#define ARE_FLOATS_EQUAL(a,b)			(((a)==(b)))										/* TODO : CHECK IT */
#define IS_FLAG_SET(v, m)				(((v) & (m)) == (m))
#ifndef MAX
#define MAX(a, b)						(((a) > (b)) ? (a) : (b))
#endif
#define ABS(a)							(((a) < 0) ? -(a) : (a))
#define SQUARE(x)						((x)*(x))
#define VARTOSTRING(x)					(#x)
#define PRINT_TOKEN(token)				printf(#token " is %d", token)
#define PRINT_VAR(var)					printf("%s=%d\n",#var,var);
#if defined (_WIN32)
#define FOLDER_DELIMITER '\\'
#else
#define FOLDER_DELIMITER '/'
#endif
#define PREC		double
#define STRTOD		strtod
#define SQRT		sqrt
#define FABS		fabs
/* we add 0.5 so if x is > 0.5 we truncate to next integer */
#define ROUND(x)	((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
	
/* enumeration for time resolution */
enum {
	SPOT_TIMERES = 0,
	QUATERHOURLY_TIMERES,
	HALFHOURLY_TIMERES,
	HOURLY_TIMERES,
	DAILY_TIMERES,
	MONTHLY_TIMERES,

	TIMERES_SIZE
};

/* gf */
enum {
	GF_TOFILL_VALID		= 1 << 0 ,
	GF_VALUE1_VALID		= 1 << 1 ,
	GF_VALUE2_VALID		= 1 << 2 ,
	GF_VALUE3_VALID		= 1 << 3 ,
	GF_ALL_VALID		= GF_TOFILL_VALID|GF_VALUE1_VALID|GF_VALUE2_VALID|GF_VALUE3_VALID
};

/* */
enum {
	GF_ALL_METHOD = 0,
	GF_VALUE1_METHOD,
	GF_TOFILL_METHOD,

	GF_METHODS
};

/* constants */
#define INVALID_VALUE		-9999
#define LEAP_YEAR_ROWS		17568
#define YEAR_ROWS			17520
#define BUFFER_SIZE			4096
#define PATH_SIZE			1024			/* TODO : CHECK IT */
#define FILENAME_SIZE		256				/* TODO : CHECK IT */

/* constants for gapfilling */
#define GF_DRIVER_1_TOLERANCE_MIN			20.0
#define GF_DRIVER_1_TOLERANCE_MAX			50.0
#define GF_DRIVER_2A_TOLERANCE_MIN			2.5
#define GF_DRIVER_2A_TOLERANCE_MAX			INVALID_VALUE
#define GF_DRIVER_2B_TOLERANCE_MIN			5.0
#define GF_DRIVER_2B_TOLERANCE_MAX			INVALID_VALUE
#define GF_TOKEN_LENGTH_MAX					32
#define GF_ROWS_MIN_MIN						0
#define GF_ROWS_MIN							0
#define GF_ROWS_MIN_MAX						10000

/* */
#define TIMESTAMP_STRING		"TIMESTAMP"
#define TIMESTAMP_START_STRING	TIMESTAMP_STRING"_START"
#define TIMESTAMP_END_STRING	TIMESTAMP_STRING"_END"
#define TIMESTAMP_HEADER		TIMESTAMP_START_STRING","TIMESTAMP_END_STRING

/* structures */
typedef struct {
	char name[FILENAME_SIZE+1];
	char path[PATH_SIZE+1];
	char fullpath[PATH_SIZE+1];
} LIST;

/* */
typedef struct {
	LIST *list;
	int count;
} FILES;

/* */
typedef struct {
	char *name;
	int (*f)(char *, char *, void *);
	void *p;
} ARGUMENT;

/* */
typedef struct {
	int YYYY;
	int MM;
	int DD;
	int hh;
	int mm;
	int ss;
} TIMESTAMP;

/* structure for gapfilling */
typedef struct {
	char mask;
	PREC similiar;
	PREC stddev;
	PREC filled;
	int quality;
	int time_window;
	int samples_count;
	int method;
} GF_ROW;

/* extern */
extern const char err_out_of_memory[];

/* prototypes */
int isnan_f  (float       x);
int isnan_d  (double      x);
int isnan_ld (long double x);
int isinf_f  (float       x);
int isinf_d  (double      x);
int isinf_ld (long double x);

PREC convert_string_to_prec(const char *const string, int *const error);
int convert_string_to_int(const char *const string, int *const error);
FILES *get_files(const char *const program_path, char *string, int *const count, int *const error);
void free_files(FILES *files, const int count);
int parse_arguments(int argc, char *argv[], const ARGUMENT *const args, const int arg_count);
int string_compare_i(const char *str1, const char *str2);
int string_n_compare_i(const char *str1, const char *str2, const int len);
char *string_copy(const char *const string);
char *string_tokenizer(char *string, const char *delimiters, char **p);
int string_concat(char *const string, const char *const string_to_add, const int size);
char *get_current_directory(void);
int add_char_to_string(char *const string, char c, const int size);
int file_exists(const char *const file);
int path_exists(const char *const path);
int compare_prec(const void * a, const void * b);
char *tokenizer(char *string, char *delimiter, char **p);
int create_dir(char *Path);

TIMESTAMP *get_timestamp(const char *const string);
int get_row_by_timestamp(const TIMESTAMP *const t, const int timeres);
int get_year_from_timestamp_string(const char *const string);

#define timestamp_start_by_row(r,y,h) timestamp_get_by_row((r),(y),(h),1)
#define timestamp_end_by_row(r,y,h) timestamp_get_by_row((r),(y),(h),0)
TIMESTAMP *timestamp_get_by_row(int row, int yy, const int hourly_dataset, const int start);
#define timestamp_start_by_row_s(r,y,h) timestamp_get_by_row_s((r),(y),(h),1)
#define timestamp_end_by_row_s(r,y,h) timestamp_get_by_row_s((r),(y),(h),0)
char *timestamp_get_by_row_s(int row, int yy, const int hourly_dataset, const int start);

#define timestamp_start_ww_by_row(r,y,h) timestamp_ww_get_by_row((r),(y),(h),1)
#define timestamp_end_ww_by_row(r,y,h) timestamp_ww_get_by_row((r),(y),(h),0)
TIMESTAMP *timestamp_ww_get_by_row(int row, int yy, const int hourly_dataset, int start);
#define timestamp_start_ww_by_row_s(r,y,h) timestamp_get_by_row_s((r),(y),(h),1)
#define timestamp_end_ww_by_row_s(r,y,h) timestamp_get_by_row_s((r),(y),(h),0)
char *timestamp_ww_get_by_row_s(int row, int yy, const int hourly_dataset, const int start);

/* gf */
GF_ROW *gf_mds(		PREC *values,
					const int struct_size,
					const int rows_count,
					const int columns_count,
					const int hourly_dataset,
					PREC value1_tolerance_min,
					PREC value1_tolerance_max,
					PREC value2_tolerance_min,
					PREC value2_tolerance_max,
					PREC value3_tolerance_min,
					PREC value3_tolerance_max,
					const int tofill_column,
					const int value1_column,
					const int value2_column,
					const int value3_column,
					const int values_min,
					const int compute_hat,
					int *no_gaps_filled_count
);

GF_ROW *gf_mds_with_qc(	PREC *values,
						const int struct_size,
						const int rows_count,
						const int columns_count,
						const int hourly_dataset,
						PREC value1_tolerance_min,
						PREC value1_tolerance_max,
						PREC value2_tolerance_min,
						PREC value2_tolerance_max,
						PREC value3_tolerance_min,
						PREC value3_tolerance_max,
						const int tofill_column,
						const int value1_column,
						const int value2_column,
						const int value3_column,
						const int value1_qc_column,
						const int value2_qc_column,
						const int value3_qc_column,
						const int qc_thrs,
						const int values_min,
						const int compute_hat,
						int *no_gaps_filled_count
);

GF_ROW *gf_mds_with_bounds(	PREC *values,
							const int struct_size,
							const int rows_count,
							const int columns_count,
							const int hourly_dataset,
							PREC value1_tolerance_min,
							PREC value1_tolerance_max,
							PREC value2_tolerance_min,
							PREC value2_tolerance_max,
							PREC value3_tolerance_min,
							PREC value3_tolerance_max,
							const int tofill_column,
							const int value1_column,
							const int value2_column,
							const int value3_column,
							const int value1_qc_column,
							const int value2_qc_column,
							const int value3_qc_column,
							const int qc_thrs,
							const int values_min,
							const int compute_hat,
							int start_row,
							int end_row,
							int *no_gaps_filled_count
);
PREC gf_get_similiar_standard_deviation(const GF_ROW *const gf_rows, const int rows_count);
PREC gf_get_similiar_median(const GF_ROW *const gf_rows, const int rows_count, int *const error);

/*  */
int get_rows_count_by_timeres(const int timeres, const int year);
int get_rows_per_day_by_timeres(const int timeres);
int get_rows_per_hour_by_timeres(const int timeres);

int get_valid_line_from_file(FILE *const f, char *buffer, const int size);

/* */
void check_memory_leak(void);

int check_timestamp(const TIMESTAMP* const p);
int timestamp_difference_in_seconds(const TIMESTAMP* const p1, const TIMESTAMP* const p2);

#if defined (_WIN32) && defined (_DEBUG) 
void dump_memory_leaks(void);
#endif

/* c++ handling */
#ifdef __cplusplus
}
#endif

/* */
#endif /* COMMON_H */
