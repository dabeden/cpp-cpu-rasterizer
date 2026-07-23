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

    void setScreenSizeVars(int& screenWidth, int& screenHeight){
        screenWidth = width; 
        screenHeight = height;
        return;
    }

private:
    int width, height;
    std::vector<Color> pixels;
};

struct Point2D { float x, y;};

struct Point3D { float x, y, z;};

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

Color randomColor() {
    return Color{
        (uint8_t)(rand() % 256),
        (uint8_t)(rand() % 256),
        (uint8_t)(rand() % 256)
    };
}


//project 3D point to 2D screen space using a simple perspective projection
Point2D project(Point3D p, float screenWidth, float screenHeight) {
    float fov = 1.0f; // just a scaling factor for now, tune by eye
    float x2d = (p.x / p.z) * fov * screenWidth + screenWidth / 2;
    float y2d = (p.y / p.z) * fov * screenHeight + screenHeight / 2;
    return {x2d, y2d};
}

int main() {

    // define frame size, plot the 3 corners of the triangle, color of the triangle, rasterize the triangle and write to a ppm file
    int width, height;
    FrameBuffer fb(1600,1600);
    fb.setScreenSizeVars(width, height);
  
    Color color{30, 20, 255};
    srand(time(nullptr));

    /* Flat triangle projection
    Point2D a{200, 50};
    Point2D b{50, 350};
    Point2D c{350, 350};
    rasterizeTriangle(fb, a, b, c, color);*/


    Point3D vertices[8] = {
        {-20, -20, 300}, // 0: front-top-left
        {20, -20, 300},  // 1: front-top-right
        {20, 20, 300},   // 2: front-bottom-right
        {-20, 20, 300},  // 3: front-bottom-left
        {-20, -20, 400}, // 4: back-top-left
        {20, -20, 400},  // 5: back-top-right
        {20, 20, 400},   // 6: back-bottom-right
        {-20, 20, 400}   // 7: back-bottom-left
    };

    struct Triangle {int a,b,c;};
    Triangle triangles[12]={
        {0, 1, 3}, {0, 1, 2}, // front face
        {4,5,6}, {4,6,7}, // back face
        {0,1,5}, {0,5,4}, // top face
        {2,3,7}, {2,7,6}, // bottom face
        {0,3,7}, {0,7,4}, // left face
        {1,2,6}, {1,6,5}  // right face
    };

    for (const Triangle& tri: triangles){
        Point2D a2d = project(vertices[tri.a], width, height);
        Point2D b2d = project(vertices[tri.b], width, height);
        Point2D c2d = project(vertices[tri.c], width, height);
        rasterizeTriangle(fb, a2d, b2d, c2d, randomColor());
    }



    fb.writePPM("triangle.ppm");
    return 0;
}