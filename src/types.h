/*
	types.h

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

#ifndef TYPES_H
#define TYPES_H

/* includes */
#include "common.h"

/* enums */
enum {
	GF_TOFILL = 0,
	GF_DRIVER_1,
	GF_DRIVER_2A,
	GF_DRIVER_2B,
	GF_ROW_INDEX,

	GF_REQUIRED_DATASET_VALUES,

	GF_TOKENS = GF_REQUIRED_DATASET_VALUES,
};

/* structures */
typedef struct {
	PREC value[GF_REQUIRED_DATASET_VALUES];
	int assigned;
} ROW;

/* */
#endif /* TYPES_H */
