#include <fstream>
#include <vector>
#include <cstdint>
#include <algorithm>

struct Color {
    uint8_t r, g, b;
};

class FrameBuffer{
public:
    FrameBuffer(int width, int height)
        : width(width), height(height), pixels(width * height, {0, 0, 0}) {}

    void setPixel(int x, int y, Color c) {
        if (x<0 || x >= width || y<0 || y >= height) return;
        pixels[y * width + x] = c;
    }

    //ppm is a straight forward format, write the color values of each pixel in order. each pixel is represented by 3 values between 0-255 in (R,G,B) format.
    void writePPM(const std::string& filename) {
        std::ofstream out(filename, std::ios::binary);
        out << "P6\n" << width << " " << height << "\n255\n";
        for (const auto& p : pixels) { 
            out.put(p.r).put(p.g).put(p.b);
        }
    }

private:
    int width, height;
    std::vector<Color> pixels;
};

struct Point2D { float x, y;};

float edgeFunction(const Point2D& a, const Point2D& b, const Point2D& p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x); // returns positive/negative value based on which side of the edge p is on
}

void rasterizeTriangle(FrameBuffer& fb, Point2D a, Point2D b, Point2D c, Color color){
    //bounding box of the triangle, dont render the whole window, draw a box around the triangle using its points and only render there.
    int minX = std::min({a.x, b.x, c.x});
    int maxX = std::max({a.x, b.x, c.x});
    int minY = std::min({a.y, b.y, c.y});
    int maxY = std::max({a.y, b.y, c.y});


    //loop within bounding box borders
    for (int y = minY; y <= maxY; y++){
        for (int x = minX; x <= maxX; x++){
            Point2D p{(float)x + 0.5f, (float)y + 0.5f};

            float w0 = edgeFunction(b,c,p);
            float w1 = edgeFunction(c,a,p);
            float w2 = edgeFunction(a,b,p);

            //if all same sign, point is inside the triangle
            if((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)){
                fb.setPixel(x, y, color);
            }
        }
    }
}

int main() {

    // define frame size, plot the 3 corners of the triangle, color of the triangle, rasterize the triangle and write to a ppm file
    FrameBuffer fb(400, 400);
    Point2D a{200, 50};
    Point2D b{50, 350};
    Point2D c{350, 350};
    Color color{30, 20, 255};

    rasterizeTriangle(fb, a, b, c, color);
    fb.writePPM("triangle.ppm");
    return 0;
}