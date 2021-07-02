internal f32
TerrainHeightFromPosition(v2 position)
{
    f32 height = 0.0f;
    
    {
        f32 height_scale = 0.004f;
        f32 scale = 0.1f;
        height += height_scale * PerlinNoise2D(position.x * scale,
                                               position.y * scale);
    }
    
    {
        f32 height_scale = 0.160f;
        f32 scale = 0.000001f;
        height += height_scale * PerlinNoise2D(position.x * scale,
                                               position.y * scale);
    }
    
    {
        f32 height_scale = 0.008f;
        f32 scale = 0.00002f;
        height += height_scale * PerlinNoise2D(position.x * scale,
                                               position.y * scale);
    }
    
    return height;
}