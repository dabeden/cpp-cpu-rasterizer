#include <fstream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <cmath>

struct Color {
    uint8_t r, g, b;
};

class FrameBuffer{
public:
    FrameBuffer(int width, int height)
        : width(width), height(height), 
        pixels(width * height, {0, 0, 0}),
        depthBuffer(width * height, std::numeric_limits<float>::infinity()) {}

    void setPixel(int x, int y, Color c) {
        if (x<0 || x >= width || y<0 || y >= height) return;
        pixels[y * width + x] = c;
    }


    float getDepth(int x, int y) const {
        if (x<0 || x >= width || y<0 || y >= height) return std::numeric_limits<float>::infinity();
        return depthBuffer[y * width + x];
    }

    void setDepth(int x, int y, float depth) {
        if (x<0 || x >= width || y<0 || y >= height) return;
        depthBuffer[y * width + x] = depth;
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
    std::vector<float> depthBuffer;
};



struct Point2D { float x, y;};

struct Point3D { float x, y, z;};

struct ProjectedPoint {Point2D screenPos; float z;};

float degreesToRadians(float degrees) {
    return degrees * (M_PI / 180.0f);
}

float edgeFunction(const Point2D& a, const Point2D& b, const Point2D& p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x); // returns positive/negative value based on which side of the edge p is on
}

void rasterizeTriangle(FrameBuffer& fb, Point2D a, Point2D b, Point2D c, float za, float zb, float zc, Color color){
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
                // normalize the three edge values so they sum to 1 -
                // this turns them into proper barycentric weights
                float area = w0 + w1 + w2;
                float alpha = w0 / area;
                float beta  = w1 / area;
                float gamma = w2 / area;
                //blend the three verts depth using weights to get depth 
                float pixelDepth = alpha * za + beta * zb + gamma * zc;

                if (pixelDepth < fb.getDepth(x, y)) {
                    fb.setPixel(x, y, color);
                    fb.setDepth(x, y, pixelDepth);
                }   
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

ProjectedPoint project(Point3D p, float screenWidth, float screenHeight) {
    float fov = 1.0f; // scalar
    float x2d = (p.x / p.z) * fov * screenWidth + screenWidth / 2;
    float y2d = (p.y / p.z) * fov * screenHeight + screenHeight / 2;
    return {{x2d, y2d}, p.z};
}

Point3D rotateY(Point3D p, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    Point3D result;
    result.x = p.x * cosA + p.z * sinA;
    result.z = -p.x * sinA + p.z * cosA;
    result.y = p.y; // unchanged, we're spinning around the vertical axis
    return result;
}

int main() {

    // define frame size, plot the 3 corners of the triangle, color of the triangle, rasterize the triangle and write to a ppm file
    int width, height;
    float angle = degreesToRadians(10); // 45 degrees in radians
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
        {0, 1, 3}, {1,2,3}, // front face
        {4,5,6}, {4,6,7}, // back face
        {0,1,5}, {0,5,4}, // top face
        {2,3,7}, {2,7,6}, // bottom face
        {0,3,7}, {0,7,4}, // left face
        {1,2,6}, {1,6,5}  // right face
    };

    
    
    for (const Triangle& tri: triangles){
        //rotate points before rasterizing, shape will spin on axis
        Point3D aRotated = rotateY(vertices[tri.a], angle);
        Point3D bRotated = rotateY(vertices[tri.b], angle);
        Point3D cRotated = rotateY(vertices[tri.c], angle);

        ProjectedPoint aProj = project(aRotated, width, height);
        ProjectedPoint bProj = project(bRotated , width, height);
        ProjectedPoint cProj = project(cRotated, width, height);
        rasterizeTriangle(fb, aProj.screenPos, bProj.screenPos, cProj.screenPos, aProj.z, bProj.z, cProj.z, randomColor());
    }



    fb.writePPM("triangle.ppm");
    return 0;
}