/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <cuda_runtime.h>

#include "cudss.h"

/*
    This example demonstrates basic usage of cuDSS APIs for extracting
    the row and column reordering information that is generated in the
    REORDERING phase.
    A system of linear algebraic equations with a sparse matrix is solved
    to show the extraction process:
                                Ax = b,
    where:
        A is the sparse input matrix,
        b is the (dense) right-hand side vector (or a matrix),
        x is the (dense) solution vector (or a matrix).
*/

#define CUDSS_EXAMPLE_FREE                                                                                             \
    do {                                                                                                               \
        free(csr_offsets_h);                                                                                           \
        free(csr_columns_h);                                                                                           \
        free(csr_values_h);                                                                                            \
        free(x_values_h);                                                                                              \
        free(b_values_h);                                                                                              \
        free(row_perm);                                                                                                \
        free(col_perm);                                                                                                \
        cudaFree(csr_offsets_d);                                                                                       \
        cudaFree(csr_columns_d);                                                                                       \
        cudaFree(csr_values_d);                                                                                        \
        cudaFree(x_values_d);                                                                                          \
        cudaFree(b_values_d);                                                                                          \
    } while (0);

#define CUDA_CALL_AND_CHECK(call, msg)                                                                                 \
    do {                                                                                                               \
        cuda_error = call;                                                                                             \
        if (cuda_error != cudaSuccess) {                                                                               \
            printf("Example FAILED: CUDA API returned error = %d, details: " #msg "\n", cuda_error);                   \
            CUDSS_EXAMPLE_FREE;                                                                                        \
            return -1;                                                                                                 \
        }                                                                                                              \
    } while (0);

#define CUDSS_CALL_AND_CHECK(call, status, msg)                                                                        \
    do {                                                                                                               \
        status = call;                                                                                                 \
        if (status != CUDSS_STATUS_SUCCESS) {                                                                          \
            printf("Example FAILED: CUDSS call ended unsuccessfully with status = %d, details: " #msg "\n", status);   \
            CUDSS_EXAMPLE_FREE;                                                                                        \
            return -2;                                                                                                 \
        }                                                                                                              \
    } while (0);

int main(int argc, char *argv[]) {
    printf("---------------------------------------------------------\n");
    printf("cuDSS example: solving a real linear 5x5 system\n"
           "with a symmetric positive-definite matrix\n"
           "with the extraction of the reordering information\n"
           "immediately after the reordering phase\n");
    printf("---------------------------------------------------------\n");
    cudaError_t   cuda_error = cudaSuccess;
    cudssStatus_t status     = CUDSS_STATUS_SUCCESS;

    const int n    = 5;
    const int nnz  = 8;
    const int nrhs = 1;

    int    *csr_offsets_h = NULL;
    int    *csr_columns_h = NULL;
    double *csr_values_h  = NULL;
    double *x_values_h = NULL, *b_values_h = NULL;

    int    *csr_offsets_d = NULL;
    int    *csr_columns_d = NULL;
    double *csr_values_d  = NULL;
    double *x_values_d = NULL, *b_values_d = NULL;

    int *row_perm = NULL;
    int *col_perm = NULL;

    /* Allocate host memory for the sparse input matrix A,
       right-hand side x and solution b*/

    csr_offsets_h = (int *)malloc((n + 1) * sizeof(int));
    csr_columns_h = (int *)malloc(nnz * sizeof(int));
    csr_values_h  = (double *)malloc(nnz * sizeof(double));
    x_values_h    = (double *)malloc(nrhs * n * sizeof(double));
    b_values_h    = (double *)malloc(nrhs * n * sizeof(double));
    row_perm      = (int *)malloc(n * sizeof(int));
    col_perm      = (int *)malloc(n * sizeof(int));

    const int row_perm_expected[5] = {4, 3, 1, 0, 2};
    const int col_perm_expected[5] = {4, 3, 1, 0, 2};

    if (!csr_offsets_h || !csr_columns_h || !csr_values_h || !x_values_h || !b_values_h) {
        printf("Error: host memory allocation failed\n");
        return -1;
    }

    /* Initialize host memory for A and b */
    int i              = 0;
    csr_offsets_h[i++] = 0;
    csr_offsets_h[i++] = 2;
    csr_offsets_h[i++] = 4;
    csr_offsets_h[i++] = 6;
    csr_offsets_h[i++] = 7;
    csr_offsets_h[i++] = 8;

    i                  = 0;
    csr_columns_h[i++] = 0;
    csr_columns_h[i++] = 2;
    csr_columns_h[i++] = 1;
    csr_columns_h[i++] = 2;
    csr_columns_h[i++] = 2;
    csr_columns_h[i++] = 4;
    csr_columns_h[i++] = 3;
    csr_columns_h[i++] = 4;

    i                 = 0;
    csr_values_h[i++] = 4.0;
    csr_values_h[i++] = 1.0;
    csr_values_h[i++] = 3.0;
    csr_values_h[i++] = 2.0;
    csr_values_h[i++] = 5.0;
    csr_values_h[i++] = 1.0;
    csr_values_h[i++] = 1.0;
    csr_values_h[i++] = 2.0;

    /* Note: Right-hand side b is initialized with values which correspond
       to the exact solution vector {1, 2, 3, 4, 5} */
    i               = 0;
    b_values_h[i++] = 7.0;
    b_values_h[i++] = 12.0;
    b_values_h[i++] = 25.0;
    b_values_h[i++] = 4.0;
    b_values_h[i++] = 13.0;

    /* Allocate device memory for A, x and b */
    CUDA_CALL_AND_CHECK(cudaMalloc(&csr_offsets_d, (n + 1) * sizeof(int)), "cudaMalloc for csr_offsets");
    CUDA_CALL_AND_CHECK(cudaMalloc(&csr_columns_d, nnz * sizeof(int)), "cudaMalloc for csr_columns");
    CUDA_CALL_AND_CHECK(cudaMalloc(&csr_values_d, nnz * sizeof(double)), "cudaMalloc for csr_values");
    CUDA_CALL_AND_CHECK(cudaMalloc(&b_values_d, nrhs * n * sizeof(double)), "cudaMalloc for b_values");
    CUDA_CALL_AND_CHECK(cudaMalloc(&x_values_d, nrhs * n * sizeof(double)), "cudaMalloc for x_values");

    /* Copy host memory to device for A and b */
    CUDA_CALL_AND_CHECK(cudaMemcpy(csr_offsets_d, csr_offsets_h, (n + 1) * sizeof(int), cudaMemcpyHostToDevice),
                        "cudaMemcpy for csr_offsets");
    CUDA_CALL_AND_CHECK(cudaMemcpy(csr_columns_d, csr_columns_h, nnz * sizeof(int), cudaMemcpyHostToDevice),
                        "cudaMemcpy for csr_columns");
    CUDA_CALL_AND_CHECK(cudaMemcpy(csr_values_d, csr_values_h, nnz * sizeof(double), cudaMemcpyHostToDevice),
                        "cudaMemcpy for csr_values");
    CUDA_CALL_AND_CHECK(cudaMemcpy(b_values_d, b_values_h, nrhs * n * sizeof(double), cudaMemcpyHostToDevice),
                        "cudaMemcpy for b_values");

    /* Create a CUDA stream */
    cudaStream_t stream = NULL;
    CUDA_CALL_AND_CHECK(cudaStreamCreate(&stream), "cudaStreamCreate");

    /* Creating the cuDSS library handle */
    cudssHandle_t handle;

    CUDSS_CALL_AND_CHECK(cudssCreate(&handle), status, "cudssCreate");

    /* (optional) Setting the custom stream for the library handle */
    CUDSS_CALL_AND_CHECK(cudssSetStream(handle, stream), status, "cudssSetStream");

    /* Creating cuDSS solver configuration and data objects */
    cudssConfig_t solverConfig;
    cudssData_t   solverData;

    CUDSS_CALL_AND_CHECK(cudssConfigCreate(&solverConfig), status, "cudssConfigCreate");
    CUDSS_CALL_AND_CHECK(cudssDataCreate(handle, &solverData), status, "cudssDataCreate");

    /* Create matrix objects for the right-hand side b and solution x (as dense matrices). */
    cudssMatrix_t x, b;

    int64_t nrows = n, ncols = n;
    int     ldb = ncols, ldx = nrows;
    CUDSS_CALL_AND_CHECK(cudssMatrixCreateDn(&b, ncols, nrhs, ldb, b_values_d, CUDA_R_64F, CUDSS_LAYOUT_COL_MAJOR),
                         status, "cudssMatrixCreateDn for b");
    CUDSS_CALL_AND_CHECK(cudssMatrixCreateDn(&x, nrows, nrhs, ldx, x_values_d, CUDA_R_64F, CUDSS_LAYOUT_COL_MAJOR),
                         status, "cudssMatrixCreateDn for x");

    /* Create a matrix object for the sparse input matrix. */
    cudssMatrix_t         A;
    cudssMatrixType_t     mtype = CUDSS_MTYPE_SPD;
    cudssMatrixViewType_t mview = CUDSS_MVIEW_UPPER;
    cudssIndexBase_t      base  = CUDSS_BASE_ZERO;
    CUDSS_CALL_AND_CHECK(cudssMatrixCreateCsr(&A, nrows, ncols, nnz, csr_offsets_d, NULL, csr_columns_d, csr_values_d,
                                              CUDA_R_32I, CUDA_R_64F, mtype, mview, base),
                         status, "cudssMatrixCreateCsr");

    /* Generate the reordering isolated */
    CUDSS_CALL_AND_CHECK(cudssExecute(handle, CUDSS_PHASE_REORDERING, solverConfig, solverData, A, x, b), status,
                         "cudssExecute for reordering");

    std::size_t rowSizeWritten{};
    std::size_t colSizeWritten{};
    CUDSS_CALL_AND_CHECK(
        cudssDataGet(handle, solverData, CUDSS_DATA_PERM_REORDER_ROW, row_perm, n * sizeof(int), &rowSizeWritten),
        status, "cudssDataGet for reorder row perm");

    CUDSS_CALL_AND_CHECK(
        cudssDataGet(handle, solverData, CUDSS_DATA_PERM_REORDER_COL, col_perm, n * sizeof(int), &colSizeWritten),
        status, "cudssDataGet for reorder col perm");

    /* Finish the rest of the solve */
    CUDSS_CALL_AND_CHECK(
        cudssExecute(handle, CUDSS_PHASE_SYMBOLIC_FACTORIZATION | CUDSS_PHASE_FACTORIZATION | CUDSS_PHASE_SOLVE,
                     solverConfig, solverData, A, x, b),
        status, "cudssExecute for everything combined");

    /* Destroying opaque objects, matrix wrappers and the cuDSS library handle */
    CUDSS_CALL_AND_CHECK(cudssMatrixDestroy(A), status, "cudssMatrixDestroy for A");
    CUDSS_CALL_AND_CHECK(cudssMatrixDestroy(b), status, "cudssMatrixDestroy for b");
    CUDSS_CALL_AND_CHECK(cudssMatrixDestroy(x), status, "cudssMatrixDestroy for x");
    CUDSS_CALL_AND_CHECK(cudssDataDestroy(handle, solverData), status, "cudssDataDestroy");
    CUDSS_CALL_AND_CHECK(cudssConfigDestroy(solverConfig), status, "cudssConfigDestroy");
    CUDSS_CALL_AND_CHECK(cudssDestroy(handle), status, "cudssHandleDestroy");

    CUDA_CALL_AND_CHECK(cudaStreamSynchronize(stream), "cudaStreamSynchronize");

    /* Print the solution and compare against the exact solution */
    CUDA_CALL_AND_CHECK(cudaMemcpy(x_values_h, x_values_d, nrhs * n * sizeof(double), cudaMemcpyDeviceToHost),
                        "cudaMemcpy for x_values");

    bool passed = true;
    for (int i = 0; i < n; i++) {
        printf("x[%d] = %1.4f expected %1.4f\n", i, x_values_h[i], double(i + 1));
        if (fabs(x_values_h[i] - (i + 1)) > 2.e-15)
            passed = false;
    }

    if (rowSizeWritten != n * sizeof(int)) {
        printf("row sizes written = %lu, expected %lu\n", rowSizeWritten, n * sizeof(int));
        passed = false;
    }
    if (colSizeWritten != n * sizeof(int)) {
        printf("col sizes written = %lu, expected %lu\n", colSizeWritten, n * sizeof(int));
        passed = false;
    }
    for (int i = 0; i < n; i++) {
        printf("row_perm[%d] = %d expected %d\n", i, row_perm[i], row_perm_expected[i]);
        if (row_perm[i] != row_perm_expected[i])
            passed = false;
    }
    for (int i = 0; i < n; i++) {
        printf("col_perm[%d] = %d expected %d\n", i, col_perm[i], col_perm_expected[i]);
        if (col_perm[i] != col_perm_expected[i])
            passed = false;
    }

    /* Release the data allocated on the user side */

    CUDSS_EXAMPLE_FREE;

    if (status == CUDSS_STATUS_SUCCESS && passed) {
        printf("Example PASSED\n");
        return 0;
    } else {
        printf("Example FAILED\n");
        return -1;
    }
}