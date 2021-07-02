typedef struct
{
    i32 ids[4096];
    u64 ids_count;
} R_OpenGLNameStack;

typedef struct R_ShaderProgram R_ShaderProgram;

typedef struct
{
    i32 bound_vao_id;
    i32 bound_diffuse_id;
    i32 bound_reflectivity_id;
    i32 bound_shine_damping_id;
    
    R_OpenGLNameStack vaos;
    R_OpenGLNameStack vbos;
    R_OpenGLNameStack textures;
    
    u32 shadow_map_size;
    u32 shadow_depth_map_fbo;
    u32 shadow_depth_map_texture;
    
    const R_ShaderProgram *bound_shader_program;
} R_State;
global R_State r_ctx = {0};

const global r_diffuse_texture_slot       = 0;
const global r_reflectivity_texture_slot  = 1;
const global r_shine_damping_texture_slot = 2;
const global r_shadow_map_texture_slot    = 3;

internal void
R_CleanupGPUBuffers(void)
{
    for(StackForEach(i32, vao_id, r_ctx.vaos.ids))
    {
        GL_DeleteVertexArrays(1, vao_id);
    }
    
    for(StackForEach(i32, vbo_id, r_ctx.vbos.ids))
    {
        GL_DeleteBuffers(1, vbo_id);
    }
    
    for(StackForEach(i32, texture_id, r_ctx.textures.ids))
    {
        GL_DeleteTextures(1, texture_id);
    }
}

//~NOTE(tbt): models

typedef struct
{
    i32 vao_id;
    i32 vertex_count;
} R_RawModel;


internal i32
R_CreateAndBindVAO(void)
{
    i32 vao_id;
    GL_GenVertexArrays(1, &vao_id);
    GL_BindVertexArray(vao_id);
    StackPush(r_ctx.vaos.ids, vao_id);
    r_ctx.bound_vao_id = vao_id;
    return vao_id;
}

internal void
R_StoreFloatsInAttributeList(i32 attribute_number,
                             i32 component_count,
                             f32 data[],
                             u64 data_count)
{
    i32 vbo_id;
    GL_GenBuffers(1, &vbo_id);
    StackPush(r_ctx.vbos.ids, vbo_id);
    GL_BindBuffer(GL_ARRAY_BUFFER, vbo_id);
    GL_BufferData(GL_ARRAY_BUFFER, data_count * sizeof(f32), data, GL_STATIC_DRAW);
    GL_EnableVertexAttribArray(attribute_number);
    GL_VertexAttribPointer(attribute_number, component_count, GL_FLOAT, GL_FALSE, 0, 0);
    GL_BindBuffer(GL_ARRAY_BUFFER, 0);
}

internal i32
R_LoadAndBindIndexBuffer(i32 indices[],
                         u64 indices_count)
{
    i32 ibo_id;
    GL_GenBuffers(1, &ibo_id);
    StackPush(r_ctx.vbos.ids, ibo_id);
    GL_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
    GL_BufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(i32), indices, GL_STATIC_DRAW);
    return ibo_id;
}

internal R_RawModel
R_LoadToVAO(f32 positions[],
            u64 position_count,
            f32 normals[],
            u64 normals_count,
            f32 texture_coords[],
            u64 texture_coords_count,
            i32 indices[],
            u64 indices_count)
{
    R_RawModel result = {0};
    result.vao_id = R_CreateAndBindVAO();
    result.vertex_count = indices_count;
    R_LoadAndBindIndexBuffer(indices, indices_count);
    R_StoreFloatsInAttributeList(0, 3, positions, position_count);
    R_StoreFloatsInAttributeList(1, 2, texture_coords, texture_coords_count);
    R_StoreFloatsInAttributeList(2, 3, normals, normals_count);
    return result;
}

internal R_RawModel
R_RawModelFromGeo(String8 path)
{
    R_RawModel result = {0};
    
    String8 file;
    os->LoadEntireFile(&os->frame_arena, path, &file.data, &file.size);
    
    u8 *read_ptr = file.data;
    
    u16 vertex_count = ((u16 *)file.data)[0];
    u16 indices_count = ((u16 *)file.data)[1];
    read_ptr += sizeof(u16) * 2;
    
    u64 positions_size = vertex_count * 3 * sizeof(f32);
    u64 normals_size = vertex_count * 3 * sizeof(f32);
    u64 texture_coords_size = vertex_count * 2 * sizeof(f32);
    
    f32 *positions = M_ArenaPush(&os->frame_arena, positions_size);
    f32 *normals = M_ArenaPush(&os->frame_arena, normals_size);
    f32 *texture_coords = M_ArenaPush(&os->frame_arena, texture_coords_size);
    i32 *indices = M_ArenaPush(&os->frame_arena, indices_count * sizeof(i32));
    
    f32 *positions_from_file = (f32 *)read_ptr;
    for(ForInRange(i, 0, vertex_count))
    {
        positions[i * 3 + 0] = positions_from_file[i * 3 + 0];
        positions[i * 3 + 1] = positions_from_file[i * 3 + 1];
        positions[i * 3 + 2] = positions_from_file[i * 3 + 2];
    }
    read_ptr += positions_size;
    
    f32 *normals_from_file = (f32 *)read_ptr;
    for(ForInRange(i, 0, vertex_count))
    {
        normals[i * 3 + 0] = normals_from_file[i * 3 + 0];
        normals[i * 3 + 1] = normals_from_file[i * 3 + 1];
        normals[i * 3 + 2] = normals_from_file[i * 3 + 2];
    }
    read_ptr += normals_size;
    
    f32 *texture_coords_from_file = (f32 *)read_ptr;
    for(ForInRange(i, 0, vertex_count))
    {
        texture_coords[i * 2 + 0] = texture_coords_from_file[i * 2 + 0];
        texture_coords[i * 2 + 1] = texture_coords_from_file[i * 2 + 1];
    }
    read_ptr += texture_coords_size;
    
    u16 *indices_from_file = (u16 *)read_ptr;
    for(ForInRange(i, 0, indices_count))
    {
        indices[i] = indices_from_file[i];
    }
    
    result = R_LoadToVAO(positions, vertex_count * 3,
                         normals, vertex_count * 3,
                         texture_coords, vertex_count * 2,
                         indices, indices_count);
    
    return result;
}

internal v3
R_SurfaceNormalFromTriangle(v3 v_0,
                            v3 v_1,
                            v3 v_2)
{
    v3 result;
    
    v3 u = V3MinusV3(v_1, v_0);
    v3 v = V3MinusV3(v_2, v_0);
    
    result.x = (u.y * v.z) - (u.z * v.y);
    result.y = (u.z * v.x) - (u.x * v.z);
    result.z = (u.x * v.y) - (u.y * v.x);
    
    return result;
}

internal R_RawModel
R_RawModelFromTerrain(f32 size,
                      i32 vertices_per_edge)
{
    int vertices_count = vertices_per_edge * vertices_per_edge;
    int indices_count = 6 * (vertices_per_edge - 1) * (vertices_per_edge - 1);
    
    v3 *positions = M_ArenaPush(&os->frame_arena, vertices_count * sizeof(v3));
    v3 *normals = M_ArenaPush(&os->frame_arena, vertices_count * 3 * sizeof(v3));
    v2 *texture_coords = M_ArenaPush(&os->frame_arena, vertices_count * sizeof(v2));
    i32 *indices = M_ArenaPush(&os->frame_arena, indices_count * sizeof(i32));
    
    
    i32 vertex_index = 0;
    
    for(ForInRange(i, 0, vertices_per_edge))
    {
        for(ForInRange(j, 0, vertices_per_edge))
        {
            {
                positions[vertex_index].x = (f32)j / ((f32)vertices_per_edge - 1) * size;
                positions[vertex_index].z = (f32)i / ((f32)vertices_per_edge - 1) * size;
                positions[vertex_index].y = TerrainHeightFromPosition(v2(positions[vertex_index].x,
                                                                         positions[vertex_index].z));
            }
            
            {
                f32 offset = 1.0f;
                
                f32 height_l = TerrainHeightFromPosition(v2(positions[vertex_index].x - offset,
                                                            positions[vertex_index].z));
                f32 height_r = TerrainHeightFromPosition(v2(positions[vertex_index].x + offset,
                                                            positions[vertex_index].z));
                f32 height_d = TerrainHeightFromPosition(v2(positions[vertex_index].x,
                                                            positions[vertex_index].z - offset));
                f32 height_u = TerrainHeightFromPosition(v2(positions[vertex_index].x,
                                                            positions[vertex_index].z + offset));
                
                normals[vertex_index] = v3(height_l - height_r, 2.0f, height_d - height_u);
                normals[vertex_index] = V3Normalize(normals[vertex_index]);
            }
            
            {
                texture_coords[vertex_index].x = (f32)j / ((f32)vertices_per_edge - 1);
                texture_coords[vertex_index].y = (f32)i / ((f32)vertices_per_edge - 1);
            }
            
            vertex_index++;
        }
    }
    
    int index_index = 0;
    for(ForInRange(gz, 0, vertices_per_edge - 1))
    {
        for(ForInRange(gx, 0, vertices_per_edge - 1))
        {
            int tl = (gz * vertices_per_edge) + gx;
            int tr = tl + 1;
            int bl = ((gz + 1) * vertices_per_edge) + gx;
            int br = bl + 1;
            indices[index_index++] = tl;
            indices[index_index++] = bl;
            indices[index_index++] = tr;
            
            indices[index_index++] = tr;
            indices[index_index++] = bl;
            indices[index_index++] = br;
        }
    }
    
    return R_LoadToVAO((f32 *)positions, 3 * vertices_count,
                       (f32 *)normals, 3 * vertices_count,
                       (f32 *)texture_coords, 2 * vertices_count,
                       indices, indices_count);
}

//~NOTE(tbt): textures

typedef struct
{
    i32 diffuse_id;
    i32 reflectivity_id;
    i32 shine_damping_id;
} R_ModelTexture;

internal i32
R_TextureFromFlatColour(v3 colour)
{
    i32 result = 0;
    
    GL_GenTextures(1, &result);
    GL_ActiveTexture(GL_TEXTURE0);
    GL_BindTexture(GL_TEXTURE_2D, result);
    r_ctx.bound_diffuse_id = result;
    
    GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    u32 data = (255 << 24);
    data |= (u32)(255 * colour.r) <<  0;
    data |= (u32)(255 * colour.g) <<  8;
    data |= (u32)(255 * colour.b) << 16;
    
    GL_TexImage2D(GL_TEXTURE_2D,
                  0,
                  GL_RGBA8,
                  1,
                  1,
                  0,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  &data);
    
    StackPush(r_ctx.textures.ids, result);
    return result;
}

internal i32
R_TextureFromPNG(String8 filename)
{
    i32 result = 0;
    
    void *file;
    u64 file_size;
    os->LoadEntireFile(&os->frame_arena, filename, &file, &file_size);
    
    i32 width, height, channels;
    u8 *pixels = stbi_load_from_memory(file, file_size, &width, &height, &channels, 4);
    
    if (pixels)
    {
        GL_GenTextures(1, &result);
        GL_ActiveTexture(GL_TEXTURE0);
        GL_BindTexture(GL_TEXTURE_2D, result);
        r_ctx.bound_diffuse_id = result;
        
        GL_TexImage2D(GL_TEXTURE_2D,
                      0,
                      GL_RGBA8,
                      width,
                      height,
                      0,
                      GL_RGBA,
                      GL_UNSIGNED_BYTE,
                      pixels);
        
        GL_GenerateMipmap(GL_TEXTURE_2D);
        GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        
        StackPush(r_ctx.textures.ids, result);
    }
    else
    {
        LogError("error loading texture: '%.*s'\n", StringExpand(filename));
    }
    
    return result;
}

internal R_ModelTexture
R_ModelTextureFromDiffuseTexture(i32 diffuse_id)
{
    local_persist i32 default_reflectivity_map = 0;
    local_persist i32 default_shine_damping_map = 0;
    if(0 == default_reflectivity_map ||
       0 == default_shine_damping_map)
    {
        default_reflectivity_map = R_TextureFromFlatColour(v3(0.0f, 0.0f, 0.0f));
        default_shine_damping_map = R_TextureFromFlatColour(v3(1.0f, 1.0f, 1.0f));
    }
    
    R_ModelTexture result;
    result.diffuse_id = diffuse_id;
    result.reflectivity_id = default_reflectivity_map;
    result.shine_damping_id = default_shine_damping_map;
    return result;
}

internal R_ModelTexture
R_ModelTextureFromTextureIDs(i32 diffuse_id,
                             i32 reflectivity_id,
                             i32 shine_damping_id)
{
    R_ModelTexture result;
    result.diffuse_id = diffuse_id;
    result.reflectivity_id = reflectivity_id;
    result.shine_damping_id = shine_damping_id;
    return result;
}

typedef struct
{
    R_RawModel raw_model;
    R_ModelTexture texture;
} R_TexturedModel;

//~NOTE(tbt): shaders

typedef struct
{
    String8 name;
    i32 type;
    i32 location;
} R_Uniform;

struct R_ShaderProgram
{
    i32 program_id;
    i32 vert_id;
    i32 frag_id;
    
    const R_Uniform *uniforms;
    i32 uniforms_count;
};


global R_ShaderProgram r_static_model_shader;
global R_ShaderProgram r_shadow_shader;


internal i32
R_CompileSingleShader(const char *source,
                      int type)
{
    int shader_id = GL_CreateShader(type);
    GL_ShaderSource(shader_id, 1, &source, NULL);
    GL_CompileShader(shader_id);
    
    i32 compile_status;
    GL_GetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);
    if(GL_FALSE == compile_status)
    {
        char error_messages[4096] = {0};
        GL_GetShaderInfoLog(shader_id, sizeof(error_messages), NULL, error_messages);
        LogError("Could not compile shader: %s", error_messages);
    }
    return shader_id;
}

internal void R_UploadNamedUniform(const R_ShaderProgram *shader_program, String8 name, const void *value);

internal R_ShaderProgram
R_CompileAndLinkShaderProgram(String8 vert_source_filename,
                              String8 frag_source_filename)
{
    R_ShaderProgram result;
    
    char *vert_source = os->LoadEntireFileAndNullTerminate(&os->frame_arena, vert_source_filename);
    char *frag_source = os->LoadEntireFileAndNullTerminate(&os->frame_arena, frag_source_filename);
    
    result.vert_id = R_CompileSingleShader(vert_source, GL_VERTEX_SHADER);
    result.frag_id = R_CompileSingleShader(frag_source, GL_FRAGMENT_SHADER);
    
    result.program_id = GL_CreateProgram();
    GL_AttachShader(result.program_id, result.vert_id);
    GL_AttachShader(result.program_id, result.frag_id);
    GL_LinkProgram(result.program_id);
    GL_ValidateProgram(result.program_id);
    
    i32 link_status;
    GL_GetShaderiv(result.program_id, GL_LINK_STATUS, &link_status);
    if(GL_FALSE == link_status)
    {
        char error_messages[4096] = {0};
        GL_GetProgramInfoLog(result.program_id, sizeof(error_messages), NULL, error_messages);
        Log("message while linking shader: %s", error_messages);
    }
    
    // NOTE(tbt): enumerate uniforms and cache locations
    GL_GetProgramiv(result.program_id, GL_ACTIVE_UNIFORMS, &result.uniforms_count);
    result.uniforms = M_ArenaPush(&os->permanent_arena, result.uniforms_count * sizeof(*result.uniforms));
    for(i32 uniform_index = 0;
        uniform_index < result.uniforms_count;
        uniform_index += 1)
    {
        R_Uniform *uniform = (R_Uniform *)&result.uniforms[uniform_index];
        GLsizei name_length;
        GL_GetActiveUniformsiv(result.program_id, 1, &uniform_index, GL_UNIFORM_NAME_LENGTH, &name_length);
        uniform->name.size = name_length - 1; // NOTE(tbt): don't care about null terminator
        uniform->name.str = M_ArenaPushZero(&os->permanent_arena, name_length);
        GL_GetActiveUniformName(result.program_id,
                                uniform_index,
                                name_length,
                                &name_length,
                                uniform->name.str);
        GL_GetActiveUniformsiv(result.program_id, 1, &uniform_index, GL_UNIFORM_TYPE, &uniform->type);
        uniform->location = GL_GetUniformLocation(result.program_id, uniform->name.str);
    }
    
    R_UploadNamedUniform(&result, S8Lit("u_diffuse_texture"), &r_diffuse_texture_slot);
    R_UploadNamedUniform(&result, S8Lit("u_reflectivity_texture"), &r_reflectivity_texture_slot);
    R_UploadNamedUniform(&result, S8Lit("u_shine_damping_texture"), &r_shine_damping_texture_slot);
    R_UploadNamedUniform(&result, S8Lit("u_shadow_map_texture"), &r_shadow_map_texture_slot);
    
    return result;
}

internal void
R_BindShaderProgram(const R_ShaderProgram *shader_program)
{
    if(NULL != shader_program)
    {
        GL_UseProgram(shader_program->program_id);
    }
    r_ctx.bound_shader_program = shader_program;
}

internal const R_Uniform *
R_UniformFromString(const R_ShaderProgram *shader_program,
                    String8 name)
{
    local_persist const R_Uniform dummy =
    {
        .name = S8LitDesignated("__dummy__"),
        .type = -1,
        .location = -1,
    };
    
    for(i32 uniform_index = 0;
        uniform_index < shader_program->uniforms_count;
        uniform_index += 1)
    {
        const R_Uniform *uniform = &shader_program->uniforms[uniform_index];
        if(StringMatch(name, uniform->name))
        {
            return uniform;
        }
    }
    
    LogWarning("shader does not have uniform '%.*s'", StringExpand(name));
    return &dummy;
}

internal void
R_UploadUniform(const R_ShaderProgram *shader_program,
                const R_Uniform *uniform,
                const void *value)
{
    R_BindShaderProgram(shader_program);
    
    switch(uniform->type)
    {
        case(GL_FLOAT):
        {
            GL_Uniform1f(uniform->location, *((f32 *)value));
        } break;
        
        case(GL_FLOAT_VEC2):
        {
            const v2 *v = value;
            GL_Uniform2f(uniform->location, v->x, v->y);
        } break;
        
        case(GL_FLOAT_VEC3):
        {
            const v3 *v = value;
            GL_Uniform3f(uniform->location, v->x, v->y, v->z);
        } break;
        
        case(GL_FLOAT_VEC4):
        {
            const v4 *v = value;
            GL_Uniform4f(uniform->location, v->x, v->y, v->z, v->w);
        } break;
        
        case(GL_FLOAT_MAT4):
        {
            const m4 *m = value;
            GL_UniformMatrix4fv(uniform->location, 1, GL_FALSE, &m->elements[0][0]);
        } break;
        
        case(GL_INT):
        case(GL_SAMPLER_1D):
        case(GL_SAMPLER_2D):
        case(GL_SAMPLER_3D):
        case(GL_SAMPLER_CUBE):
        {
            GL_Uniform1i(uniform->location, *((i32 *)value));
        } break;
        
        case(GL_INT_VEC2):
        {
            const i32 *v = value;
            GL_Uniform2i(uniform->location, v[0], v[1]);
        } break;
        
        case(GL_INT_VEC3):
        {
            const i32 *v = value;
            GL_Uniform3i(uniform->location, v[0], v[1], v[2]);
        } break;
        
        case(GL_INT_VEC4):
        {
            const i32 *v = value;
            GL_Uniform4i(uniform->location, v[0], v[1], v[2], v[3]);
        } break;
        
        
        default:
        {
            LogError("could not set '%.*s'", StringExpand(uniform->name));
            // TODO(tbt): add more uniform types
        } break;
    }
}

internal void
R_UploadNamedUniform(const R_ShaderProgram *shader_program,
                     String8 name,
                     const void *value)
{
    R_UploadUniform(shader_program,
                    R_UniformFromString(shader_program, name),
                    value);
}

internal void
R_CleanupShaderProgram(const R_ShaderProgram *program)
{
    R_BindShaderProgram(NULL);
    GL_DetachShader(program->program_id, program->vert_id);
    GL_DetachShader(program->program_id, program->frag_id);
    GL_DeleteShader(program->vert_id);
    GL_DeleteShader(program->frag_id);
    GL_DeleteProgram(program->program_id);
}

//~NOTE(tbt): view projection and transformation

internal m4
R_CreateTransformationMatrix(v3 translation,
                             v3 rotation,
                             f32 scale)
{
    m4 result;
    result = M4InitD(1.0f);
    result = M4MultiplyM4(result, M4TranslateV3(translation));
    result = M4MultiplyM4(result, M4RotateV3(v3(1.0f, 0.0f, 0.0f), rotation.x));
    result = M4MultiplyM4(result, M4RotateV3(v3(0.0f, 1.0f, 0.0f), rotation.y));
    result = M4MultiplyM4(result, M4RotateV3(v3(0.0f, 0.0f, 1.0f), rotation.z));
    result = M4MultiplyM4(result, M4ScaleV3(v3(scale, scale, scale)));
    return result;
}

internal void
R_SetProjectionMatrix(void)
{
    
    f32 fov = 70.0f * (3.14f / 180.0f);
    f32 aspect = (f32)os->window_size.x / (f32)os->window_size.y;
    f32 near_plane = 0.1f;
    f32 far_plane = 1000.0f;
    
    f32 x_scale = (1.0f / Tan(fov / 2.0f));
    f32 y_scale = x_scale * aspect;
    f32 frustum_length = far_plane - near_plane;
    
    m4 projection_matrix = {0};
    projection_matrix = M4InitD(1.0f);
    projection_matrix.elements[0][0] = x_scale;
    projection_matrix.elements[1][1] = y_scale;
    projection_matrix.elements[2][2] = -((far_plane + near_plane) / frustum_length);
    projection_matrix.elements[2][3] = -1.0f;
    projection_matrix.elements[3][2] = -((2.0f * near_plane * far_plane) / frustum_length);
    projection_matrix.elements[3][3] = 0.0f;
    
    String8 u_projection_matrix = S8Lit("u_projection_matrix");
    R_UploadNamedUniform(&r_static_model_shader, u_projection_matrix, &projection_matrix);
    // NOTE(tbt): if any additional shaders get added - upload projection matrix here
}


//~NOTE(tbt): lighting

typedef struct
{
    v3 position;
    v3 colour;
} R_Light;

internal void
R_SetLight(const R_Light *light)
{
    R_UploadNamedUniform(&r_static_model_shader, S8Lit("u_light_position"), &light->position);
    R_UploadNamedUniform(&r_static_model_shader, S8Lit("u_light_colour"), &light->colour);
    
    v3 scene_centre = v3(0.0f, 0.0f, 0.0f);
    v3 light_position = light->position;
    
    m4 light_space_matrix = M4MultiplyM4(M4Ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1.0f, 200.0f),
                                         M4LookAt(light_position,
                                                  scene_centre,
                                                  v3(0.0f, 1.0f, 0.0f)));
    
    String8 u_light_space_matrix = S8Lit("u_light_space_matrix");
    R_UploadNamedUniform(&r_static_model_shader, u_light_space_matrix, &light_space_matrix);
    R_UploadNamedUniform(&r_shadow_shader, u_light_space_matrix, &light_space_matrix);
}

typedef struct
{
    v3 sky_colour;
    f32 fog_density;
    f32 fog_gradient;
} R_Environment;

internal void
R_SetEnvironment(const R_Environment *environment)
{
    GL_ClearColor(environment->sky_colour.r, environment->sky_colour.g, environment->sky_colour.b, 1.0f);
    R_UploadNamedUniform(&r_static_model_shader, S8Lit("u_sky_colour"), &environment->sky_colour);
    R_UploadNamedUniform(&r_static_model_shader, S8Lit("u_fog_density"), &environment->fog_density);
    R_UploadNamedUniform(&r_static_model_shader, S8Lit("u_fog_gradient"), &environment->fog_gradient);
}

//~NOTE(tbt): actual rendering

internal void
R_Init(void)
{
    //-NOTE(tbt): compile shaders
    r_static_model_shader = R_CompileAndLinkShaderProgram(S8Lit("../resources/vert_static_model.glsl"),
                                                          S8Lit("../resources/frag_static_model.glsl"));
    r_shadow_shader = R_CompileAndLinkShaderProgram(S8Lit("../resources/vert_shadows.glsl"),
                                                    S8Lit("../resources/frag_shadows.glsl"));
    
    //-NOTE(tbt): calculate and upload projection matrix
    R_SetProjectionMatrix();
    
    // NOTE(tbt): general OpenGL setup
    GL_Enable(GL_DEPTH_TEST);
    GL_Enable(GL_MULTISAMPLE);
    GL_Enable(GL_CULL_FACE);
    GL_CullFace(GL_BACK);
    
    //-NOTE(tbt): initialise render context
    r_ctx.bound_vao_id = -1;
    r_ctx.bound_diffuse_id = -1;
    r_ctx.bound_reflectivity_id = -1;
    r_ctx.bound_shine_damping_id = -1;
    
    //-NOTE(tbt): setup shadow map
    {
        r_ctx.shadow_map_size = 4096;
        
        GL_GenFramebuffers(1, &r_ctx.shadow_depth_map_fbo);
        GL_GenTextures(1, &r_ctx.shadow_depth_map_texture);
        GL_BindTexture(GL_TEXTURE_2D, r_ctx.shadow_depth_map_texture);
        GL_TexImage2D(GL_TEXTURE_2D,
                      0, GL_DEPTH_COMPONENT,
                      r_ctx.shadow_map_size, r_ctx.shadow_map_size,
                      0, GL_DEPTH_COMPONENT,
                      GL_FLOAT, NULL);
        GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        GL_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        GL_TexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (f32[]){ 1.0f, 1.0f, 1.0f, 1.0f });
        
        GL_BindFramebuffer(GL_FRAMEBUFFER, r_ctx.shadow_depth_map_fbo);
        GL_FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r_ctx.shadow_depth_map_texture, 0);
        GL_DrawBuffer(GL_NONE);
        GL_ReadBuffer(GL_NONE);
        GL_BindFramebuffer(GL_FRAMEBUFFER, 0);  
    }
}

typedef struct
{
    R_TexturedModel model;
    m4 transformation_matrix;
    const R_ShaderProgram *shader_program;
} R_Cmd;

global R_Cmd r_command_queue[4096] = {0};
global u64 r_command_queue_count = 0;

internal void
R_CmdPush(const R_TexturedModel *model,
          const m4 *transformation_matrix)
{
    R_Cmd cmd;
    //__debugbreak();
    MemoryCopy(&cmd.model, model, sizeof(*model));
    MemoryCopy(&cmd.transformation_matrix, transformation_matrix, sizeof(*transformation_matrix));
    StackPush(r_command_queue, cmd);
}

internal void
R_ExecuteCmdQueue(const R_ShaderProgram *shader_program)
{
    // TODO(tbt): sort by vao and texture ids to reduce unnecessary binds
    
    R_BindShaderProgram(shader_program);
    
    for(StackForEach(R_Cmd, cmd, r_command_queue))
    {
        for(i32 uniform_index = 0;
            uniform_index < shader_program->uniforms_count;
            uniform_index += 1)
        {
            const R_Uniform *uniform = &shader_program->uniforms[uniform_index];
            if(StringMatch(S8Lit("u_transformation_matrix"), uniform->name))
            {
                R_UploadUniform(shader_program, uniform, &cmd->transformation_matrix);
            }
        }
        
        if(1 || r_ctx.bound_vao_id != cmd->model.raw_model.vao_id)
        {
            GL_BindVertexArray(cmd->model.raw_model.vao_id);
            r_ctx.bound_vao_id = cmd->model.raw_model.vao_id;
        }
        
        if(1 || r_ctx.bound_diffuse_id != cmd->model.texture.diffuse_id)
        {
            GL_ActiveTexture(GL_TEXTURE0 + r_diffuse_texture_slot);
            GL_BindTexture(GL_TEXTURE_2D, cmd->model.texture.diffuse_id);
            r_ctx.bound_diffuse_id = cmd->model.texture.diffuse_id;
        }
        
        if(1 || r_ctx.bound_reflectivity_id != cmd->model.texture.reflectivity_id)
        {
            GL_ActiveTexture(GL_TEXTURE0 + r_reflectivity_texture_slot);
            GL_BindTexture(GL_TEXTURE_2D, cmd->model.texture.reflectivity_id);
            r_ctx.bound_reflectivity_id = cmd->model.texture.reflectivity_id;
        }
        
        if(1 || r_ctx.bound_shine_damping_id != cmd->model.texture.shine_damping_id)
        {
            GL_ActiveTexture(GL_TEXTURE0 + r_shine_damping_texture_slot);
            GL_BindTexture(GL_TEXTURE_2D, cmd->model.texture.shine_damping_id);
            r_ctx.bound_shine_damping_id = cmd->model.texture.shine_damping_id;
        }
        
        GL_DrawElements(GL_TRIANGLES, cmd->model.raw_model.vertex_count, GL_UNSIGNED_INT, 0);
    }
}

internal void
R_RenderScene(void)
{
    // NOTE(tbt): render shadow map
    {
        GL_BindFramebuffer(GL_FRAMEBUFFER, r_ctx.shadow_depth_map_fbo);
        GL_Viewport(0, 0, r_ctx.shadow_map_size, r_ctx.shadow_map_size);
        GL_Clear(GL_DEPTH_BUFFER_BIT);
        R_ExecuteCmdQueue(&r_shadow_shader);
    }
    
    // NOTE(tbt): render scene
    {
        GL_BindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_Viewport(0, 0, os->window_size.x, os->window_size.y);
        GL_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GL_ActiveTexture(GL_TEXTURE0 + r_shadow_map_texture_slot);
        GL_BindTexture(GL_TEXTURE_2D, r_ctx.shadow_depth_map_texture);
        R_ExecuteCmdQueue(&r_static_model_shader);
    }
    
    r_command_queue_count = 0;
}