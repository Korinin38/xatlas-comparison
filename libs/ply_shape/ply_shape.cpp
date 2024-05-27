#include <cmath>
#include "happly.h"
#include "ply_shape.h"

float distance(float x1, float y1, float x2, float y2) {
    float dx_sqr = (x2 - x1) * (x2 - x1);
    float dy_sqr = (y2 - y1) * (y2 - y1);
    return sqrt(dx_sqr + dy_sqr);
}

namespace ply_shape {

    void AABB::set(float xmin, float xmax, float ymin, float ymax) {
        this->xmin = xmin;
        this->xmax = xmax;
        this->ymin = ymin;
        this->ymax = ymax;
    }

    float AABB::xsize() const {
        return xmax - xmin;
    }

    float AABB::ysize() const {
        return ymax - ymin;
    }

    float AABB::area() const {
        return xsize() * ysize();
    }


    Shape::Shape(const std::vector<std::vector<float>> &vertices,
                 const std::vector<int> &indices) {
        std::vector<std::pair<float, float>> res_vertices;
        float area = 0;
        float perimeter = 0;

        float xmin = std::numeric_limits<float>::max();
        float ymin = std::numeric_limits<float>::max();
        float xmax = std::numeric_limits<float>::min();
        float ymax = std::numeric_limits<float>::min();

        for (int i = 0; i < indices.size(); ++i) {
            int j = (i + 1) % indices.size();
            int ind1 = indices[i];
            int ind2 = indices[j];

            float x1 = vertices[0][ind1];
            float x2 = vertices[0][ind2];
            float y1 = vertices[1][ind1];
            float y2 = vertices[1][ind2];

            xmin = std::min(xmin, x1);
            ymin = std::min(ymin, y1);
            xmax = std::max(xmax, x1);
            ymax = std::max(ymax, y1);

            perimeter += distance(x1, y1, x2, y2);
            area += (x1 * y2 - x2 * y1);

            res_vertices.emplace_back(x1, y1);
        }

        this->area = std::abs(area * 0.5);
        this->perimeter = perimeter;

//    this->AABB_xmin = xmin;
//    this->AABB_ymin = ymin;
//    this->AABB_xmax = xmax;
//    this->AABB_ymax = ymax;
        aabb.set(xmin, xmax, ymin, ymax);

        for (auto &v: res_vertices) {
            v.first -= xmin;
            v.second -= ymin;
        }
        this->vertices.swap(res_vertices);
    }

    std::ostream &operator<<(std::ostream &os, const Shape &sh) {
        os << "Shape with " << sh.vertices.size() << " vertices." << std::endl;
        os << "Area: " << sh.area << std::endl;
        os << "Perimeter: " << sh.perimeter << std::endl;
        os << std::endl;

        os << "Bounding box: [" << sh.aabb.xmin << " - " << sh.aabb.xmax << "] x [" << sh.aabb.ymin << " - "
           << sh.aabb.ymax
           << "]" << std::endl;

        return os;
    }

    bool greater_area(Shape &a, Shape &b) {
        return (a.area > b.area);
    }

    bool greater_bb_area(Shape &a, Shape &b) {
        return (a.aabb.area() > b.aabb.area());
    }

    std::vector<Shape> import_ply(const std::string &filename) {
        happly::PLYData plyIn(filename);
        std::vector<std::vector<float>> vertices;
        vertices.push_back(plyIn.getElement("vertex").getProperty<float>("x"));
        vertices.push_back(plyIn.getElement("vertex").getProperty<float>("y"));
        vertices.push_back(plyIn.getElement("vertex").getProperty<float>("z"));

        std::vector<std::vector<int>> faces =
                plyIn.getElement("face").getListProperty<int>("vertex_index");

        std::vector<Shape> shapes;
        shapes.reserve(faces.size());
        for (auto &face: faces) {
            shapes.emplace_back(vertices, face);
        }
        return shapes;
    }

    void export_ply(const std::vector<Shape> &shapes,
                    const std::vector<std::pair<float, float>> &offsets,
                    int count,
                    std::string filename) {
        std::vector<std::vector<float>> vertices(3);
        std::vector<std::vector<int>> faces;

        int vertex_num = 0;
        for (int i = 0; i < count; ++i) {
            std::vector<int> indices;
            for (std::pair<float, float> v: shapes[i].vertices) {
                vertices[0].push_back(v.first + offsets[i].first);
                vertices[1].push_back(v.second + offsets[i].second);
                vertices[2].push_back(0);

                indices.push_back(vertex_num);
                vertex_num++;
            }
            faces.push_back(indices);
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

    void export_ply_aabb(const std::vector<Shape> &shapes,
                         const std::vector<std::pair<float, float>> &offsets,
                         int count,
                         std::string filename) {
        std::vector<std::vector<float>> vertices(3);
        std::vector<std::vector<int>> faces;

        int vertex_num = 0;
        for (int i = 0; i < count; ++i) {
            std::vector<int> indices;

            AABB aabb = shapes[i].aabb;
            vertices[0].push_back(offsets[i].first);
            vertices[1].push_back(offsets[i].second);
            vertices[2].push_back(0);

            vertices[0].push_back(offsets[i].first + aabb.xsize());
            vertices[1].push_back(offsets[i].second);
            vertices[2].push_back(0);

            vertices[0].push_back(offsets[i].first + aabb.xsize());
            vertices[1].push_back(offsets[i].second + aabb.ysize());
            vertices[2].push_back(0);

            vertices[0].push_back(offsets[i].first);
            vertices[1].push_back(offsets[i].second + aabb.ysize());
            vertices[2].push_back(0);

            indices.push_back(vertex_num++);
            indices.push_back(vertex_num++);
            indices.push_back(vertex_num++);
            indices.push_back(vertex_num++);

            faces.push_back(indices);
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
}

