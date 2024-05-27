#include "test_bitTree.h"
#include <iostream>

using namespace internal;

#define DRAW 1
#if DRAW
#include "CImg.h"
#endif

using namespace internal;

#if DRAW
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

void draw(const BitImage2dTree &image, std::string filename) {
    int ixlen = image.width();
    int iylen = image.height();
    // image for a current layer
    cimg_library::CImg<unsigned char> export_img = cimg_library::CImg(ixlen, iylen, 1, 1);
//#pragma omp parallel for collapse(2)
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

void drawTreePart(const BitImage &image, std::string filename) {
    int ixlen = image.width() / 2;
    int iylen = image.height();
    // image for a current layer
    cimg_library::CImg<unsigned char> export_img = cimg_library::CImg(ixlen, iylen, 1, 1);
//#pragma omp parallel for collapse(2)
    for (int ix = 0; ix < ixlen; ix++) {
        for (int iy = 0; iy < iylen; iy++) {
            unsigned char red = 0;
            unsigned char green = 0;
            unsigned char blue = 0;
            int pixel_status = 0;
            pixel_status += image.get(ix * 2, iy) ? 1 : 0;

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

void drawTreeFull(const BitImage &image, std::string filename) {
    int ixlen = image.width() / 2;
    int iylen = image.height();
    // image for a current layer
    cimg_library::CImg<unsigned char> export_img = cimg_library::CImg(ixlen, iylen, 1, 1);
//#pragma omp parallel for collapse(2)
    for (int ix = 0; ix < ixlen; ix++) {
        for (int iy = 0; iy < iylen; iy++) {
            unsigned char red = 0;
            unsigned char green = 0;
            unsigned char blue = 0;
            int pixel_status = 0;
            pixel_status += image.get(ix * 2 + 1, iy) ? 1 : 0;

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
    int w = 8196, h = 8196;
    int cw = 1418, ch = 982;
    BitImage imageAtlas(w, h), imageChart(cw, ch);

    FILE *f = fopen("bitimgAtlas.log", "r");
    char tmp = false;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            fread(&tmp, sizeof(char), 1, f);
            if (tmp == '1') imageAtlas.set(x, y);
        }
        fread(&tmp, sizeof(char), 1, f);
    }
    fclose(f);

    f = fopen("bitimgChart.log", "r");
    int offset_x, offset_y;
    fscanf(f, "%d %d\n", &offset_x, &offset_y);
    tmp = false;
    for (int y = 0; y < ch; ++y) {
        for (int x = 0; x < cw; ++x) {
            fread(&tmp, sizeof(char), 1, f);
            if (tmp == '1') imageChart.set(x, y);
        }
        fread(&tmp, sizeof(char), 1, f);
    }
    fclose(f);

    draw(imageAtlas, "imgAtlas.png");
    draw(imageChart, "imgChart.png");

    BitImage2dTree tree(w, h);
    tree.fill(imageAtlas, 0, 0);
    draw(tree, "treeImgAtlas.png");
    BitImage atlasOutput;
    tree.decompressInto(&atlasOutput);
    draw(atlasOutput, "trueTreeImgAtlas.png");
    BitImageOpt a = BitImageOpt(imageAtlas);
    a.decompressInto(&atlasOutput);
    draw(atlasOutput, "optImgAtlas.png");
    drawTreePart(tree.m_data, "treePart.png");
    drawTreeFull(tree.m_data, "treeFull.png");
    draw(tree.m_data, "tree_.png");

    BitImage2dTree charttree(cw, ch);
    charttree.fill(imageChart, 0, 0);
    draw(charttree, "treeImgChart.png");
    charttree.decompressInto(&atlasOutput);
    draw(atlasOutput, "trueTreeImgChart.png");
    BitImageOpt b = BitImageOpt(imageChart);
    b.decompressInto(&atlasOutput);
    draw(atlasOutput, "optImgChart.png");
    drawTreePart(charttree.m_data, "ChartPart.png");
    drawTreeFull(charttree.m_data, "ChartFull.png");


    std::cout << (tree.canBlit(imageChart, 156, 210) ? "true" : "false") << std::endl;
    std::cout << (tree.canBlit(0, 0, 1000, 1000) ? "true" : "false") << std::endl;
    tree.fill(imageChart, 156, 210);

    draw(tree, "treeFilledImgAtlas.png");
    drawTreePart(tree.m_data, "treePartFilled.png");
    drawTreeFull(tree.m_data, "treeFullFilled.png");

    return 0;
}