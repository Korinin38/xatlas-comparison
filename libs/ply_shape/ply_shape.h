#ifndef SIMPLE_ATLAS_CRAMMING_PLY_SHAPE_H
#define SIMPLE_ATLAS_CRAMMING_PLY_SHAPE_H
#include <vector>
#include <ostream>
namespace ply_shape {

    /**  Axis-Aligned Bounding Box
     */
    class AABB {
    public:
        float xmin;
        float xmax;
        float ymin;
        float ymax;

        void set(float xmin, float xmax, float ymin, float ymax);

        float xsize() const;
        float ysize() const;
        float area() const;
    };

    /** 2D-shape with precalculated options, such as:
     * perimeter,
     * area,
     * AABB-box.
     * Vertex coordinates are 2d, with coordinates relative to upper-left corner of bounding box.
     */
    class Shape {
    public:
        /**
         *
         * @param vertices uses only first two vectors (supposedly x and y)
         * @param indices indices of vertices that are used, in some order.
         */
        Shape(const std::vector<std::vector<float>> &vertices,
              const std::vector<int> &indices);

        friend std::ostream &operator<<(std::ostream &, const Shape &);

        friend bool greater_area(Shape &a, Shape &b);

        friend bool greater_bb_area(Shape &a, Shape &b);

//private:
        std::vector<std::pair<float, float>> vertices;
        double area;
        float perimeter;
        AABB aabb;
    };

    bool greater_area(Shape &a, Shape &b);

    bool greater_bb_area(Shape &a, Shape &b);


    std::vector<Shape> import_ply(const std::string &filename);

    void export_ply(const std::vector<Shape> &shapes,
                    const std::vector<std::pair<float, float>> &offsets,
                    int count,
                    std::string filename);

    void export_ply_aabb(const std::vector<Shape> &shapes,
                    const std::vector<std::pair<float, float>> &offsets,
                     int count,
                    std::string filename);
}


#endif //SIMPLE_ATLAS_CRAMMING_PLY_SHAPE_H
