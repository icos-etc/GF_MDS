/*
	common.c

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

/* includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include "common.h"

/* os dependant */
#if defined (_WIN32)
#ifndef STRICT
#define STRICT
#endif /* STRICT */
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#endif /* WIN32_MEAN_AND_LEAN */
#include <windows.h>
/* for memory leak */
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif /* _DEBUG */
#pragma comment(lib, "kernel32.lib")
static WIN32_FIND_DATA wfd;
static HANDLE handle;
#elif defined (linux) || defined (__linux) || defined (__linux__) || defined (__APPLE__)
#include <unistd.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
static DIR *dir;
static struct dirent *dit;
#endif

/*
	note on August 17, 2011 removed space in delimiters 'cause path with
	spaces e.g.: c:\documents and settings .... will not be handled correctly
*/
static const char comma_delimiter[] = ",";
static const char plus_delimiter[] = "+";
static const char filter[] = "*.*";

/* error strings */
static const char err_unable_open_path[] = "unable to open path: %s\n\n";
static const char err_unable_open_file[] = "unable to open file: %s\n\n";
static const char err_path_too_big[] = "specified path \"%s\" is too big.\n\n";
static const char err_filename_too_big[] = "filename \"%s\" is too big.\n\n";
static const char err_empty_argument[] = "empty argument\n";
static const char err_unknown_argument[] = "unknown argument: \"%s\"\n\n";
static const char err_gf_too_less_values[] = "too few valid values to apply gapfilling\n";
static const char err_wildcards_with_no_extension_used[] = "wildcards with no extension used\n";

/* external strings */
const char err_out_of_memory[] = "out of memory";

/* */
int isnan_f  (float       x) { return x != x; }
int isnan_d  (double      x) { return x != x; }
int isnan_ld (long double x) { return x != x; }
int isinf_f  (float       x) { return !isnan (x) && isnan (x - x); }
int isinf_d  (double      x) { return !isnan (x) && isnan (x - x); }
int isinf_ld (long double x) { return !isnan (x) && isnan (x - x); }

/* */
int create_dir(char *Path) {
#if defined (_WIN32)
	char DirName[256];
	char* p = Path;
	char* q = DirName;

	while ( *p ) {
		if ( ('\\' == *p) || ('/' == *p) ) {
			if ( ':' != *(p-1) ) {
				if ( !path_exists(DirName) ) {
					if ( !CreateDirectory(DirName, NULL) ) {
						return 0;
					}
				}
			}
		}
		*q++ = *p++;
		*q = '\0';
	}
	if ( !path_exists(DirName)) {
		return CreateDirectory(DirName, NULL);
	} else {
		return 1;
	}
#elif defined (linux) || defined (__linux) || defined (__linux__) || defined (__APPLE__)
	if ( -1 == mkdir(Path,0777) ) {
		return 0;
	} else {
		return 1;
	}
#else
	return 0;
#endif
}

/* */
static int scan_path(const char *const path) {
#if defined (_WIN32)
	handle = FindFirstFile(path, &wfd);
	if ( INVALID_HANDLE_VALUE == handle ) {
		return 0;
	}
#elif defined (linux) || defined (__linux) || defined (__linux__) || defined (__APPLE__)
	dir = opendir(path);
	if ( !dir ) {
		return 0;
	}
#endif
	/* ok */
	return 1;
}


static int get_files_from_path(const char *const path, FILES **files, int *const count, const int grouped) {
	int i;
	FILES *files_no_leak;
	LIST *list_no_leak;
#if defined (_WIN32)
	do {
		if ( !IS_FLAG_SET(wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ) {
			/* check length */
			i = strlen(wfd.cFileName);
			if ( i > FILENAME_SIZE ) {
				printf(err_filename_too_big, wfd.cFileName);
				free_files(*files, *count);
				return 0;
			}

			if ( !grouped ) {
				/* alloc memory */
				files_no_leak = realloc(*files, (++*count)*sizeof*files_no_leak);
				if ( !files_no_leak ) {
					puts(err_out_of_memory);
					free_files(*files, *count-1);
					return 0;
				}

				/* assign pointer */
				*files = files_no_leak;
				(*files)[*count-1].list = NULL;
				(*files)[*count-1].count = 1;
			}

			/* allocate memory for list */
			list_no_leak = realloc((*files)[*count-1].list, (grouped ? ++(*files)[*count-1].count : 1) * sizeof*list_no_leak);
			if ( !list_no_leak ) {
				puts(err_out_of_memory);
				free_files(*files, *count);
				return 0;
			}
			(*files)[*count-1].list = list_no_leak;

			/* assign evalues */
			strncpy((*files)[*count-1].list[(*files)[*count-1].count-1].name, wfd.cFileName, i);
			(*files)[*count-1].list[(*files)[*count-1].count-1].name[i] = '\0';

			strcpy((*files)[*count-1].list[(*files)[*count-1].count-1].path, path);

			strcpy((*files)[*count-1].list[(*files)[*count-1].count-1].fullpath, path);
			if ( !string_concat((*files)[*count-1].list[(*files)[*count-1].count-1].fullpath, wfd.cFileName, PATH_SIZE) ) {
				printf(err_filename_too_big, wfd.cFileName);
				free_files(*files, *count);
				return 0;
			}
		}
	} while ( FindNextFile(handle, &wfd) );

	/* close handle */
	FindClose(handle);
#elif defined (linux) || defined (__linux) || defined (__linux__) || defined (__APPLE__)
	for ( ; ; ) {
		dit = readdir(dir);
		if ( !dit ) {
			closedir(dir);
			return 1;
		}

		if ( dit->d_type == DT_REG ) {
			/* check length */
			i = strlen(dit->d_name);
			if ( i >= FILENAME_SIZE ) {
				printf(err_filename_too_big, dit->d_name);
				free_files(*files, *count);
				return 0;
			}

			if ( !grouped ) {
				/* alloc memory */
				files_no_leak = realloc(*files, (++*count)*sizeof*files_no_leak);
				if ( !files_no_leak ) {
					puts(err_out_of_memory);
					free_files(*files, *count-1);
					return 0;
				}

				/* assign pointer */
				*files = files_no_leak;
				(*files)[*count-1].list = NULL;
				(*files)[*count-1].count = 1;
			}

			/* allocate memory for list */
			list_no_leak = realloc((*files)[*count-1].list, (grouped ? ++(*files)[*count-1].count : 1) * sizeof*list_no_leak);
			if ( !list_no_leak ) {
				puts(err_out_of_memory);
				free_files(*files, *count);
				return 0;
			}
			(*files)[*count-1].list = list_no_leak;

			/* assign evalues */
			strncpy((*files)[*count-1].list[(*files)[*count-1].count-1].name, dit->d_name, i);
			(*files)[*count-1].list[(*files)[*count-1].count-1].name[i] = '\0';

			strcpy((*files)[*count-1].list[(*files)[*count-1].count-1].path, path);

			strcpy((*files)[*count-1].list[(*files)[*count-1].count-1].fullpath, path);
			if ( !string_concat((*files)[*count-1].list[(*files)[*count-1].count-1].fullpath, dit->d_name, PATH_SIZE) ) {
				printf(err_filename_too_big, dit->d_name);
				free_files(*files, *count);
				return 0;
			}
		}
	}
	/* close handle */
	closedir(dir);
#endif

	/* ok */
	return 1;
}

/*
	free_files
*/
void free_files(FILES *files, const int count) {
	if ( files ) {
		int i;
		for ( i = 0 ; i < count; i++ ) {
			free(files[i].list);
		}
		free(files);
	}
}

/*
	TODO
	CHECK: on ubuntu fopen erroneously open a path (maybe a bug on NTFS partition driver ?)
*/
FILES *get_files(const char *const program_path, char *string, int *const count, int *const error) {
	int i;
	int y;
	int plusses_count;
	int token_length;
	char *token_by_comma;
	char *token_by_plus;
	char *p;
	char *p2;
	char *p3;

	FILE *f;
	FILES *files;
	FILES *files_no_leak;
    LIST *list_no_leak;

	/* check parameters */
	assert(string && count && error);

	/* reset */
	files = NULL;
	*count = 0;
	*error = 0;

	/* loop for each commas */
	for ( token_by_comma = string_tokenizer(string, comma_delimiter, &p); token_by_comma; token_by_comma = string_tokenizer(NULL, comma_delimiter, &p) ) {
		/* get token length */
		for ( token_length = 0; token_by_comma[token_length]; token_length++ );

		/* if length is 0 skip to next token */
		if ( !token_length ) {
			continue;
		}

		/* scan for plusses */
		plusses_count = 0;
		for ( y = 0; y < token_length; y++ ) {
			if ( plus_delimiter[0] == token_by_comma[y] ) {
				/* check if next char is a plus too */
				if ( y < token_length-1 ) {
					if ( plus_delimiter[0] == token_by_comma[y+1] ) {
						++y;
						continue;
					}
				}

				/* plus found! */
				++plusses_count;
			}
		}

		/* no grouping */
		if ( !plusses_count ) {
			/* token is a path ? */
			if ( token_by_comma[token_length-1] == FOLDER_DELIMITER ) {
			#if defined (_WIN32)
				/* add length of filter */
				for ( i = 0; filter[i]; i++ );
				token_length += i;
			#endif

				/* add null terminating char */
				++token_length;

				/* alloc memory */
				p2 = malloc(token_length*sizeof*p2);
				if ( !p2 ) {
					puts(err_out_of_memory);
					*error = 1;
					free_files(files, *count);
					return NULL;
				}

				/* copy token */
				strcpy(p2, token_by_comma);

			#if defined (_WIN32)
				/* add filter at end */
				strcat(p2, filter);
			#endif

				/* scan path */
				if ( !scan_path(p2) ) {
					printf(err_unable_open_path, p2);
					*error = 1;
					free(p2);
					free_files(files, *count);
					return NULL;
				}

				/* get files */
				if ( !get_files_from_path(token_by_comma, &files, count, 0) ) {
					printf(err_unable_open_path, token_by_comma);
					*error = 1;
					free(p2);
					free_files(files, *count);
					return NULL;
				}

				/* free memory */
				free(p2);
			} else {
				/* check for wildcard */
				if ( '*' == token_by_comma[0] ) {
					if ( token_length < 2 ) {
						puts(err_wildcards_with_no_extension_used);
						*error = 1;
						free_files(files, *count);
						return NULL;
					}

					/* add length of filter */
					for ( i = 0; program_path[i]; i++ );
					token_length += i;

					/* add null terminating char */
					++token_length;

					/* alloc memory */
					p2 = malloc(token_length*sizeof*p2);
					if ( !p2 ) {
						puts(err_out_of_memory);
						*error = 1;
						free_files(files, *count);
						return NULL;
					}

					/* copy token */
					strcpy(p2, program_path);

					#if defined (_WIN32)
						/* add filter at end */
						strcat(p2, token_by_comma);
					#endif

					/* scan path */
					if ( !scan_path(p2) ) {
						printf(err_unable_open_path, p2);
						*error = 1;
						free(p2);
						free_files(files, *count);
						return NULL;
					}

					/* get files */
					if ( !get_files_from_path(program_path, &files, count, 0) ) {
						printf(err_unable_open_path, token_by_comma);
						*error = 1;
						free(p2);
						free_files(files, *count);
						return NULL;
					}

					/* free memory */
					free(p2);
				} else {
					/* check if we can simply open token_by_comma */
					f = fopen(token_by_comma, "r");
					if ( !f ) {
						printf(err_unable_open_file, token_by_comma);
						*error = 1;
						free_files(files, *count);
						return NULL;
					}

					/* close file */
					fclose(f);

					/* allocate memory */
					files_no_leak = realloc(files, (++*count)*sizeof*files_no_leak);
					if ( !files_no_leak ) {
						puts(err_out_of_memory);
						free_files(files, *count-1);
						*error = 1;
						return NULL;
					}

					/* assign memory */
					files = files_no_leak;

					/* allocate memory for 1 file */
					files[*count-1].count = 1;
					files[*count-1].list = malloc(sizeof*files[*count-1].list);
					if ( !files[*count-1].list ) {
						puts(err_out_of_memory);
						*error = 1;
						free_files(files, *count);
						return NULL;
					}

					/* check if token has a FOLDER_DELIMITER */
					p2 = strrchr(token_by_comma, FOLDER_DELIMITER);
					if ( p2 ) {
						/* skip FOLDER_DELIMITER */
						++p2;
						/* get length */
						y = strlen(p2);

						/* check filename length */
						if ( y > FILENAME_SIZE ) {
							printf(err_filename_too_big, p2);
							*error = 1;
							free_files(files, *count);
							return NULL;
						}

						/* assign values */
						strncpy(files[*count-1].list->name, p2, y);
						files[*count-1].list->name[y] = '\0';

						strcpy(files[*count-1].list->fullpath, token_by_comma);
						*p2 = '\0';

						strcpy(files[*count-1].list->path, token_by_comma);
					} else {
						/* assign values */
						strcpy(files[*count-1].list->name, token_by_comma);
						if ( program_path ) {
							strcpy(files[*count-1].list->path, program_path);
							strcpy(files[*count-1].list->fullpath, program_path);
							if ( !string_concat(files[*count-1].list->fullpath, token_by_comma, PATH_SIZE) ) {
								printf(err_filename_too_big, token_by_comma);
								free_files(files, *count);
								return 0;
							}
						} else {
							strcpy(files[*count-1].list->path, token_by_comma);
							strcpy(files[*count-1].list->fullpath, token_by_comma);
						}
					}
				}
			}
		} else {
			/* alloc memory */
			files_no_leak = realloc(files, (++*count)*sizeof*files_no_leak);
			if ( !files_no_leak ) {
				puts(err_out_of_memory);
				free_files(files, *count-1);
				return 0;
			}

			/* assign pointer */
			files = files_no_leak;
			files[*count-1].list = NULL;
			files[*count-1].count = 0;

			/* loop for each plus */
			for ( token_by_plus = string_tokenizer(token_by_comma, plus_delimiter, &p2); token_by_plus; token_by_plus = string_tokenizer(NULL, plus_delimiter, &p2) ) {
				/* get token length */
				i = strlen(token_by_plus);
				/* token is a path ? */
				if ( token_by_plus[i-1] == FOLDER_DELIMITER ) {
					/* add length of filter */
					token_length += strlen(filter);

					/* add null terminating char */
					++token_length;

					/* alloc memory */
					p3 = malloc(i*sizeof*p3);
					if ( !p3 ) {
						puts(err_out_of_memory);
						*error = 1;
						free_files(files, *count);
						return NULL;
					}

					/* copy token */
					strcpy(p3, token_by_plus);

					/* add filter at end */
					strcat(p3, filter);

					/* scan path */
					if ( !scan_path(p3) ) {
						printf(err_unable_open_path, p3);
						*error = 1;
						free(p3);
						free_files(files, *count);
						return NULL;
					}

					/* get files */
					if ( !get_files_from_path(token_by_plus, &files, count, 1) ) {
						printf(err_unable_open_path, token_by_plus);
						*error = 1;
						free(p3);
						free_files(files, *count);
						return NULL;
					}

					/* free memory */
					free(p3);
				} else {
					/* check if we can simply open path */
					f = fopen(token_by_plus, "r");
					if ( !f ) {
						printf(err_unable_open_file, token_by_plus);
						*error = 1;
						free_files(files, *count);
						return NULL;
					}

					/* close file */
					fclose(f);

					/* check length */
					if ( token_length >= PATH_SIZE ) {
						printf(err_path_too_big, token_by_plus);
						*error = 1;
						free_files(files, *count);
						return NULL;
					}

					/* allocate memory */
					++files[*count-1].count;
					list_no_leak = realloc(files[*count-1].list, files[*count-1].count*sizeof*list_no_leak);
					if ( !list_no_leak ) {
						puts(err_out_of_memory);
						*error = 1;
						free_files(files, *count);
						return NULL;
					}

					/* assign pointer */
					files[*count-1].list = list_no_leak;

					/* check if token has a FOLDER_DELIMITER */
					p3 = strrchr(token_by_comma, FOLDER_DELIMITER);
					if ( p3 ) {
						/* skip FOLDER_DELIMITER */
						++p3;

						/* get length */
						y = strlen(p3);

						/* check filename length */
						if ( y > FILENAME_SIZE ) {
							printf(err_filename_too_big, p3);
							*error = 1;
							free_files(files, *count);
							return NULL;
						}

						/* assign values */
						strncpy(files[*count-1].list[files[*count-1].count-1].name, p3, y);
						files[*count-1].list->name[y] = '\0';

						strcpy(files[*count-1].list[files[*count-1].count-1].fullpath, token_by_plus);
						*p3 = '\0';

						strcpy(files[*count-1].list[files[*count-1].count-1].path, token_by_plus);

					} else {
						/* check length */
						if ( i > FILENAME_SIZE ) {
							printf(err_filename_too_big, token_by_plus);
							*error = 1;
							free_files(files, *count);
							return NULL;
						}

						/* assign values */
						strcpy(files[*count-1].list[files[*count-1].count-1].name, token_by_plus);
						if ( program_path ) {
							strcpy(files[*count-1].list[files[*count-1].count-1].path, program_path);
							strcpy(files[*count-1].list[files[*count-1].count-1].fullpath, program_path);
							if ( !string_concat(files[*count-1].list[files[*count-1].count-1].fullpath, token_by_plus, PATH_SIZE) ) {
								printf(err_filename_too_big, token_by_plus);
								free_files(files, *count);
								return 0;
							}
						} else {
							strcpy(files[*count-1].list[files[*count-1].count-1].path, token_by_plus);
							strcpy(files[*count-1].list[files[*count-1].count-1].fullpath, token_by_plus);
						}
					}
				}
			}
		}
	}

	/* ok */
	return files;
}

/* */
PREC convert_string_to_prec(const char *const string, int *const error) {
	PREC value;
	char *p;

	/* reset */
	*error = 0;

	if ( !string ) {
		*error = 1;
		return 0.0;
	}

	errno = 0;

	value = (PREC)STRTOD(string, &p);
	STRTOD(p, NULL);
	if ( string == p || *p || errno ) {
		*error = 1;
	}

	return value;
}


/* */
static int check_for_argument(const char *const string , const char *const pattern, char **param) {
	char *pptr = NULL;
	char *sptr = NULL;
	char *start = NULL;

	/* reset */
	*param = NULL;

	for ( start = (char *)string; *start; start++ ) {
	    /* find start of pattern in string */
	    for ( ; (*start && (toupper(*start) != toupper(*pattern))); start++)
	          ;
	    if (start != string+1 )
	          return 0;

	    pptr = (char *)pattern;
	    sptr = (char *)start;

	    while (toupper(*sptr) == toupper(*pptr)) {
			sptr++;
			pptr++;

			/* if end of pattern then pattern was found */
			if ( !*pptr ) {
				if ( !*sptr ) {
					return 1;
				} else
				/* check for next char to be an '=' */
				if ( *sptr == '=' ) {
					if ( *(++sptr) ) {
						*param = sptr;
					}
					return 1;
				}

				return 0;
	    	}
		}
	}

	return 0;
}

/* */
int parse_arguments(int argc, char *argv[], const ARGUMENT *const args, const int arg_count) {
	int i;
	int ok;
	char *param;

	/* */
	while ( argc > 1 ) {
		/* check for arguments */
		if ( ( '-' != argv[1][0]) && ( '/' != argv[1][0]) ) {
			if ( '\0' == argv[1][0] ) {
				puts(err_empty_argument);
			} else {
				printf(err_unknown_argument, argv[1]);
			}
			return 0;
		}

		/* */
		ok = 0;
		for ( i = 0; i < arg_count; i++ ) {
			if ( check_for_argument(argv[1], args[i].name, &param) ) {
				ok = 1;

				/* check if function is present */
				assert(args[i].f);

				/* call function */
				if ( !args[i].f(args[i].name, param, args[i].p) ) {
					return 0;
				}

				break;
			}
		}

		/* */
		if ( !ok ) {
			printf(err_unknown_argument, argv[1]+1);
			return 0;
		}

		/* */
		++argv;
		--argc;
	}

	/* ok */
	return 1;
}

/* */
int string_compare_i(const char *str1, const char *str2) {
	register signed char __res;

	/* added on April 23, 2013 */
	if ( (NULL == str1) && (NULL == str2 ) ) {
		return 0;
	}
	if ( NULL == str1 ) {
		return 1;
	}
	if ( NULL == str2 ) {
		return -1;
	}

	/* */
	while ( 1 ) {
		if ( (__res = toupper( *str1 ) - toupper( *str2++ )) != 0 || !*str1++ ) {
			break;
		}
	}

	/* returns an integer greater than, equal to, or less than zero */
	return __res;
}

/* */
int string_n_compare_i(const char *str1, const char *str2, const int len) {
	int i;
	register signed char __res;

	if ( len <= 0 ) {
		return -1;
	}

	/* added on April 23, 2013 */
	if ( (NULL == str1) && (NULL == str2 ) ) {
		return 0;
	}
	if ( NULL == str1 ) {
		return 1;
	}
	if ( NULL == str2 ) {
		return -1;
	}

	/* */
	i = 0;
	while ( 1 ) {
		if ( (__res = toupper( *str1 ) - toupper( *str2++ )) != 0 || !*str1++ ) {
			break;
		}
		if ( ++i >= len ) {
			break;
		}
	}

	/* returns an integer greater than, equal to, or less than zero */
	return __res;
}

/* */
char *string_copy(const char *const string) {
	int i;
	int len;
	char *p;

	/* check for null pointer */
	if ( ! string ) {
		return NULL;
	}

	/* get length of string */
	for ( len = 0; string[len]; len++ );

	/* allocate memory */
	p = malloc(len+1);
	if ( ! p ) {
		return NULL;
	}

	/* copy ! */
	for ( i = 0; i < len; i++ ) {
		p[i] = string[i];
	}
	p[len] = '\0';

	return p;
}

/* */
char *string_tokenizer(char *string, const char *delimiters, char **p) {
	char *sbegin;
	char *send;

	sbegin = string ? string : *p;
	sbegin += strspn(sbegin, delimiters);
	if ( *sbegin == '\0') {
		*p = "";
		return NULL;
	}

	send = sbegin + strcspn(sbegin, delimiters);
	if ( *send != '\0') {
		*send++ = '\0';
	}
	*p = send;

	return sbegin;
}

/* */
int convert_string_to_int(const char *const string, int *const error) {
	int value = 0;
	char *p = NULL;

	/* reset */
	*error = 0;

	if ( !string ) {
		*error = 1;
		return 0;
	}

	errno = 0;
	value = (int)strtod(string, &p);
	strtod(p, NULL);
	if ( string == p || *p || errno ) {
		*error = 1;
	}

	return value;
}

/* */
char *get_current_directory(void) {
	char *p;
#if defined (_WIN32)
	p = malloc((MAX_PATH+1)*sizeof *p);
	if ( !p ) {
		return NULL;
	}
	if ( !GetModuleFileName(NULL, p, MAX_PATH) ) {
		free(p);
		return NULL;
	}
	p[(strrchr(p, '\\')-p)+1] = '\0';
	return p;
#elif defined (linux) || defined (__linux) || defined (__linux__) || defined (__APPLE__)
	int len;
	p = malloc((MAXPATHLEN+1)*sizeof *p);
	if ( !p ) {
		return NULL;
	}
	if ( !getcwd(p, MAXPATHLEN) ) {
		free(p);
		return NULL;
	}
	/* check if last chars is a FOLDER_DELIMITER */
	len = strlen(p);
	if ( !len ) {
		free(p);
		return NULL;
	}
	if ( p[len-1] != FOLDER_DELIMITER ) {
		if ( !add_char_to_string(p, FOLDER_DELIMITER, MAXPATHLEN) ) {
			free(p);
			return NULL;
		}
	}
	return p;
#else
	return NULL;
#endif
}

/* */
int add_char_to_string(char *const string, char c, const int size) {
	int i;

	/* check for null pointer */
	if ( !string ) {
		return 0;
	}

	/* compute length */
	for ( i = 0; string[i]; i++ );

	/* check length */
	if ( i >= size-1 ) {
		return 0;
	}

	/* add char */
	string[i] = c;
	string[i+1] = '\0';

	/* */
	return 1;
}

/* */
int string_concat(char *const string, const char *const string_to_add, const int size) {
	int i;
	int y;

	/* check for null pointer */
	if ( !string || !string_to_add ) {
		return 0;
	}

	/* compute lenghts */
	for ( i = 0; string[i]; i++ );
	for ( y = 0; string_to_add[y]; y++ );

	/* check length */
	if ( i >= size-y-1 ) {
		return 0;
	}

	strcat(string, string_to_add);

	/* */
	return 1;
}

/* */
int path_exists(const char *const path) {
#if defined (_WIN32)
	DWORD dwResult;
#endif
	if ( !path ) {
		return 0;
	}
#if defined (_WIN32)
	dwResult = GetFileAttributes(path);
	if (dwResult != INVALID_FILE_ATTRIBUTES && (dwResult & FILE_ATTRIBUTE_DIRECTORY)) {
		return 1;
	}
#elif defined (linux) || defined (__linux) || defined (__linux__) || defined (__APPLE__)
	if ( !access(path, W_OK) ) {
		return 1;
	}
#endif
	return 0;
}

/* */
int file_exists(const char *const file) {
#if defined (_WIN32)
	DWORD dwResult;
#endif
	if ( !file ) {
		return 0;
	}
#if defined (_WIN32)
	dwResult = GetFileAttributes(file);
	if (dwResult != INVALID_FILE_ATTRIBUTES && !(dwResult & FILE_ATTRIBUTE_NORMAL)) {
		return 1;
	}
#elif defined (linux) || defined (__linux) || defined (__linux__) || defined (__APPLE__)
	if ( !access(file, W_OK) ) {
		return 1;
	}
#endif
	return 0;
}

/* TODO : implement a better comparison for equality */
int compare_prec(const void * a, const void * b) {
	if ( *(PREC *)a < *(PREC *)b ) {
		return -1;
	} else if ( *(PREC *)a > *(PREC *)b ) {
		return 1;
	} else {
		return 0;
	}
}

/* */
char *tokenizer(char *string, char *delimiter, char **p) {
	int i;
	int j;
	int c;
	char *_p;

	/* */
	if ( string ) {
		*p = string;
		_p = string;
	} else {
		_p = *p;
		if ( !_p ) {
			return NULL;
		}
	}

	/* */
	c = 0;
	for (i = 0; _p[i]; i++) {
		for (j = 0; delimiter[j]; j++) {
			if ( _p[i] == delimiter[j] ) {
				*p += c + 1;
				_p[i] = '\0';
				return _p;
			} else {
				++c;
			}
		}
	}

	/* */
	*p = NULL;
	return _p;
}

/* added on January 17, 2018 */
int get_rows_count_by_timeres(const int timeres, const int year) {
	int rows_count;

	assert((timeres > SPOT_TIMERES) && (timeres <= HOURLY_TIMERES));

	/* assume HALFHOUR_TIMERES */
	rows_count = IS_LEAP_YEAR(year) ? LEAP_YEAR_ROWS : YEAR_ROWS;

	switch ( timeres ) {
		case QUATERHOURLY_TIMERES:
			rows_count *= 2;
		break;

		case HOURLY_TIMERES:
			rows_count /= 2;
		break;
	}

	return rows_count;
}

/* added on January 17, 2018 */
int get_rows_per_day_by_timeres(const int timeres) {
	int rows_per_day;

	assert((timeres > SPOT_TIMERES) && (timeres <= HOURLY_TIMERES));

	rows_per_day = 48; /* assume HALFHOUR_TIMERES */

	switch ( timeres ) {
		case QUATERHOURLY_TIMERES:
			rows_per_day *= 2;
		break;

		case HOURLY_TIMERES:
			rows_per_day /= 2;
		break;
	}

	return rows_per_day;
}

/* added on January 17, 2018 */
int get_rows_per_hour_by_timeres(const int timeres) {
	int rows_per_hour;

	assert((timeres > SPOT_TIMERES) && (timeres <= HOURLY_TIMERES));

	rows_per_hour = 2; /* assume HALFHOUR_TIMERES */

	switch ( timeres ) {
		case QUATERHOURLY_TIMERES:
			rows_per_hour *= 2;
		break;

		case HOURLY_TIMERES:
			rows_per_hour /= 2;
		break;
	}

	return rows_per_hour;
}

/* */
TIMESTAMP *get_timestamp(const char *const string) {
	int i;
	int j;
	int index;
	int error;
	char *p;
	char token[5];
	TIMESTAMP *t;
	const char *field[] = { "year", "month", "day", "hour", "minute", "second" };
	const int field_size[] = { 4, 2, 2, 2, 2, 2 };

	for ( i = 0; string[i]; i++ );
	if ( (i < 0) || (i > 14) || (i & 1) ) {
		puts("bad length for timestamp");
		return NULL;
	}

	t = malloc(sizeof*t);
	if ( ! t ) {
		puts(err_out_of_memory);
		return NULL;
	}
	t->YYYY = 0;
	t->MM = 0;
	t->DD = 0;
	t->hh = 0;
	t->mm = 0;
	t->ss = 0;

	p = (char *)string;
	index = 0;
	while ( 1 ) {
		i = 0;
		while ( i < field_size[index] ) {
			token[i++] = *p++;
		}
		if ( i != field_size[index] ) {
			printf("bad field '%s' on %s\n\n", field[index], string);
			free(t);
			return NULL;
		}
		token[field_size[index]] = '\0';
		j = convert_string_to_int(token, &error);
		if ( error ) {
			printf("bad value '%s' for field '%s' on %s\n\n", token, field[index], string);
			free(t);
			return NULL;
		}

		switch ( index ) {
			case 0:	/* year */
				t->YYYY = j;
			break;

			case 1: /* month */
				t->MM = j;
			break;

			case 2: /* day */
				t->DD = j;
			break;

			case 3: /* hour */
				t->hh = j;
			break;

			case 4: /* minute */
				t->mm = j;
			break;

			case 5: /* second */
				t->ss = j;
			break;
		}
		++index;
		if ( ! *p ) {
			return t;
		}
	}
}

/* updated on January 17, 2018 */
int get_row_by_timestamp(const TIMESTAMP *const t, const int timeres) {
	int i;
	int y;
	int rows_per_day;
	int rows_per_hour;
	const int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	assert((timeres > SPOT_TIMERES) && (timeres <= HOURLY_TIMERES));

	/* */
	if ( ! t ) {
		return -1;
	}

	rows_per_day = get_rows_per_day_by_timeres(timeres);
	rows_per_hour = get_rows_per_hour_by_timeres(timeres);
	
	/* check for last row */
	if (	(1 == t->DD) &&
			(1 == t->MM) &&
			(0 == t->hh) &&
			(0 == t->mm) ) {
		i = get_rows_count_by_timeres(timeres, t->YYYY-1);
	} else {
		i = 0;
		for (y = 0; y < t->MM - 1; y++ ) {
			i += days_in_month[y];
		}

		/* leap year ? */
		if ( IS_LEAP_YEAR(t->YYYY) && ((t->MM - 1) > 1) ) {
			++i;
		}

		/* */
		i += t->DD - 1;
		i *= rows_per_day;
		i += t->hh * rows_per_hour;
		if ( t->mm > 0 ) {
			++i;
		}
	}

	/* return zero based index */
	return --i;
}

/* */
int get_year_from_timestamp_string(const char *const string) {
	int year;
	TIMESTAMP *t;

	if ( ! string || ! string[0] ) {
		return -1;
	}

	t = get_timestamp(string);
	if ( ! t ) {
		return -1;
	}

	year = t->YYYY;
	free(t);

	return year;

}

/* */
TIMESTAMP *timestamp_get_by_row(int row, int yy, const int timeres, const int start) {
	int i;
	int is_leap;
	int rows_per_day;
	int days_per_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	static TIMESTAMP t = { 0 };

	/* updated on January 17, 2018 */
	assert((timeres > SPOT_TIMERES) && (timeres <= HOURLY_TIMERES));

	/* reset */
	t.YYYY = yy;
	t.ss = 0;

	/* leap year ? */
	is_leap = IS_LEAP_YEAR(yy);
	if ( is_leap ) {
		++days_per_month[1];
	}

	/* inc row...we used 1 based index */
	if ( ! start ) {
		++row;
	}
	
	/* compute rows per day */
	/* updated on January 17, 2018 */
	rows_per_day = get_rows_per_day_by_timeres(timeres);

	/* get day and month */
	t.DD = row / rows_per_day;
	for ( i = 0, t.MM = 0; t.MM < 12; ++t.MM ) {
		i += days_per_month[t.MM];
		if ( t.DD <= i ) {
			t.DD -= i - days_per_month[t.MM];
			break;
		}
	}
	if ( ++t.DD > days_per_month[t.MM] ) {
		t.DD = 1;
		if ( ++t.MM > 11 ) {
			t.MM = 0;
			++t.YYYY;
		}
	}
	++t.MM;

	/* get hour */
	/* updated on January 17, 2018 */
	if ( QUATERHOURLY_TIMERES == timeres ) {
		// TODO
		// CHECK THIS!
		t.hh = (row % rows_per_day) / 4;
		t.mm = (row % 15) * 15;
	} else if ( HALFHOURLY_TIMERES == timeres ) {
		t.hh = (row % rows_per_day) / 2;

		/* even row ? */
		if ( row & 1 ) {
			t.mm = 30;
		} else {
			t.mm = 0;
		}
	} else if ( HOURLY_TIMERES == timeres ) {
		t.hh = row % rows_per_day;
		t.mm = 0;
	}

	/* ok */
	return &t;
}

/* updated on January 17, 2018 */
char *timestamp_get_by_row_s(int row, int yy, const int timeres, const int start) {
	TIMESTAMP *t;
	static char buffer[12+1] = { 0 };

	assert((timeres > SPOT_TIMERES) && (timeres <= HOURLY_TIMERES));

	t = timestamp_get_by_row(row, yy, timeres, start);
	sprintf(buffer, "%04d%02d%02d%02d%02d", t->YYYY, t->MM, t->DD, t->hh, t->mm);
	return buffer;
}

/* updated on January 17, 2018 */
TIMESTAMP *timestamp_ww_get_by_row(int row, int year, const int timeres, int start) {
	int i;
	int last;

	/* */
	assert((row >= 0) &&(row < 52));
	assert((timeres > SPOT_TIMERES) && (timeres <= HOURLY_TIMERES));

	/* */
	last = (52-1 == row);
	i = 7 * get_rows_per_day_by_timeres(timeres);
	row *= i;

	/* */
	if ( ! start ) {
		if ( last ) {
			row = get_rows_count_by_timeres(timeres, year);			
		} else {
			row += i;
		}
		start = 1;
		--row;
	}

	/* */
	return timestamp_get_by_row(row, year, timeres, start);
}

/* updated on January 17, 2018 */
char *timestamp_ww_get_by_row_s(int row, int yy, const int timeres, const int start) {
	TIMESTAMP *t;
	static char buffer[8+1] = { 0 };

	assert((timeres > SPOT_TIMERES) && (timeres <= HOURLY_TIMERES));

	t = timestamp_ww_get_by_row(row, yy, timeres, start);
	sprintf(buffer, "%04d%02d%02d", t->YYYY, t->MM, t->DD);
	return buffer;
}

/* private function for gapfilling */
static PREC gf_get_similiar_mean(const GF_ROW *const gf_rows, const int rows_count) {
 	int i;
	PREC mean;

	/* check parameter */
	assert(gf_rows);

	/* get mean */
	mean = 0.0;
	for ( i = 0; i < rows_count; i++ ) {
		mean += gf_rows[i].similiar;
	}
	mean /= rows_count;

	/* check for NAN */
	if ( mean != mean ) {
		mean = INVALID_VALUE;
	}

	/* */
	return mean;
}

/* gapfilling */
PREC gf_get_similiar_standard_deviation(const GF_ROW *const gf_rows, const int rows_count) {
	int i;
	PREC mean;
	PREC sum;
	PREC sum2;

	/* check parameter */
	assert(gf_rows);

	/* get mean */
	mean = gf_get_similiar_mean(gf_rows, rows_count);
	if ( IS_INVALID_VALUE(mean) ) {
		return INVALID_VALUE;
	}

	/* compute standard deviation */
	sum = 0.0;
	sum2 = 0.0;
	for ( i = 0; i < rows_count; i++ ) {
		sum = (gf_rows[i].similiar - mean);
		sum *= sum;
		sum2 += sum;
	}
	sum2 /= rows_count-1;
	sum2 = (PREC)SQRT(sum2);

	/* check for NAN */
	if ( sum2 != sum2 ) {
		sum2 = INVALID_VALUE;
	}

	/* */
	return sum2;
}

/* gapfilling */
PREC gf_get_similiar_median(const GF_ROW *const gf_rows, const int rows_count, int *const error) {
	int i;
	PREC *p_median;
	PREC result;

	/* check for null pointer */
	assert(gf_rows);

	/* reset */
	*error = 0;

	if ( !rows_count ) {
		return INVALID_VALUE;
	} else if ( 1 == rows_count ) {
		return gf_rows[0].similiar;
	}

	/* get valid values */
	p_median = malloc(rows_count*sizeof*p_median);
	if ( !p_median ) {
		*error = 1;
		return INVALID_VALUE;
	}
	for ( i = 0; i < rows_count; i++ ) {
		p_median[i] = gf_rows[i].similiar;
	}

	/* sort values */
	qsort(p_median, rows_count, sizeof *p_median, compare_prec);

	/* get median */
	if ( rows_count & 1 ) {
		result = p_median[((rows_count+1)/2)-1];
	} else {
		result = ((p_median[(rows_count/2)-1] + p_median[rows_count/2]) / 2);
	}

	/* free memory */
	free(p_median);

	/* check for NAN */
	if ( result != result ) {
		result = INVALID_VALUE;
	}

	/* */
	return result;
}

/* private function for gapfilling */
static int gapfill(	PREC *values,
					const int struct_size,
					GF_ROW *const gf_rows,
					const int start_window,
					const int end_window,
					const int current_row,
					const int start,
					const int end,
					const int step,
					const int method,
					const int timeres,
					const PREC value1_tolerance_min,
					const PREC value1_tolerance_max,
					const PREC value2_tolerance_min,
					const PREC value2_tolerance_max,
					const PREC value3_tolerance_min,
					const PREC value3_tolerance_max,
					const int tofill_column,
					const int value1_column,
					const int value2_column,
					const int value3_column) {
	int i;
	int y;
	int j;
	int z;
	int window;
	int window_start;
	int window_end;
	int window_current;
	int samples_count;
	PREC value1_tolerance;
	PREC value2_tolerance;
	PREC value3_tolerance;
	PREC *window_current_values;
	PREC *row_current_values;

	/* check parameter */
	assert(values && gf_rows && (method >=0 && method < GF_METHODS));
	assert((timeres > SPOT_TIMERES) && (timeres <= HOURLY_TIMERES));

	/* reset */
	window = 0;
	window_start = 0;
	window_end = 0;
	window_current = 0;
	samples_count = 0;
	value1_tolerance = value1_tolerance_min;
	value2_tolerance = value2_tolerance_min;
	value3_tolerance = value3_tolerance_min;

	/* modified on January 17, 2018 */
	/* j is and index checker for timeres */
	switch ( timeres ) {
		case QUATERHOURLY_TIMERES:
			j = 9;
		break;

		case HALFHOURLY_TIMERES:
			j = 5;
		break;

		case HOURLY_TIMERES:
			j = 3;
		break;
	}

	/* */
	i = start;
	if ( GF_TOFILL_METHOD == method ) {
		/* modified on January 17, 2018 */
		switch ( timeres ) {
			case QUATERHOURLY_TIMERES:
				z = 96;
			break;

			case HALFHOURLY_TIMERES:
				z = 48;
			break;

			case HOURLY_TIMERES:
				z = 24;
			break;
		}
	} else {
		z = 1;
	}
	while ( i <= end ) {
		/* reset */
		samples_count = 0;

		/* compute window */
		/* modified on January 17, 2018 */
		switch ( timeres ) {
			case QUATERHOURLY_TIMERES:
				window = 96 * i;
			break;

			case HALFHOURLY_TIMERES:
				window = 48 * i;
			break;

			case HOURLY_TIMERES:
				window = 24 * i;
			break;
		}

		/* get window start index */
		window_start = current_row - window;
		if ( GF_TOFILL_METHOD == method ) {
			/* modified on January 17, 2018 */
			switch ( timeres ) {
				case QUATERHOURLY_TIMERES:
					window_start -= 4;
				break;

				case HALFHOURLY_TIMERES:
					window_start -= 2;
				break;

				case HOURLY_TIMERES:
					window_start -= 1;
				break;
			}
		}

		if ( GF_TOFILL_METHOD != method ) {
			/* fix for recreate markus code */
			++window_start;
		}

		/* get window end index */
		window_end = current_row + window;
		if (GF_TOFILL_METHOD == method ) {
			/* modified on January 17, 2018 */
			switch ( timeres ) {
				case QUATERHOURLY_TIMERES:
					window_end += 5;
				break;

				case HALFHOURLY_TIMERES:
					window_end += 3;
				break;

				case HOURLY_TIMERES:
					window_end += 2;
				break;
			}
		}

		/*	fix bounds for first two methods
			cause in hour method (NEE_METHOD) a window start at -32 and window end at 69,
			it will be fixed to window start at 0 and this is an error...
		*/
		if ( GF_TOFILL_METHOD != method ) {
			if ( window_start < 0 ) {
				window_start = 0;
			}

			if ( window_end > end_window ) {
				window_end = end_window;
			}

			/* modified on June 25, 2013 */
			/* compute tolerance for value1 */
			if ( IS_INVALID_VALUE(value1_tolerance_min) ) {
				value1_tolerance = value1_tolerance_max;
			} else if ( IS_INVALID_VALUE(value1_tolerance_max) ) {
				value1_tolerance = value1_tolerance_min;
			} else {
				value1_tolerance = ((PREC *)(((char *)values)+current_row*struct_size))[value1_column];
				if ( value1_tolerance < value1_tolerance_min ) {
					value1_tolerance = value1_tolerance_min;
				} else if ( value1_tolerance > value1_tolerance_max ) {
					value1_tolerance = value1_tolerance_max;
				}
			}

			/* modified on January 17, 2018 */
			/* compute tolerance for value2 */
			if ( IS_INVALID_VALUE(value2_tolerance_min) ) {
				value2_tolerance = GF_DRIVER_2A_TOLERANCE_MIN;
			} else if ( ! IS_INVALID_VALUE(value2_tolerance_max) ) {
				value2_tolerance = ((PREC *)(((char *)values)+current_row*struct_size))[value2_column];
				if ( value2_tolerance < value2_tolerance_min ) {
					value2_tolerance = value2_tolerance_min;
				} else if ( value2_tolerance > value2_tolerance_max ) {
					value2_tolerance = value2_tolerance_max;
				}
			}

			/* modified on January 17, 2018 */
			/* compute tolerance for value3 */
			if ( IS_INVALID_VALUE(value3_tolerance_min) ) {
				value3_tolerance = GF_DRIVER_2B_TOLERANCE_MIN;
			} else if ( ! IS_INVALID_VALUE(value3_tolerance_max) ) {
				value3_tolerance = ((PREC *)(((char *)values)+current_row*struct_size))[value3_column];
				if ( value3_tolerance < value3_tolerance_min ) {
					value3_tolerance = value3_tolerance_min;
				} else if ( value3_tolerance > value3_tolerance_max ) {
					value3_tolerance = value3_tolerance_max;
				}
			}
		}

		assert(! IS_INVALID_VALUE(value1_tolerance));
		assert(! IS_INVALID_VALUE(value2_tolerance));
		assert(! IS_INVALID_VALUE(value3_tolerance));

		/* loop through window */
		for ( window_current = window_start; window_current < window_end; window_current += z ) {
			window_current_values = ((PREC *)(((char *)values)+window_current*struct_size));
			row_current_values = ((PREC *)(((char *)values)+current_row*struct_size));

			switch ( method ) {
				case GF_ALL_METHOD:
					if ( IS_FLAG_SET(gf_rows[window_current].mask, GF_ALL_VALID) ) {
						if (
								(FABS(window_current_values[value2_column]-row_current_values[value2_column]) < value2_tolerance) &&
								(FABS(window_current_values[value1_column]-row_current_values[value1_column]) < value1_tolerance) &&
								(FABS(window_current_values[value3_column]-row_current_values[value3_column]) < value3_tolerance)
							) {
							gf_rows[samples_count++].similiar = window_current_values[tofill_column];
						}
					}
				break;

				case GF_VALUE1_METHOD:
					if ( IS_FLAG_SET(gf_rows[window_current].mask, (GF_TOFILL_VALID|GF_VALUE1_VALID)) ) {
						if ( FABS(window_current_values[value1_column]-row_current_values[value1_column]) < value1_tolerance ) {
							gf_rows[samples_count++].similiar = window_current_values[tofill_column];
						}
					}
				break;

				case GF_TOFILL_METHOD:
					for ( y = 0; y < j; y++ ) {
						if ( ((window_current+y) < 0) || (window_current+y) >= end_window ) {
							continue;
						}
						if ( IS_FLAG_SET(gf_rows[window_current+y].mask, GF_TOFILL_VALID) ) {
							gf_rows[samples_count++].similiar = ((PREC *)(((char *)values)+((window_current+y)*struct_size)))[tofill_column];
						}
					}
				break;
			}
		}

		if ( samples_count > 1 ) {
			/* set mean */
			gf_rows[current_row].filled = gf_get_similiar_mean(gf_rows, samples_count);

			/* set standard deviation */
			gf_rows[current_row].stddev = gf_get_similiar_standard_deviation(gf_rows, samples_count);

			/* set method */
			gf_rows[current_row].method = method + 1;

			/* set time-window */
			gf_rows[current_row].time_window = i * 2;

			/* fix hour method timewindow */
			if ( GF_TOFILL_METHOD == method ) {
				++gf_rows[current_row].time_window;
			}

			/* set samples */
			gf_rows[current_row].samples_count = samples_count;

			/* ok */
			return 1;
		}

		/* inc loop */
		i += step;

		/* break if window bigger than  */
		if ( (window_start < start_window) && (window_end > end_window) ) {
			break;
		}
	}

	/* */
	return 0;
}

/* */
GF_ROW *gf_mds_with_bounds(	PREC *values,
							const int struct_size,
							const int rows_count,
							const int columns_count,
							const int timeres,
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
							int *no_gaps_filled_count) {
	int i;
	int c;
	int valids_count;
	GF_ROW *gf_rows;

	/* */
	assert(values && rows_count && no_gaps_filled_count);

	/* reset */
	*no_gaps_filled_count = 0;
	if ( start_row < 0  ) {
		start_row = 0;
	}
	if ( -1 == end_row ) {
		end_row = rows_count;
	} else if ( end_row > rows_count ) {
		end_row = rows_count;
	}

	/* allocate memory */
	gf_rows = malloc(rows_count*sizeof*gf_rows);
	if ( !gf_rows ) {
		puts(err_out_of_memory);
		return NULL;
	}

	/* reset */
	for ( i = 0; i < rows_count; i++ ) {
		gf_rows[i].mask = 0;
		gf_rows[i].similiar = INVALID_VALUE;
		gf_rows[i].stddev = INVALID_VALUE;
		gf_rows[i].filled = INVALID_VALUE;
		gf_rows[i].quality = INVALID_VALUE;
		gf_rows[i].time_window = 0;
		gf_rows[i].samples_count = 0;
		gf_rows[i].method = 0;
	}

	/* update mask and count valids TO FILL */
	valids_count = 0;
	for ( i = start_row; i < end_row; i++ ) {
		for ( c = 0; c < columns_count; c++ ) {
			if ( !IS_INVALID_VALUE(((PREC *)(((char *)values)+i*struct_size))[c]) ) {
				if ( tofill_column == c ) {
					gf_rows[i].mask |= GF_TOFILL_VALID;
				} else if ( value1_column == c ) {
					gf_rows[i].mask |= GF_VALUE1_VALID;
				} else if ( value2_column == c ) {
					gf_rows[i].mask |= GF_VALUE2_VALID;
				} else if ( value3_column == c ) {
					gf_rows[i].mask |= GF_VALUE3_VALID;
				}
			}
		}

		/* check for QC */
		if (	!IS_INVALID_VALUE(qc_thrs) &&
				(value1_qc_column != -1) &&
				!IS_INVALID_VALUE(((PREC *)(((char *)values)+i*struct_size))[value1_qc_column]) ) {
			if ( ((PREC *)(((char *)values)+i*struct_size))[value1_qc_column] > qc_thrs ) {
				gf_rows[i].mask &= ~GF_VALUE1_VALID;
			}
		}

		if (	!IS_INVALID_VALUE(qc_thrs) &&
				(value2_qc_column != -1) &&
				!IS_INVALID_VALUE(((PREC *)(((char *)values)+i*struct_size))[value2_qc_column]) ) {
			if ( ((PREC *)(((char *)values)+i*struct_size))[value2_qc_column] > qc_thrs ) {
				gf_rows[i].mask &= ~GF_VALUE2_VALID;
			}
		}

		if (	!IS_INVALID_VALUE(qc_thrs) &&
				(value3_qc_column != -1) &&
				!IS_INVALID_VALUE(((PREC *)(((char *)values)+i*struct_size))[value3_qc_column]) ) {
			if ( ((PREC *)(((char *)values)+i*struct_size))[value3_qc_column] > qc_thrs ) {
				gf_rows[i].mask &= ~GF_VALUE3_VALID;
			}
		}

		if ( IS_FLAG_SET(gf_rows[i].mask, GF_TOFILL_VALID) ) {
			++valids_count;
		}
	}

	if ( valids_count < values_min ) {
		puts(err_gf_too_less_values);
		free(gf_rows);
		return NULL;
	}

	/* modified on January 17, 2018 */
	if ( IS_INVALID_VALUE(value1_tolerance_min) && IS_INVALID_VALUE(value1_tolerance_max) ) {
		value1_tolerance_min = GF_DRIVER_1_TOLERANCE_MIN;
		value1_tolerance_max = GF_DRIVER_1_TOLERANCE_MAX;
	} else if ( IS_INVALID_VALUE(value1_tolerance_min) ) {
		value1_tolerance_min = GF_DRIVER_1_TOLERANCE_MIN;
	} else if ( IS_INVALID_VALUE(value1_tolerance_max) ) {
		value1_tolerance_max = INVALID_VALUE;
	}

	/* modified on January 17, 2018 */
	if ( IS_INVALID_VALUE(value2_tolerance_min) && IS_INVALID_VALUE(value2_tolerance_max) ) {
		value2_tolerance_min = GF_DRIVER_2A_TOLERANCE_MIN;
		value2_tolerance_max = GF_DRIVER_2A_TOLERANCE_MAX;
	} else if ( IS_INVALID_VALUE(value2_tolerance_min) ) {
		value2_tolerance_min = GF_DRIVER_2A_TOLERANCE_MIN;
	} else if ( IS_INVALID_VALUE(value2_tolerance_max) ) {
		value2_tolerance_max = INVALID_VALUE;
	}

	/* modified on January 17, 2018 */
	if ( IS_INVALID_VALUE(value3_tolerance_min) && IS_INVALID_VALUE(value3_tolerance_max) ) {
		value3_tolerance_min = GF_DRIVER_2B_TOLERANCE_MIN;
		value3_tolerance_max = GF_DRIVER_2B_TOLERANCE_MAX;
	} else if ( IS_INVALID_VALUE(value3_tolerance_min) ) {
		value3_tolerance_min = GF_DRIVER_2B_TOLERANCE_MIN;
	} else if ( IS_INVALID_VALUE(value3_tolerance_max) ) {
		value3_tolerance_max = INVALID_VALUE;
	}

	/* loop for each row */
	for ( i = start_row; i < end_row; i++ ) {
		/* copy value from TOFILL to FILLED */
		gf_rows[i].filled = ((PREC *)(((char *)values)+i*struct_size))[tofill_column];

		/* compute hat ? */
		if ( !IS_INVALID_VALUE(gf_rows[i].filled) && !compute_hat ) {
			continue;
		}

		/*	fill
			Added 20140422: if a gap is impossible to fill, e.g. if with MDV there are no data in the whole dataset acquired in a range +/- one hour,
			the data point is not filled and the qc is set to -9999
		*/
		if ( !gapfill(values, struct_size, gf_rows, start_row, end_row, i, 7, 14, 7, GF_ALL_METHOD, timeres, value1_tolerance_min, value1_tolerance_max, value2_tolerance_min, value2_tolerance_max, value3_tolerance_min, value3_tolerance_max, tofill_column, value1_column, value2_column, value3_column) )
			if ( !gapfill(values, struct_size, gf_rows, start_row, end_row, i, 7, 7, 7, GF_VALUE1_METHOD, timeres, value1_tolerance_min, value1_tolerance_max, value2_tolerance_min, value2_tolerance_max, value3_tolerance_min, value3_tolerance_max, tofill_column, value1_column, value2_column, value3_column) )
				if ( !gapfill(values, struct_size, gf_rows, start_row, end_row, i, 0, 2, 1, GF_TOFILL_METHOD, timeres, value1_tolerance_min, value1_tolerance_max, value2_tolerance_min, value2_tolerance_max, value3_tolerance_min, value3_tolerance_max, tofill_column, value1_column, value2_column, value3_column) )
					if ( !gapfill(values, struct_size, gf_rows, start_row, end_row, i, 21, 77, 7, GF_ALL_METHOD, timeres, value1_tolerance_min, value1_tolerance_max, value2_tolerance_min, value2_tolerance_max, value3_tolerance_min, value3_tolerance_max, tofill_column, value1_column, value2_column, value3_column) )
						if ( !gapfill(values, struct_size, gf_rows, start_row, end_row, i, 14, 77, 7, GF_VALUE1_METHOD, timeres, value1_tolerance_min, value1_tolerance_max, value2_tolerance_min, value2_tolerance_max, value3_tolerance_min, value3_tolerance_max, tofill_column, value1_column, value2_column, value3_column) )
							if ( !gapfill(values, struct_size, gf_rows, start_row, end_row, i, 3, end_row + 1, 3, GF_TOFILL_METHOD, timeres, value1_tolerance_min, value1_tolerance_max, value2_tolerance_min, value2_tolerance_max, value3_tolerance_min, value3_tolerance_max, tofill_column, value1_column, value2_column, value3_column) ) {
								++*no_gaps_filled_count;
								continue;
							}

		/* compute quality */
		gf_rows[i].quality =	(gf_rows[i].method > 0) +
								((gf_rows[i].method == 1 && gf_rows[i].time_window > 14) || (gf_rows[i].method == 2 && gf_rows[i].time_window > 14) || (gf_rows[i].method == 3 && gf_rows[i].time_window > 1)) +
								((gf_rows[i].method == 1 && gf_rows[i].time_window > 56) || (gf_rows[i].method == 2 && gf_rows[i].time_window > 28) || (gf_rows[i].method == 3 && gf_rows[i].time_window > 5));
	}

	/* ok */
	return gf_rows;
}

/* */
GF_ROW *gf_mds(PREC *values, const int struct_size, const int rows_count, const int columns_count, const int timeres,
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
																									int *no_gaps_filled_count) {
	return gf_mds_with_bounds(	values,
								struct_size,
								rows_count,
								columns_count,
								timeres,
								value1_tolerance_min,
								value1_tolerance_max,
								value2_tolerance_min,
								value2_tolerance_max,
								value3_tolerance_min,
								value3_tolerance_max,
								tofill_column,
								value1_column,
								value2_column,
								value3_column,
								-1,
								-1,
								-1,
								INVALID_VALUE,
								values_min,
								compute_hat,
								-1,
								-1,
								no_gaps_filled_count
	);
}

/* */
GF_ROW *gf_mds_with_qc(PREC *values, const int struct_size, const int rows_count, const int columns_count, const int timeres,
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
																										int *no_gaps_filled_count) {
	return gf_mds_with_bounds(	values,
								struct_size,
								rows_count,
								columns_count,
								timeres,
								value1_tolerance_min,
								value1_tolerance_max,
								value2_tolerance_min,
								value2_tolerance_max,
								value3_tolerance_min,
								value3_tolerance_max,
								tofill_column,
								value1_column,
								value2_column,
								value3_column,
								value1_qc_column,
								value2_qc_column,
								value3_qc_column,
								qc_thrs,
								values_min,
								compute_hat,
								-1,
								-1,
								no_gaps_filled_count
	);
}

/* */
char *get_datetime_in_timestamp_format(void) {
	const char timestamp_format[] = "%04d%02d%02d%02d%02d%02d";
	static char buffer[14 + 1];
#if defined (_WIN32)
	SYSTEMTIME st;

	GetLocalTime(&st);
	sprintf(buffer, timestamp_format,	st.wYear,
										st.wMonth,
										st.wDay,
										st.wHour,
										st.wMinute,
										st.wSecond
	);
#elif defined (linux) || defined (__linux) || defined (__linux__) || defined (__APPLE__)
    struct timeval tval;
    struct tm *time;
    if (gettimeofday(&tval, NULL) != 0) {
        fprintf(stderr, "Fail of gettimeofday\n");
        return buffer;
    }
    time = localtime(&tval.tv_sec);
    if (time == NULL) {
        fprintf(stderr, "Fail of localtime\n");
        return buffer;
    }

    sprintf(buffer, timestamp_format,
            time->tm_year + 1900,
            time->tm_mon,
            time->tm_mday,
            time->tm_hour,
            time->tm_min,
            time->tm_sec
	);
#endif
	return buffer;
}

/* */
char *get_timeres_in_string(int timeres) {
	if ( (timeres < 0) || (timeres >= TIMERES_SIZE) ) {
		return NULL;
	}

	switch ( timeres ) {
		case SPOT_TIMERES: return "spot";
		case QUATERHOURLY_TIMERES: return "quaterhourly";
		case HALFHOURLY_TIMERES: return "halfhourly";
		case HOURLY_TIMERES: return "hourly";
		case DAILY_TIMERES: return "daily";
		case MONTHLY_TIMERES: return "monthly";
		default: return NULL;
	}
}

/* */
int get_valid_line_from_file(FILE *const f, char *buffer, const int size) {
	int i;

	if ( !f || !buffer || !size ) {
		return 0;
	}

	buffer[0] = '\0';
	while ( 1 ) {
		if ( ! fgets(buffer, size, f) ) {
			return 0;
		}

		/* remove carriage return and newline */
		for ( i = 0; buffer[i]; i++ ) {
			if ( ('\r' == buffer[i]) || ('\n' == buffer[i]) ) {
				buffer[i] = '\0';
				break;
			}
		}

		/* skip empty line */
		if ( buffer[0] != '\0' ) {
			break;
		}
	}
	return 1;
}

void check_memory_leak(void) {
#ifdef _DEBUG
#if _WIN32
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
	_CrtDumpMemoryLeaks();
#else /* _WIN32 */
	assert(0 && "no memory leak check available for this platform");
#endif
#endif /* _DEBUG */
}

int check_timestamp(const TIMESTAMP* const p)
{
	int hour;

	const int days_per_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	assert(p);

	if ( p->YYYY <= 0 ) return 0;
	if ( (p->MM <= 0) || (p->MM > 12) ) return 0;
	if ( p->DD <= 0 ) return 0;

	if ( 2 == p->MM ) // check for february leap
	{
		if ( p->DD > days_per_month[p->MM-1]
				+ IS_LEAP_YEAR(p->YYYY) )
		{
			return 0;
		}
	}
	else if ( p->DD > days_per_month[p->MM-1] )
	{
		return 0;
	}

	// fix 24 hh
	hour = p->hh;
	if ( (hour < 0) || (hour > 23) ) return 0;
	if ( (p->mm < 0) || (p->hh > 59) ) return 0;
	if ( (p->ss < 0) || (p->ss > 59) ) return 0;

	return 1;
}

static int timestamp_to_epoch(const TIMESTAMP* const p)
{
#define YEAR_0 (1900)
	int iYear;
	int iDoy;

	const int _iDoy[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

	iYear = p->YYYY - YEAR_0;
	iDoy = _iDoy[p->MM-1] + (p->DD-1);
	if ( IS_LEAP_YEAR(p->YYYY) && (p->MM > 2) ) ++iDoy;

	return p->ss + p->mm*60 + p->hh*3600 + iDoy*86400 +
	(iYear-70)*31536000 + ((iYear-69)/4)*86400 -
	((iYear-1)/100)*86400 + ((iYear+299)/400)*86400;
#undef YEAR_0
}

int timestamp_difference_in_seconds(const TIMESTAMP* const p1, const TIMESTAMP* const p2)
{
	int s1;
	int s2;

	assert(p1);
	assert(p2);

	s1 = timestamp_to_epoch(p1);
	s2 = timestamp_to_epoch(p2);

	return s1-s2;
}

#if defined (_WIN32) && defined (_DEBUG) 
void dump_memory_leaks(void) {
	_CrtDumpMemoryLeaks();
}
#endif
