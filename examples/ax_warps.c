/*
 * Copyright (C) 2016 Algolux Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

/*!
 * \file
 * \brief A Warp perspective example with OpenVX
 * \author Emeric Vigier <emeric.vigier@algolux.com>
 */

#include <assert.h>
#include <stdio.h>
#include <VX/vx.h>
#include <VX/vxu.h>
#include <VX/vx_lib_debug.h>
#include <VX/vx_helper.h>

#define IMAGE_WIDTH  4160
#define IMAGE_HEIGHT 2774

#define PERF_MILLISECOND 1000000.0f
#define PERF_MICROSECOND 1000.0f
#define PERF_NANOSECOND  1.0f
#define PERF_TIMEUNIT    PERF_MILLISECOND

#define CHECK_ALL_ITEMS(array, iter, status, label) { \
    status = VX_SUCCESS; \
    for ((iter) = 0; (iter) < dimof(array); (iter)++) { \
        if ((array)[(iter)] == 0) { \
            printf("Item %u in "#array" is null!\n", (iter)); \
            assert((array)[(iter)] != 0); \
            status = VX_ERROR_NOT_SUFFICIENT; \
        } \
    } \
    if (status != VX_SUCCESS) { \
        goto label; \
    } \
}

typedef struct ax_node {
    char name[256];
} ax_node_t;

void usage(const char *prg) {
    printf("USAGE: %s <input-img> <output-img>\n", prg);
}

void axPrintPerf(const char *name, vx_perf_t *perf) {
    printf("%10s (ms): sum:%12.3f avg:%12.3f min:%12.3f max:%12.3f num:%3lu\n", name,
            (vx_float32)perf->sum/PERF_TIMEUNIT,
            (vx_float32)perf->avg/PERF_TIMEUNIT,
            (vx_float32)perf->min/PERF_TIMEUNIT,
            (vx_float32)perf->max/PERF_TIMEUNIT,
            perf->num);
}

int main(int argc, char **argv) {
    vx_context ctx = NULL;
    vx_status ret = VX_FAILURE;
    vx_uint32 width = IMAGE_WIDTH, height = IMAGE_HEIGHT;
    vx_graph graph = NULL;
    int i;

    if (argc < 3) {
        usage(argv[0]);
        ret = -1;
        goto end;
    }

    vx_char *srcfilename = argv[1];
    vx_char *dstfilename = argv[2];
    printf("src img: %s\n", srcfilename);
    printf("dst img: %s\n", dstfilename);

    // create context
    ctx = vxCreateContext();
    if (!ctx) {
        fprintf(stderr, "error: vxCreateContext %d\n", ret);
        ret = -1;
        goto end;
    }

    // create images
    vx_image images[] = {
        vxCreateImage(ctx, width, height, VX_DF_IMAGE_U8),
        vxCreateImage(ctx, width, height, VX_DF_IMAGE_U8),
        vxCreateImage(ctx, width, height, VX_DF_IMAGE_U8),
        vxCreateImage(ctx, width, height, VX_DF_IMAGE_U8),
    };

    printf("created %lu images %u x %u\n", dimof(images), width, height);

    // load vxFReadImageNode kernel
    ret = vxLoadKernels(ctx, "openvx-debug");
    ret |= vxLoadKernels(ctx, "openvx-extras");

    // create graph
    graph = vxCreateGraph(ctx);
    ret = vxGetStatus((vx_reference)graph);
    if (ret != VX_SUCCESS) {
        fprintf(stderr, "error: vxCreateGraph %d\n", ret);
        goto relImg;
    }

    //! [warp perspective]
    // x0 = a x + b y + c;
    // y0 = d x + e y + f;
    // z0 = g x + h y + i;
    vx_float32 mat[3][3] = {
        {0.98f, -0.17f, 0.0f}, // 'x' coefficients
        {0.17f,  0.98f, 0.0f}, // 'y' coefficients
        {0.0f,   0.0f,  1.0f}, // offsets
    };

    vx_matrix matrix = vxCreateMatrix(ctx, VX_TYPE_FLOAT32, 3, 3);
    vxWriteMatrix(matrix, mat);

    // the pipeline definition
    vx_node nodes[] = {
        vxFReadImageNode(graph, srcfilename, images[0]),
        vxWarpPerspectiveNode(graph, images[0], matrix,
            VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR, images[1]),
        vxWarpPerspectiveNode(graph, images[1], matrix,
            VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR, images[2]),
        vxWarpPerspectiveNode(graph, images[2], matrix,
            VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR, images[3]),
        vxFWriteImageNode(graph, images[3], dstfilename),
    };
    CHECK_ALL_ITEMS(nodes, i, ret, relMat);

    ax_node_t axnodes[] = {
        { "Read" },
        { "Warp3x3" },
        { "Warp3x3" },
        { "Warp3x3" },
        { "Write" },
    };

//    ret = vx_useImmediateBorderMode(ctx, node[1]);
//    if (ret != VX_SUCCESS) {
//        fprintf(stderr, "error: vx_useImmediateBorderMode %d\n", ret);
//        goto relGraph;
//    }

    // validate pipeline
    ret = vxVerifyGraph(graph);
    if (ret != VX_SUCCESS) {
        fprintf(stderr, "error: vxVerifyGraph %d\n", ret);
        goto relNod;
    }

    // run pipeline!
    ret = vxProcessGraph(graph);

    vx_perf_t perf_node;
    vx_perf_t perf_graph;

    vxQueryGraph(graph, VX_GRAPH_ATTRIBUTE_PERFORMANCE, &perf_graph, sizeof(perf_graph));
    axPrintPerf("Graph", &perf_graph);

    for (i = 0; i < dimof(nodes); ++i) {
        vxQueryNode(nodes[i], VX_NODE_ATTRIBUTE_PERFORMANCE, &perf_node, sizeof(perf_node));
        axPrintPerf(axnodes[i].name, &perf_node);
    }

relNod:
    for (i = 0; i < dimof(nodes); ++i) {
        vxReleaseNode(&nodes[i]);
    }
relMat:
    vxReleaseMatrix(&matrix);
    vxReleaseGraph(&graph);
relImg:
    for (i = 0; i < dimof(images); ++i) {
        vxReleaseImage(&images[i]);
    }
    ret = vxReleaseContext(&ctx);
    if (ret != VX_SUCCESS) {
        fprintf(stderr, "error: vxReleaseContext %d\n", ret);
        goto end;
    }

     ret = 0;
end:
    return ret;
}
