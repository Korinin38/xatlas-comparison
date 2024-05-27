//
// Created by agisoft on 13.07.23.
//
#include "test_compressor.h"
#include <iostream>
#define DRAW 1

#if DRAW
#include "CImg.h"
#endif

using namespace internal;

#if DRAW
void load(BitImage &image, std::string filename) {
	cimg_library::CImg<unsigned char> img(("trash_images/" + filename).c_str());
	image.resize(img.width(), img.height(), true);
	for (int y = 0; y < img.height(); ++y) {
		for (int x = 0; x < img.width(); ++x) {
			if (img(x, y) > 128)
				image.set(x, y);
		}
	}
}

void draw(const BitImage &image, std::string filename) {
    int ixlen = image.width();
    int iylen = image.height();
    // image for a current layer
    cimg_library::CImg<unsigned char> export_img = cimg_library::CImg(ixlen, iylen, 1, 1);
#pragma omp parallel for collapse(2)
    for (int ix = 0; ix < ixlen; ix++) {
        for (int iy = 0; iy < iylen; iy++) {
            unsigned char red = 0;
            unsigned char green = 0;
            unsigned char blue = 0;
            int pixel_status = 0;
            pixel_status += image.get(ix, iy) ? 1 : 0;

            switch (pixel_status) {
                case 0:
                    break;
                case 1:
                    red = 255;
                    break;
                default:
                    green = blue = 255;
            }
            export_img(ix, iy, 0) = red;
        }
    }
    export_img.save(("trash_images/" + filename).c_str());
}
#endif

int main() {
//    int w = 8196, h = 8196;
//    int cw = 1418, ch = 982;
//    BitImage imageAtlas(w, h), imageChart(cw, ch);
//
//    FILE *f = fopen("bitimgAtlas.log", "r");
//    char tmp = false;
//    for (int y = 0; y < h; ++y) {
//        for (int x = 0; x < w; ++x) {
//            fread(&tmp, sizeof(char), 1, f);
//            if (tmp == '1') imageAtlas.set(x, y);
//        }
//        fread(&tmp, sizeof(char), 1, f);
//    }
//    fclose(f);
//
//    f = fopen("bitimgChart.log", "r");
//    int offset_x, offset_y;
//    fscanf(f, "%d %d\n", &offset_x, &offset_y);
////    fread(&tmp, sizeof(char), 1, f);
//    for (int y = 0; y < ch; ++y) {
//        for (int x = 0; x < cw; ++x) {
//            fread(&tmp, sizeof(char), 1, f);
//            if (tmp == '1') imageChart.set(x, y);
//        }
//        fread(&tmp, sizeof(char), 1, f);
//    }
//    fclose(f);
//
//#if DRAW
//    draw(imageAtlas, "imageAtlas.png");
//    draw(imageChart, "imageChart.png");
//#endif
//    BitCompressor compressorAtlas(imageAtlas);
//    BitCompressor compressorChart(imageChart);
//
//#if DRAW
//    BitImage newImageAtlas;
////    BitImage newImageChart;
////    compressorAtlas.decompressInto(&newImageAtlas);
////    compressorChart.decompressInto(&newImageChart);
////
////    draw(newImageAtlas, "newImageAtlas.png");
////    draw(newImageChart, "newImageChart.png");
//#endif
//
//    std::cout << (compressorAtlas.canBlit(compressorChart, offset_x, offset_y) ? "true" : "false") << std::endl;
//
//    compressorAtlas.insert(compressorChart, offset_x, offset_y);
//    std::cout << (compressorAtlas.canBlit(compressorChart, offset_x, offset_y) ? "true" : "false") << std::endl;
//    compressorAtlas.decompressInto(&newImageAtlas);
//    draw(newImageAtlas, "filledImageAtlas.png");


	BitImage a;
	load(a, "chart_118,6_0n(29, 1)_.png");
	BitImage b;
	load(b, "chart_overlap.png");

//	a.set(2, 2);
//	a.set(5, 5);

	draw(a, "p0.png");
	BitImage tmp[3];
	a.rotateTo(&tmp[0], &tmp[1], &tmp[2]);

	draw(tmp[0], "p1.png");
	draw(tmp[1], "p2.png");
	draw(tmp[2], "p3.png");
	draw(b, "p4.png");

	CoarsePyramid pyramid(&a, &b, 3, 2, false, true);
	for (int i = 0; i < pyramid.m_data.size(); ++i) {
		draw(pyramid.m_data[i], ("pyr" + std::to_string(i) + ".png").c_str());
	}

	BitImage &tmp1 = pyramid.get(2, 2, 1, 5);

	draw(tmp1, "get126.png");


//	a.reduceTo(&tmp, 2, 1, 0, false);
//	draw(tmp, "p1,0.png");
//	a.reduceTo(&tmp, 2, 1, 1, false);
//	draw(tmp, "p1,1.png");

//	for (int i = 0; i < 8; ++i) {
//		BitImage tmp;
//		a.reduceTo(&tmp, 8, i, 0, false);
//		draw(tmp, "v" + std::to_string(i) + ".png");
//	}
}