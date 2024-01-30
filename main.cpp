#include <limits>
#include "model.h"
#include "our_gl.h"
#include "iostream"

const int width = 800;
const int height = 800;
Model* model = NULL;
const TGAColor red = { 0, 0, 255, 255 };
const TGAColor white = { 255, 255, 255, 255 };
const TGAColor green = { 0, 255, 0, 255 };
const TGAColor blue = { 255, 0 , 0, 255 };
const Vec3f light_dir(0, 0, -1);

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
//逐行扫描
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

Vec2i GetUVLerpCoord(Vec2i* uvCoord, Vec3f bc)
{
    int u = uvCoord[0].x * bc.x + uvCoord[1].x * bc.z + uvCoord[2].x * bc.y;
    int v = uvCoord[0].y * bc.x + uvCoord[1].y * bc.z + uvCoord[2].y * bc.y;
    return Vec2i(u,v);;
}

//三角形重心
void DrawTriangle(Vec3f* ver, TGAImage& image, Vec2i* uvCoord, float* zbuffer, float intensity, TGAImage& tex)
{
    Vec2f bundingBoxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bundingBoxMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.width() - 1, image.height() - 1);
    for (int i = 0; i < 3; i++)
    {
        bundingBoxMin.x = std::max(std::min(ver[i].x, bundingBoxMin.x), 0.f);
        bundingBoxMin.y = std::max(std::min(ver[i].y, bundingBoxMin.y), 0.f);
        bundingBoxMax.x = std::min(std::max(ver[i].x, bundingBoxMax.x), clamp.x);
        bundingBoxMax.y = std::min(std::max(ver[i].y, bundingBoxMax.y), clamp.y);
    }
    Vec3f P;
    for (P.x = bundingBoxMin.x; P.x <= bundingBoxMax.x; P.x++)
    {
        for (P.y = bundingBoxMin.y; P.y <= bundingBoxMax.y; P.y++)
        {
            Vec3f u = Vec3f(ver[2].x - ver[0].x, ver[1].x - ver[0].x, ver[0].x - P.x)
                ^ Vec3f(ver[2].y - ver[0].y, ver[1].y - ver[0].y, ver[0].y - P.y);
            if (std::abs(u.z) < 1)
            {
                continue;
            }
            P.z = 0;
            Vec3f uv = Vec3f(1.0f - u.x / u.z - u.y / u.z, u.x / u.z, u.y / u.z);
            if (uv.x < 0 || uv.y < 0 || uv.z < 0)
            {
                continue;
            }
            P.z += ver[0].z * uv.x;
            P.z += ver[1].z * uv.y;
            P.z += ver[2].z * uv.z;
            if (zbuffer[int(P.x + P.y * width)] < P.z)
            {
                zbuffer[int(P.x + P.y * width)] = P.z;
                auto uvVec = GetUVLerpCoord(uvCoord, uv);
                TGAColor color_ = tex.get(uvVec.x, uvVec.y);
                //TGAColor color_ = white;
                TGAColor color__ = { color_[0] * intensity ,color_[1] * intensity ,color_[2] * intensity ,255 };
                image.set(P.x, P.y, color__);
            }

        }
    }
}

void DrawModelWithLine(TGAImage& image, Model* model)
{
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
}

Vec3f World2Screen(Vec3f v) {
    return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}

Vec2i GetTexColor(Vec3f texCoord, TGAImage& uvTex)
{
    return Vec2i(texCoord.x * uvTex.width(), texCoord.y * uvTex.height());
    //return uvTex.get(realTexCoord.x, realTexCoord.y);
}

void DrawModelWithTriangle(TGAImage& image, TGAImage& tex, Model* model)
{
    auto zbuffer = new float[width * height];
    for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        std::vector<int> uvCoords = model->tex(i);
        Vec3f screen_coords[3];
        Vec3f tCoords[3];
        Vec2i texColors[3];
        for (int j = 0; j < 3; j++)
        {
            auto world_coords = model->vert(face[j]);
            screen_coords[j] = World2Screen(world_coords);
            texColors[j] = GetTexColor(model->texVert(uvCoords[j]), tex);
            tCoords[j] = world_coords;
        }
        Vec3f Normal = (tCoords[2] - tCoords[0]) ^ (tCoords[1] - tCoords[0]);
        Normal = Normal.normalize();
        //矢量的点积在图形学中的几何意义之一：投影
        float intensity = Normal * light_dir;
        if (intensity > 0)
        {
            //映射纹理坐标
            //TGAColor color = { texColors[0][0] * intensity , intensity * texColors[0][1], intensity * texColors[0][2], 255 };
            DrawTriangle(screen_coords, image, texColors, zbuffer, intensity, tex);
        }
    }
}

void Rasterize(Vec2i p0, Vec2i p1, TGAImage& image, TGAColor color, int ybuffer[])
{
    if (p0.x > p1.x)
    {
        std::swap(p0, p1);
    }

    for (int i = p0.x; i <= p1.x; i++)
    {
        float t = (i - p0.x) / (float)(p1.x - p0.x);
        int y = p0.y * (1. - t) + p1.y * t;
        if (ybuffer[i] < y)
        {
            ybuffer[i] = y;
            image.set(i, 0, color);
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
    TGAImage texture;
    texture.read_tga_file("C:/Project/tinyrenderer/obj/african_head/african_head_diffuse.tga");
    texture.flip_vertically();
    //DrawModelWithLine(image, model);
    DrawModelWithTriangle(image, texture, model);
    image.write_tga_file("output.tga");
    delete model;
}

int main(int argc, char** argv) {

    DrawModel(argc, argv);
    return 0;
}





