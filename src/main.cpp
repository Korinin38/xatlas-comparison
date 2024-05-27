#include <iostream>
#include "x_old_example.cpp"
#include "x_example.cpp"
#include <ply_shape/ply_shape.h>
#include <filesystem>
#include <numeric>
#include <string>
#include <fstream>
#include <cstring>

namespace fs = std::filesystem;

//#define DEFAULT_MAIN x_example
//#define DEFAULT_MAIN x_old_example

std::string get_mesh_from_texmetro_json(const fs::path &texmetro) {
    std::fstream fin(texmetro);
    std::string str;
    do {
        std::getline(fin, str);
    } while (str.find("mesh\": \"") == std::string::npos && !fin.eof());
    if (!fin.eof()) {
        auto pos = str.find("mesh\": \"") + 8;
        auto end_pos = str.rfind('"');
        return str.substr(pos, end_pos - pos);
    }
    return "";
}

void getPESimple(const std::string& file_ply, const std::string &log) {
    std::vector<ply_shape::Shape> ply_shapes = ply_shape::import_ply(file_ply + ".ply");
    double area = std::accumulate(ply_shapes.begin(), ply_shapes.end(), 0.f, [](float s, const ply_shape::Shape& sh){ return s + sh.area; });
    double bbox_x = std::accumulate(ply_shapes.begin(), ply_shapes.end(), 0.f, [](float x, const ply_shape::Shape& sh){ return std::max(x, sh.aabb.xmax); });
	double last_box_x = floor(bbox_x);
	bbox_x -= last_box_x;
	double bbox_y = std::accumulate(ply_shapes.begin(), ply_shapes.end(), 0.f, [last_box_x](float y, const ply_shape::Shape& sh){ return sh.aabb.xmin < last_box_x ? y : std::max(y, sh.aabb.ymax); });
    std::cout << file_ply <<  "\tPE: " << area << "/" << last_box_x + bbox_x * bbox_y
	<< " (" << area / (last_box_x + bbox_x * bbox_y) * 100 << "%) | " << area / (last_box_x + 1) * 100 << "%" << std::endl;

	if (!log.empty()) {
		std::ofstream log_out(log, std::ios_base::app);
		log_out << area / (last_box_x + bbox_x * bbox_y) * 100 << std::flush;
		log_out.close();
	}
#if DEBUG_STAT_TO_FILE
	std::ofstream out(file_ply + "_pe.log");
	out << "PE: " << area << "/" << last_box_x + bbox_x * bbox_y
		<< " (" << area / (last_box_x + bbox_x * bbox_y) * 100 << "%) | " << area / (last_box_x + 1) * 100 << "%" << std::endl;
#endif
}

void getPEXatlas(const std::string& output_name, const std::string &log) {
    getPESimple(output_name + (BRUTE_FORCE ? "_brute" : "_rand") + "_x_charts", log);
}

int singleFileOld(int argc, char *argv[]) {
	fs::path output = argv[2];
	if (!exists(output.parent_path())) {
		std::cout << "Error: " << output << " has invalid path." << std::endl;
		return 1;
	}
	std::string log = (argc > 3) ? argv[3] : "";

	if (x_old_example::main(argc, argv) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	getPEXatlas(argv[2], log);
	return 0;
}

int singleFileNew(int argc, char *argv[]) {
    fs::path output = argv[2];
    if (!exists(output.parent_path())) {
        std::cout << "Error: " << output << " has invalid path." << std::endl;
        return 1;
    }
	std::string log = (argc > 3) ? argv[3] : "";

    if (x_example::main(argc, argv) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	getPEXatlas(argv[2], log);

    return 0;
}
int singleFile(int argc, char *argv[]) {
	if (singleFileOld(argc, argv) == EXIT_FAILURE) return EXIT_FAILURE;
	if (singleFileNew(argc, argv) == EXIT_FAILURE) return EXIT_FAILURE;
	return 0;
}

int traverseDir(const fs::path &input_directory, const fs::path &output_directory, const std::string &log) {

    if (!is_directory(output_directory)) {
        fs::create_directory(output_directory);
    }

    std::vector<fs::directory_entry> dirs;
    for (auto &entry: fs::directory_iterator(input_directory)) {
        if (!is_directory(entry))
            continue;
        if (is_regular_file(entry.path() / "texmetro.json"))
            dirs.push_back(entry);
    }
    std::sort(dirs.begin(),
              dirs.end(),
              [](auto a, auto b) { return a.path().filename() < b.path().filename(); });
    std::cout << "Found " << dirs.size() << " directories with \"texmetro.json\" inside." << std::endl;


	for (auto method : {singleFileOld, singleFileNew}) {
		for (auto &entry: dirs) {
			if (!is_directory(entry))
				continue;
			fs::path texmetro = entry.path() / "texmetro.json";
			std::string mesh_filename = get_mesh_from_texmetro_json(texmetro);
			if (mesh_filename.empty()) {
				std::cout << "Warning: mesh not found in " << entry << "." << std::endl;
				continue;
			}

			fs::create_directory(output_directory / entry.path().filename());

			std::vector<std::string> arguments = {
					"x_uv_comp",
					(entry.path() / mesh_filename).c_str(),
					(output_directory / entry.path().filename() / "output").c_str(),
					log.c_str()
			};
			std::vector<char *> in_argv;
			for (auto& arg : arguments) {
				in_argv.push_back(arg.data());
			}

			if (method(in_argv.size(), in_argv.data()) == EXIT_FAILURE) {
				return 1;
			}
			std::cout << "----------------------------------------------------" << std::endl << std::endl;

		}
	}
    return 0;
}

int main(int argc, char *argv[]) {
#ifdef DEFAULT_MAIN
    int ret = DEFAULT_MAIN::main(argc, argv);
    if (ret == EXIT_FAILURE) {
        return 1;
    }
#endif

    if (argc < 3) {
        std::cout << "Usage:\n"
                  << "    " << argv[0] << " input_directory output_directory log_file\n"
                  << "    " << argv[0] << "input_file output_name log_file\n"
                  << "Note: input_file is a path to a valid file\n"
                  << "    and must include file extention,\n"
                  << "    while output_name gets its extentions generated automatically.\n"
                  << "Note: input_directory structure:\n"
                  << "    input_directory ┐\n"
                  << "                    ├ someDir1\n"
                  << "                    ├ someDir2\n"
                  << "                    ├ ...\n"
                  << "                    ├ someDirN ┐\n"
                  << "                               ├ texmetro.json # with \"mesh\": \"[filename].obj\"\n"
                  << "                               ├ file.obj\n"
                  << "                               ├ ...\n"
                  << std::flush;
        return 1;
    }

    fs::path input = argv[1];
    fs::path output = argv[2];
	std::string log = argc > 3 ? argv[3] : "";

	if (!log.empty() && !exists(fs::path(log))) {
		std::ofstream log_out(log);
		log_out << R"(Output name,"Experiment start time","Old?",Resolution,"BruteForce?",Parallel,"Coarse-to-Fine layers",Transposition/Rotation,Charts,Time,PE)";
		log_out.close();
	}

    if (!exists(input)) {
        std::cout << "Error: " << input << " does not exist." << std::endl;
        return 1;
    }
    if (!is_directory(input)) {
        return singleFile(argc, argv);
    } else {
        return traverseDir(input, output, log);
    }
}
