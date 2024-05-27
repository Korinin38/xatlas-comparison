/*
MIT License

Copyright (c) 2018-2020 Jonathan Young

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
/*
example

This example shows how to use the xatlas API to generate a unique set of texture coordinates.

Input: an .obj model file.

Output:
	* an .obj model file (example_output.obj). This is simplistic for example purposes, it doesn't copy materials from the input .obj file.
	* texture coordinates rasterized to images, colored by chart (example_charts*.tga) and by triangle (example_tris*.tga).
*/
#include <mutex>
#include <cassert>
#include <cstdarg>
#include <cstdio>

#include <stb_image/stb_image_write.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif

#include <tiny_obj_loader/tiny_obj_loader.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <xatlas-old/xatlas.h>

#ifdef _MSC_VER
#define FOPEN(_file, _filename, _mode) { if (fopen_s(&_file, _filename, _mode) != 0) _file = NULL; }
#define STRICMP _stricmp
#else
#define FOPEN(_file, _filename, _mode) _file = fopen(_filename.c_str(), _mode)


#define STRICMP strcasecmp
#endif

#define OBJ_TRIANGULATE 1 // Pass tinyobj::triangulation flag to tinyobjloader and treat all geometry as triangles.
#define PACK_PREPARED 1

#include "parameters.h"

#define MAKE_TEN_OF_THEM 0

#define DEBUG_STAT_TO_FILE 0

#include <utils/timer.h>
#include <happly.h>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace x_old_example {
	static std::string log_output = "data/debug/plots/logs/profiling";
	static bool out_to_file = false;

	static bool s_verbose = false;

	static int Print(const char *format, ...) {
        va_list arg;
        va_start(arg, format);
        printf("\r"); // Clear progress text.
        const int result = vprintf(format, arg);
        va_end(arg);
        return result;
    }

    // May be called from any thread.
    static bool ProgressCallback(xatlas_old::ProgressCategory category, int progress, void *userData) {
        // Don't interupt verbose printing.
        if (s_verbose)
            return true;
        auto t = (timer *) userData;
        static std::mutex progressMutex;
        std::unique_lock<std::mutex> lock(progressMutex);
        if (progress == 0)
            t->restart();
        FILE *f;
		if (out_to_file) FOPEN(f, log_output, "a");
        printf("\r   %s [", xatlas_old::StringForEnum(category));
        if (out_to_file) fprintf(f, "\n   %s [", xatlas_old::StringForEnum(category));
        for (int i = 0; i < 10; i++) {
            printf(progress / ((i + 1) * 10) ? "*" : " ");
            if (out_to_file) fprintf(f, progress / ((i + 1) * 10) ? "*" : " ");
        }
        printf("] %d%% (%.2f s)", progress, t->elapsed());
        if (out_to_file) fprintf(f, "] %d%% (%.2f s)", progress, t->elapsed());
        fflush(stdout);
        if (progress == 100) {
            printf("\n      %.2f seconds (%g ms) elapsed\n", t->elapsed(), t->elapsed() * 1000);
            if (out_to_file) fprintf(f, "\n      %.2f seconds (%g ms) elapsed\n", t->elapsed(), t->elapsed() * 1000);
        }
		if (out_to_file) fclose(f);
        return true;
    }

// Write meshes.
    void export_obj(const xatlas_old::Atlas *const atlas,
                    const std::vector<tinyobj::shape_t> &shapes,
                    const std::string &name,
                    bool flat = false) {
        std::string filename = name + "_x" + (flat ? "_charts" : "") + ".obj";
//        printf("Writing '%s'... ", filename.c_str());
        FILE *file;
        FOPEN(file, filename, "w");
        if (file) {
            fprintf(file, "mtllib texture/uvcoord.mtl\nusemtl UV\n");
            uint32_t firstVertex = 0;
            for (uint32_t i = 0; i < atlas->meshCount; i++) {
                const xatlas_old::Mesh &mesh = atlas->meshes[i];
                for (uint32_t v = 0; v < mesh.vertexCount; v++) {
                    const xatlas_old::Vertex &vertex = mesh.vertexArray[v];
                    const float *pos = &shapes[i].mesh.positions[vertex.xref * 3];
                    if (!flat) {
                        fprintf(file, "v %g %g %g\n", pos[0], pos[1], pos[2]);
                    } else {
                        // TODO: Solve issue with uv=(0, 0)
                        fprintf(file, "v %g %g %g\n",
                                vertex.uv[0] / atlas->width + vertex.atlasIndex,
                                vertex.uv[1] / atlas->height,
                                0.f);
                    }
                    if (!shapes[i].mesh.normals.empty()) {
                        const float *normal = &shapes[i].mesh.normals[vertex.xref * 3];
                        fprintf(file, "vn %g %g %g\n", normal[0], normal[1], normal[2]);
                    }
//                    if (!shapes[i].mesh.texcoords.empty()) {
                    fprintf(file, "vt %g %g\n", vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height);
//                    }
                }
                fprintf(file, "o %s\n", shapes[i].name.c_str());
                fprintf(file, "s off\n");

                for (uint32_t f = 0; f < mesh.indexCount; f += 3) {
//                     TODO: Solve issue with uv=(0, 0)
//                    bool valid_vertex = true;
//                    for (uint32_t j = 0; j < 3; j++) {
//                        if (mesh.vertexArray[mesh.indexArray[f + j]].atlasIndex == -1) {
//                            valid_vertex = false;
//                            break;
//                        }
//                    }
//                    if (!valid_vertex) {
//                        continue;
//                    }
                    fprintf(file, "f ");
                    for (uint32_t j = 0; j < 3; j++) {
                        const uint32_t index = firstVertex + mesh.indexArray[f + j] + 1; // 1-indexed
                        fprintf(file, "%d/%d/%d%c", index, index, index, j == 2 ? '\n' : ' ');
                    }
                }
                firstVertex += mesh.vertexCount;
            }
            fclose(file);
//            printf("Done!\n");
        }
    }

    void export_ply(const xatlas_old::Atlas *const atlas,
                    const std::vector<tinyobj::shape_t> &shapes,
                    const std::string &name, bool flat = false) {
        std::string filename = name + "_old" + (flat ? "_charts" : "") + ".ply";
        std::vector<std::vector<float>> vertices(3);
        std::vector<std::vector<int>> faces;

        int firstVertex = 0;
        for (int i = 0; i < atlas->meshCount; ++i) {
            const xatlas_old::Mesh &mesh = atlas->meshes[i];
            for (int v = 0; v < mesh.vertexCount; ++v) {
                const xatlas_old::Vertex &vertex = mesh.vertexArray[v];
                if (!flat) {
                    const float *pos = &shapes[i].mesh.positions[vertex.xref * 3];
                    vertices[0].push_back(pos[0]);
                    vertices[1].push_back(pos[1]);
                    vertices[2].push_back(pos[2]);
                } else {
                    vertices[0].push_back(vertex.uv[0] / atlas->width + vertex.atlasIndex);
                    vertices[1].push_back(vertex.uv[1] / atlas->height);
                    vertices[2].push_back(0);
                }
            }

            for (uint32_t f = 0; f < mesh.indexCount; f += 3) {
//                bool valid_vertex = true;
//                for (uint32_t j = 0; j < 3; j++) {
//                    if (mesh.vertexArray[mesh.indexArray[f + j]].atlasIndex == -1) {
//                        valid_vertex = false;
//                        break;
//                    }
//                }
//                if (!valid_vertex) {
//                    continue;
//                }
                std::vector<int> indices;
                for (uint32_t j = 0; j < 3; j++) {
                    const int index = firstVertex + mesh.indexArray[f + j];
                    indices.push_back(index);
                }
                faces.push_back(indices);
            }
            firstVertex += mesh.vertexCount;
        }

        happly::PLYData plyOut;
        plyOut.addElement("vertex", vertices[0].size());
        plyOut.addElement("face", faces.size());

        plyOut.getElement("vertex").addProperty<float>("x", vertices[0]);
        plyOut.getElement("vertex").addProperty<float>("y", vertices[1]);
        plyOut.getElement("vertex").addProperty<float>("z", vertices[2]);
        plyOut.getElement("face").addListProperty<int>("vertex_index", faces);

        plyOut.write(filename);
    }


    void debug_export_ply(const xatlas_old::Atlas *const atlas,
                          const std::vector<tinyobj::shape_t> &shapes,
                          const std::string &name, bool flat = false) {
        (void) flat;
        std::string filename = name + "_debug_x_charts.ply";
        std::vector<std::vector<float>> vertices(3);
        std::vector<std::vector<int>> faces;

        int firstVertex = 0;
        for (int i = 0; i < atlas->meshCount; ++i) {
            const xatlas_old::Mesh &mesh = atlas->meshes[i];
            for (int v = 0; v < mesh.vertexCount; ++v) {
                const xatlas_old::Vertex &vertex = mesh.vertexArray[v];
                if (v + firstVertex == 256608
                    || v + firstVertex == 263186
                    || v + firstVertex == 217898
                    || v + firstVertex == 256609) {
                    std::cout << vertex.xref << " " << vertex.chartIndex << std::endl;
                    auto *uv = &shapes[i].mesh.texcoords[vertex.xref * 2];
                    vertices[0].push_back(uv[0]);
                    vertices[1].push_back(uv[1]);
                    vertices[2].push_back(0);
                }
            }

            firstVertex += mesh.vertexCount;
        }

        happly::PLYData plyOut;
        plyOut.addElement("vertex", vertices[0].size());

        plyOut.getElement("vertex").addProperty<float>("x", vertices[0]);
        plyOut.getElement("vertex").addProperty<float>("y", vertices[1]);
        plyOut.getElement("vertex").addProperty<float>("z", vertices[2]);

        plyOut.write(filename);
    }

	void print_options(const xatlas_old::PackOptions &options) {
		std::cout << "Packing options:\n"
				  << "    maxChartSize:       " << options.maxChartSize << "\n"
				  << "    padding:            " << options.padding << "\n"
				  << "    texelsPerUnit:      " << options.texelsPerUnit << "\n"
				  << "    resolution:         " << options.resolution << "\n"
				  << "    bilinear:           " << (options.bilinear ? "true" : "false") << "\n"
				  << "    blockAlign:         " << (options.blockAlign ? "true" : "false") << "\n"
				  << "    bruteForce:         " << (options.bruteForce ? "true" : "false") << "\n"
				  << "    createImage:        " << (options.createImage ? "true" : "false") << "\n"
				  << "    rotateChartsToAxis: " << (options.rotateChartsToAxis ? "true" : "false") << "\n"
				  << "    rotateCharts:       " << (options.rotateCharts ? "true" : "false") << std::endl;
	}

	xatlas_old::PackOptions set_options(std::ostringstream &log, bool silent = false) {
        xatlas_old::PackOptions options;

        options.maxChartSize = 0;
        options.padding = PADDING;
#if TEXELS_NORMAL
        options.texelsPerUnit = 1.f * RESOLUTION;
#else
        options.texelsPerUnit = 1.f;
#endif
        options.resolution = RESOLUTION;
        options.bilinear = false;
        options.blockAlign = BLOCK_ALIGN;
        options.bruteForce = BRUTE_FORCE;
        options.createImage = false;
        options.rotateChartsToAxis = false;
        options.rotateCharts = TRANSPOSE;

		log << options.resolution << "," << options.bruteForce << ",0,0," << options.rotateCharts << ",";
		if (!silent) print_options(options);
        return options;
    }

    int main(int argc, char *argv[]) {
		auto tim = std::time(nullptr);
		auto tim1 = *std::localtime(&tim);
		std::ostringstream oss;
		oss << log_output << "_" << std::put_time(&tim1, "%Y.%m.%d_%H:%M:%S") << ".log";
		log_output = oss.str();

        if (argc < 3) {
            printf("Usage: %s input_file output_files_name\n", argv[0]);
            printf("Note: 2nd argument does not contain file extension.\n");
            return 1;
        }
        std::string input_name = argv[1];
        std::string output_name = argv[2];

		bool dont_print_options = argc > 3 && !std::string(argv[3]).empty();

		std::ostringstream log_results_oss;
		log_results_oss << "\n" << output_name << std::put_time(&tim1, ",%Y.%m.%d %H:%M:%S,1,");
		output_name += (BRUTE_FORCE) ? "_brute" : "_rand";

		if (out_to_file) {
            FILE *f;
            FOPEN(f, log_output, "w");
            fclose(f);
        }
        // Load object file.
        printf("Loading '%s'...\n", input_name.c_str());

        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        unsigned int flags = tinyobj::triangulation;
        if (!tinyobj::LoadObj(shapes, materials, err, input_name.c_str(), nullptr, flags)) {
            printf("Error: %s\n", err.c_str());
            return EXIT_FAILURE;
        }
        if (shapes.empty()) {
            printf("Error: no shapes in obj file\n");
            return EXIT_FAILURE;
        }
        printf("   %d shapes\n", (int) shapes.size());
        // Create empty atlas.
        xatlas_old::SetPrint(Print, s_verbose);
        xatlas_old::Atlas *atlas = xatlas_old::Create();
        // Set progress callback.
        timer globalT, t;
        xatlas_old::SetProgressCallback(atlas, ProgressCallback, &t);
        // Add meshes to atlas.
        uint32_t totalVertices = 0, totalFaces = 0;

#if PACK_PREPARED
#if MAKE_TEN_OF_THEM
		for (int j = 0; j < 10; j++) {
			int i = 0;
#else
		for (int i = 0; i < (int) shapes.size(); i++) {
#endif
            const tinyobj::mesh_t &objMesh = shapes[i].mesh;
            xatlas_old::UvMeshDecl uvMeshDecl;
			uvMeshDecl.faceMaterialData = (const uint32_t *) objMesh.material_ids.data();
			if (objMesh.texcoords.empty()) {
				uvMeshDecl.vertexCount = (uint32_t) objMesh.positions.size() / 3;
				uvMeshDecl.vertexUvData = objMesh.positions.data();
				uvMeshDecl.vertexStride = sizeof(float) * 3;
			} else {
				uvMeshDecl.vertexCount = (uint32_t) objMesh.texcoords.size() / 2;
				uvMeshDecl.vertexUvData = objMesh.texcoords.data();
				uvMeshDecl.vertexStride = sizeof(float) * 2;
			}
            uvMeshDecl.indexCount = (uint32_t) objMesh.indices.size();
            uvMeshDecl.indexData = objMesh.indices.data();
            uvMeshDecl.indexFormat = xatlas_old::IndexFormat::UInt32;

            xatlas_old::AddMeshError error = xatlas_old::AddUvMesh(atlas, uvMeshDecl);
            if (error != xatlas_old::AddMeshError::Success) {
                xatlas_old::Destroy(atlas);
                printf("\rError adding mesh %d '%s': %s\n", i, shapes[i].name.c_str(), xatlas_old::StringForEnum(error));
                return EXIT_FAILURE;
            }
            totalVertices += uvMeshDecl.vertexCount;
            totalFaces += uvMeshDecl.indexCount / 3;
        }
#else
        for (int i = 0; i < (int) shapes.size(); i++) {
            const tinyobj::mesh_t &objMesh = shapes[i].mesh;
            xatlas_old::MeshDecl meshDecl;
            meshDecl.vertexCount = (uint32_t) objMesh.positions.size() / 3;
            meshDecl.vertexPositionData = objMesh.positions.data();
            meshDecl.vertexPositionStride = sizeof(float) * 3;
            if (!objMesh.normals.empty()) {
                meshDecl.vertexNormalData = objMesh.normals.data();
                meshDecl.vertexNormalStride = sizeof(float) * 3;
            }
            if (!objMesh.texcoords.empty()) {
                meshDecl.vertexUvData = objMesh.texcoords.data();
                meshDecl.vertexUvStride = sizeof(float) * 2;
            }
            meshDecl.indexCount = (uint32_t) objMesh.indices.size();
            meshDecl.indexData = objMesh.indices.data();
            meshDecl.indexFormat = xatlas_old::IndexFormat::UInt32;
#if !OBJ_TRIANGULATE
            if (objMesh.num_vertices.size() != objMesh.indices.size() / 3) {
                meshDecl.faceVertexCount = objMesh.num_vertices.data();
                meshDecl.faceCount = (uint32_t) objMesh.num_vertices.size();
            }
#endif
            xatlas_old::AddMeshError error = xatlas_old::AddMesh(atlas, meshDecl, (uint32_t) shapes.size());
            if (error != xatlas_old::AddMeshError::Success) {
                xatlas_old::Destroy(atlas);
                printf("\rError adding mesh %d '%s': %s\n", i, shapes[i].name.c_str(), xatlas_old::StringForEnum(error));
                return EXIT_FAILURE;
            }
            totalVertices += meshDecl.vertexCount;
            if (meshDecl.faceCount > 0)
                totalFaces += meshDecl.faceCount;
            else
                totalFaces += meshDecl.indexCount / 3; // Assume triangles if MeshDecl::faceCount not specified.
        }
#endif
//        xatlas_old::AddMeshJoin(
//                atlas); // Not necessary. Only called here so geometry totals are printed after the AddMesh progress indicator.
        printf("   %u total vertices\n", totalVertices);
        printf("   %u total faces\n", totalFaces);
        // Generate atlas.
        printf("Generating atlas\n");

        // Compute charts
        {
            xatlas_old::ChartOptions options;
            options.useInputMeshUvs = true;
            xatlas_old::ComputeCharts(atlas, options);
        }
        printf("%.2f seconds (%g ms) elapsed total\n", globalT.elapsed(), globalT.elapsed() * 1000);
        // atlas is empty during the mid-phase of atlas creation

        // Pack charts
        {
			xatlas_old::PackOptions options = set_options(log_results_oss, dont_print_options);
            xatlas_old::PackCharts(atlas, options);
        }
		globalT.stop();
		double time = globalT.elapsed();

        printf("   %d charts\n", atlas->chartCount);
        printf("   %d atlases\n", atlas->atlasCount);
        for (uint32_t i = 0; i < atlas->atlasCount; i++)
            printf("      %d: %0.2f%% utilization\n", i, atlas->utilization[i] * 100.0f);
        printf("   %ux%u resolution\n", atlas->width, atlas->height);
        totalVertices = 0;
        for (uint32_t i = 0; i < atlas->meshCount; i++) {
            const xatlas_old::Mesh &mesh = atlas->meshes[i];
            totalVertices += mesh.vertexCount;
            // Input and output index counts always match.
            assert(mesh.indexCount == (uint32_t) shapes[i].mesh.indices.size());
        }
        printf("   %u total vertices\n", totalVertices);
        printf("%.2f seconds (%g ms) elapsed total\n", time, time * 1000);

		log_results_oss << atlas->chartCount << "," << time << ",";

		if (argc > 3) {
			std::ofstream log_out(argv[3], std::ios_base::app);
			log_out << log_results_oss.str();
			log_out.close();
		}

//        export_obj(atlas, shapes, output_name);
//        export_obj(atlas, shapes, output_name, true);
//        export_ply(atlas, shapes, output_name);

#if DEBUG_STAT_TO_FILE
		{
			FILE *file;
			FOPEN(file, (output_name + "_time.log"), "w");
			fprintf(file, "%.2f seconds (%g ms) elapsed total\n", time, time * 1000);
			fclose(file);
		}
#endif

        export_ply(atlas, shapes, output_name, true);

        // Cleanup.
        xatlas_old::Destroy(atlas);
        printf("Done\n");
        return EXIT_SUCCESS;
    }
} // namespace x_example