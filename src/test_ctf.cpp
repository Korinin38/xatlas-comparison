#include "x_example.cpp"


static bool ProgressCallback(xatlas::ProgressCategory category, int progress, void *userData) {
	auto t = (timer *) userData;
	static std::mutex progressMutex;
	std::unique_lock<std::mutex> lock(progressMutex);
	if (progress == 0)
		t->restart();
	FILE *f;
	FOPEN(f, x_example::log_output, "a");
	printf("\r   %s [", xatlas::StringForEnum(category));
	for (int i = 0; i < 10; i++) {
		printf(progress / ((i + 1) * 10) ? "*" : " ");
	}
	printf("] %d%% (%.2f s)", progress, t->elapsed());
	fflush(stdout);
	if (progress == 100) {
		printf("\n      %.2f seconds (%g ms) elapsed\n", t->elapsed(), t->elapsed() * 1000);
		if (x_example::out_to_file) fprintf(f, "%.2f\n", t->elapsed());
	}
	fclose(f);
	return true;
}

int main(int argc, char *argv[]) {
	x_example::out_to_file = true;
	if (argc < 3) {
		printf("Usage: %s input_file output_files_name\n", argv[0]);
		printf("Note: 2nd argument does not contain file extension.\n");
		return 1;
	}
	std::string input_name = argv[1];
	std::string output_name = argv[2];
	if (x_example::out_to_file) {
		FILE *f;
		FOPEN(f, x_example::log_output, "w");
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
	xatlas::SetPrint(x_example::Print, x_example::s_verbose);
	xatlas::Atlas *atlas = xatlas::Create();
	// Add meshes to atlas.
	uint32_t totalVertices = 0, totalFaces = 0;

	for (int i = 0; i < (int) shapes.size(); i++) {
		const tinyobj::mesh_t &objMesh = shapes[i].mesh;
		xatlas::UvMeshDecl uvMeshDecl;
		uvMeshDecl.faceMaterialData = (const uint32_t *) objMesh.material_ids.data();
		uvMeshDecl.vertexCount = (uint32_t) objMesh.texcoords.size() / 2;
		uvMeshDecl.vertexUvData = objMesh.texcoords.data();
		uvMeshDecl.vertexStride = sizeof(float) * 2;
		uvMeshDecl.indexCount = (uint32_t) objMesh.indices.size();
		uvMeshDecl.indexData = objMesh.indices.data();
		uvMeshDecl.indexFormat = xatlas::IndexFormat::UInt32;

		xatlas::AddMeshError error = xatlas::AddUvMesh(atlas, uvMeshDecl);
		if (error != xatlas::AddMeshError::Success) {
			xatlas::Destroy(atlas);
			printf("\rError adding mesh %d '%s': %s\n", i, shapes[i].name.c_str(), xatlas::StringForEnum(error));
			return EXIT_FAILURE;
		}
		totalVertices += uvMeshDecl.vertexCount;
		totalFaces += uvMeshDecl.indexCount / 3;
	}

	printf("   %u total vertices\n", totalVertices);
	printf("   %u total faces\n", totalFaces);
	// Generate atlas.
	printf("Generating atlas\n");

	// Compute charts
	xatlas::ChartOptions computeOptions;
	computeOptions.useInputMeshUvs = true;
	xatlas::ComputeCharts(atlas, computeOptions);

	// Set progress callback.
	timer globalT, t;
	xatlas::SetProgressCallback(atlas, ProgressCallback, &t);
	// Pack charts, record time
	xatlas::PackOptions options = x_example::set_options();
	for (int rotate = 0; rotate < 2; ++rotate) {
		for (int levels = 4; levels >= 0; --levels) {
			for (int rate = 16; rate >= 2; --rate) {
				if (levels == 0 && rate > 4)
					continue;
				options.coarseLevels = levels;
				options.coarseLevelRate = rate;
				options.rotateCharts = rotate;
				std::cout << "Coarse levels: " << levels << "\n"
						  << "Coarse rate:   " << rate << "\n"
						  << "Rotate charts: " << (rotate ? "true" : "false") << std::endl;
				if (x_example::out_to_file) {
					FILE *f;
					FOPEN(f, x_example::log_output, "a");
					fprintf(f, "Coarse levels: %d\nCoarse rate:   %d\nRotate charts: %s\n", levels, rate, (rotate ? "true" : "false"));
					fclose(f);
				}

				xatlas::PackCharts(atlas, options);
			}
		}
	}

	printf("%.2f seconds (%g ms) elapsed total\n", globalT.elapsed(), globalT.elapsed() * 1000);

	// Cleanup.
	xatlas::Destroy(atlas);
	printf("Done\n");
	return EXIT_SUCCESS;
}