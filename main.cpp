#include <limits>
#include "model.h"
#include "our_gl.h"


const int width = 800;
const int height = 800;
Model* model = NULL;
const TGAColor red = { 0, 0, 255, 255 };
const TGAColor white = { 255,255, 255, 255 };
const TGAColor green = { 0,255, 0, 255 };

void Drawline(int x1, int y1, int x2, int y2, TGAImage& image, TGAColor color)
{
    bool steep = false;
    if (std::abs(x1 - x2) < std::abs(y1 - y2))
    {
        std::swap(x1, y1);
        std::swap(x2, y2);
        steep = true;
    }
    if (x1 > x2)
    {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    int dx = x2 - x1;
    int dy = y2 - y1;
    float error = 0;
    float deError = std::abs(dy) * 2;
    for (float i = x1; i <= x2; i++)
    {
        float t = (i - x1) / (float)(x2 - x1);
        float y = t * y2 + y1 * (1 - t);
        if (steep)
        {
            image.set(y, i, color);
        }
        else
        {
            image.set(i, y, color);
        }
        error += deError;
        if (error > .5)
        {
            y += (y2 > y1 ? 1 : -1);
            error -= dx * 2;
        }
    }
}

void DrawTriangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color)
{
    if (t0.y == t1.y && t0.y == t2.y) return; // I dont care about degenerate triangles 
    // sort the vertices, t0, t1, t2 lower−to−upper (bubblesort yay!) 
    if (t0.y > t1.y) std::swap(t0, t1);
    if (t0.y > t2.y) std::swap(t0, t2);
    if (t1.y > t2.y) std::swap(t1, t2);
    int total_height = t2.y - t0.y;
    for (int i = t0.y; i <= t1.y; i++)
    {
        int segmentHeight = (t1.y - t0.y);
        float alpha = (float)(i - t0.y) / total_height;
        float beta = (float)(i - t0.y) / segmentHeight;
        Vec2i A = t0 + (t2 - t0) * alpha;
        Vec2i B = t0 + (t1 - t0) * beta;
        if (A.x > B.x)std::swap(A, B);
        for (int j = A.x; j <= B.x; j++)
        {
            image.set(j, i, color);
        }
    }
    for (int i = t1.y; i <= t2.y; i++)
    {
        int segmentHeight = (t2.y - t1.y);
        float alpha = (float)(i - t0.y) / total_height;
        float beta = (float)(i - t1.y) / segmentHeight;
        Vec2i A = t0 + (t2 - t0) * alpha;
        Vec2i B = t1 + (t2 - t1) * beta;
        if (A.x > B.x)std::swap(A, B);
        for (int j = A.x; j <= B.x; j++)
        {
            image.set(j, i, color);
        }
    }
}

void DrawModel(int argc, char** argv)
{
    if (argc == 2)
    {
        model = new Model(argv[1]);
    }
    else
    {
        model = new Model("C:/Project/tinyrenderer/obj/african_head/african_head.obj");
    }
    TGAImage image(width, height, TGAImage::RGB);
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++)
        {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j + 1) % 3]);
            int x0 = (v0.x + 1.) * width / 2.;
            int y0 = (v0.y + 1.) * height / 2.;
            int x1 = (v1.x + 1.) * width / 2.;
            int y1 = (v1.y + 1.) * height / 2.;
            Drawline(x0, y0, x1, y1, image, white);
        }
    }
    image.write_tga_file("output.tga");
    delete model;
}

int main(int argc, char** argv) {

    TGAImage image(width, height, TGAImage::RGB);
    Vec2i t0[3] = { Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80) };
    Vec2i t1[3] = { Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180) };
    Vec2i t2[3] = { Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180) };
    DrawTriangle(t0[0], t0[1], t0[2], image, red);
    DrawTriangle(t1[0], t1[1], t1[2], image, white);
    DrawTriangle(t2[0], t2[1], t2[2], image, green);
    image.write_tga_file("output.tga");
    return 0;
}





