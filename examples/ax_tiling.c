/*
 * Copyright (c) 2013-2014 The Khronos Group Inc.
 * Copyright (c) 2016 Algolux Inc.
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

#include <stdio.h>
#include <VX/vx.h>
#include <VX/vx_khr_tiling.h>
#include <VX/vx_lib_debug.h>
#include <VX/vx_helper.h>
#include "vx_tiling_ext.h"

#define PERF_MILLISECOND 1000000.0f
#define PERF_MICROSECOND 1000.0f
#define PERF_NANOSECOND  1.0f
#define PERF_TIMEUNIT    PERF_MILLISECOND

/*! \file
 * \brief An example of how to call the tiling nodes.
 * \example vx_tiling_main.c
 * \author Emeric Vigier <emeric.vigier@algolux.com>
 */

vx_node vxTilingAddNode(vx_graph graph, vx_image in0, vx_image in1, vx_image out)
{
    vx_reference params[] = {
        (vx_reference)in0,
        (vx_reference)in1,
        (vx_reference)out,
    };
    return vxCreateNodeByStructure(graph,
                                    VX_KERNEL_ADD_TILING,
                                    params,
                                    dimof(params));
}

vx_node vxTilingAlphaNode(vx_graph graph, vx_image in, vx_scalar alpha, vx_image out)
{
    vx_reference params[] = {
        (vx_reference)in,
        (vx_reference)alpha,
        (vx_reference)out,
    };
    return vxCreateNodeByStructure(graph,
                                    VX_KERNEL_ALPHA_TILING,
                                    params,
                                    dimof(params));
}

vx_node vxTilingBoxNode(vx_graph graph, vx_image in, vx_image out, vx_uint32 width, vx_uint32 height)
{
    vx_reference params[] = {
        (vx_reference)in,
        (vx_reference)out,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                    VX_KERNEL_BOX_MxN_TILING,
                                    params,
                                    dimof(params));
    if (node && (width&1) && (height&1))
    {
        vx_neighborhood_size_t nbhd;
        vxQueryNode(node, VX_NODE_ATTRIBUTE_INPUT_NEIGHBORHOOD, &nbhd, sizeof(nbhd));
        nbhd.left = 0 - ((width - 1)/2);
        nbhd.right = ((width - 1)/2);
        nbhd.top = 0 - ((height - 1)/2);
        nbhd.bottom = ((height - 1)/2);
        vxSetNodeAttribute(node, VX_NODE_ATTRIBUTE_INPUT_NEIGHBORHOOD, &nbhd, sizeof(nbhd));
    }
    return node;
}

vx_node vxTilingGaussianNode(vx_graph graph, vx_image in, vx_image out)
{
    vx_reference params[] = {
        (vx_reference)in,
        (vx_reference)out,
    };
    return vxCreateNodeByStructure(graph,
                                    VX_KERNEL_GAUSSIAN_3x3_TILING,
                                    params,
                                    dimof(params));
}

typedef struct ax_node {
    vx_node node;
    char name[256];
} ax_node_t;

void usage(const char *prg) {
    printf("USAGE: %s <input-img>\n", prg);
}

void axPrintPerf(const char *name, vx_perf_t *perf) {
    printf("%10s (ms): sum:%12.3f avg:%12.3f min:%12.3f max:%12.3f num:%3lu\n", name,
            (vx_float32)perf->sum/PERF_TIMEUNIT,
            (vx_float32)perf->avg/PERF_TIMEUNIT,
            (vx_float32)perf->min/PERF_TIMEUNIT,
            (vx_float32)perf->max/PERF_TIMEUNIT,
            perf->num);
}

int main(int argc, char *argv[]) {
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();

    if (argc < 2) {
        usage(argv[0]);
        goto relCtx;
    }

    vx_char *srcfilename = argv[1];
    printf("src img: %s\n", srcfilename);

    FILE *fp = fopen(srcfilename, "r");
    if (!fp) {
        goto relCtx;
    }

    char pgmstr[1024];
    unsigned int n;
    n = fread(pgmstr, 1, sizeof(pgmstr), fp);
    if (n != sizeof(pgmstr)) {
        goto relClose;
    }

    const char delim = '\n';
    const char *token = NULL;
    unsigned int width, height;

    // PGM P5 magic string
    token = strtok(pgmstr, &delim);
    // PGM author
    token = strtok(NULL, &delim);
    // PGM image size
    token = strtok(NULL, &delim);
    sscanf(token, "%u %u", &width, &height);
    printf("width:%u height:%u\n", width, height);

    status = vxGetStatus((vx_reference)context);
    if (status != VX_SUCCESS) {
        fprintf(stderr, "error: vxCreateContext\n");
        goto relClose;
    }

    vx_rectangle_t rect = {1, 1, width + 1, height + 1};
    vx_uint32 i = 0;
    vx_image images[] = {
            vxCreateImage(context, width + 2, height + 2, VX_DF_IMAGE_U8), // 0:input
            vxCreateImageFromROI(images[0], &rect),       // 1:ROI input
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), // 2:box
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), // 3:gaussian
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), // 4:alpha
            vxCreateImage(context, width, height, VX_DF_IMAGE_S16),// 5:add
    };

    vx_float32 a = 0.5f;
    vx_scalar alpha = vxCreateScalar(context, VX_TYPE_FLOAT32, &a);
    status |= vxLoadKernels(context, "openvx-tiling");
    status |= vxLoadKernels(context, "openvx-debug");
    if (status != VX_SUCCESS) {
        fprintf(stderr, "error: vxLoadKernels %d\n", status);
        goto relImg;
    }

    vx_graph graph = vxCreateGraph(context);
    status = vxGetStatus((vx_reference)context);
    if (status != VX_SUCCESS) {
        fprintf(stderr, "error: vxGetStatus\n");
        goto relKern;
    }

    ax_node_t axnodes[] = {
        { vxFReadImageNode(graph, srcfilename, images[1]), "Read" },
        { vxTilingBoxNode(graph, images[1], images[2], 5, 5), "Box" },
        { vxFWriteImageNode(graph, images[2], "ot_box.pgm"), "Write" },
        { vxTilingGaussianNode(graph, images[1], images[3]), "Gaussian" },
        { vxFWriteImageNode(graph, images[3], "ot_gauss.pgm"), "Write" },
        { vxTilingAlphaNode(graph, images[1], alpha, images[4]), "Alpha" },
        { vxFWriteImageNode(graph, images[4], "ot_alpha.pgm"), "Write" },
        { vxTilingAddNode(graph, images[1], images[4], images[5]), "Add" },
        { vxFWriteImageNode(graph, images[5], "ot_add.pgm"), "Write" },
    };

    for (i = 0; i < dimof(axnodes); i++) {
        if (axnodes[i].node == 0) {
            fprintf(stderr, "error: Failed to create node[%u]\n", i);
            status = VX_ERROR_INVALID_NODE;
            goto relNod;
        }
    }

    status = vxVerifyGraph(graph);
    if (status != VX_SUCCESS) {
        fprintf(stderr, "error: vxVerifyGraph %d\n", status);
        goto relNod;
    }

    status = vxProcessGraph(graph);
    if (status != VX_SUCCESS) {
        fprintf(stderr, "error: vxProcessGraph %d\n", status);
        goto relNod;
    }

    // perf timings
    vx_perf_t perf_node;
    vx_perf_t perf_graph;

    vxQueryGraph(graph, VX_GRAPH_ATTRIBUTE_PERFORMANCE, &perf_graph, sizeof(perf_graph));
    axPrintPerf("Graph", &perf_graph);

    for (i = 0; i < dimof(axnodes); ++i) {
        vxQueryNode(axnodes[i].node, VX_NODE_ATTRIBUTE_PERFORMANCE, &perf_node, sizeof(perf_node));
        axPrintPerf(axnodes[i].name, &perf_node);
    }
relNod:
    for (i = 0; i < dimof(axnodes); i++) {
        vxReleaseNode(&axnodes[i].node);
    }
    vxReleaseGraph(&graph);

relKern:
relImg:
    for (i = 0; i < dimof(images); i++) {
        vxReleaseImage(&images[i]);
    }
relClose:
    fclose(fp);
relCtx:
    vxReleaseContext(&context);

    printf("%s::main() returns = %d\n", argv[0], status);
    return (int)status;
}
