#include "language_layer.h"
#include "maths.h"
#include "memory.h"
#include "strings.h"
#include "perlin.h"
#include "os.h"
#include "opengl.h"

#include "language_layer.c"
#include "maths.c"
#include "memory.c"
#include "strings.c"
#include "perlin.c"
#include "os.c"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#include "stb_image.h"

//~

typedef struct
{
    b8 is_key_down[Key_Max];
    b8 is_mouse_button_down[3];
} KeyState;

#include "jam_game_container_helpers.c"
#include "jam_game_terrain.c"
#include "jam_game_renderer.c"
#include "jam_game_entities.c"


//~
APP_PERMANENT_LOAD
{
    os = os_;
    LoadAllOpenGLProcedures();
    
    //-NOTE(tbt): initialise renderer
    R_Init();
    R_SetEnvironment((R_Environment[]){ v3(0.7f, 0.6f, 0.8f), 0.0035f, 1.5f });
    
    //-NOTE(tbt): load models
    R_TexturedModel tree_model;
    tree_model.raw_model = R_RawModelFromGeo(S8Lit("../resources/pine.geo"));
    tree_model.texture = R_ModelTextureFromTextureIDs(R_TextureFromPNG(S8Lit("../resources/pineDiffuse.png")),
                                                      R_TextureFromPNG(S8Lit("../resources/pineReflectivity.png")),
                                                      R_TextureFromPNG(S8Lit("../resources/pineShineDamping.png")));
    
    //-NOTE(tbt): create terrain
    {
        Entity *e = PushEntity();
        e->flags |= EntityFlags_DrawModel;
        e->model.raw_model = R_RawModelFromTerrain(200.0f, 6);
        e->model.texture = R_ModelTextureFromDiffuseTexture(R_TextureFromFlatColour(v3(0.06f, 0.24f, 0.17f)));
    }
    
    //-NOTE(tbt): place trees
    for(ForCount(200))
    {
        Entity *e = PushEntity();
        
        e->flags |= EntityFlags_DrawModel;
        e->model = tree_model;
        
        f32 x = RandomF32(0.0f, 200.0f);
        f32 y = RandomF32(0.0f, 200.0f);
        
        SetEntityTransform(e, (EntityTransform)
                           {
                               v3(x, TerrainHeightFromPosition(v2(x, y)), y),
                               v3(0.0f, RandomF32(-100.0f, 100.0f), 0.0f),
                               RandomF32(0.75f, 1.25f),
                           });
    }
    
    //-NOTE(tbt): create player camera
    Entity *camera = PushEntity();
    {
        camera->flags |= EntityFlags_IsCamera;
        camera->flags |= EntityFlags_PlayerMovement;
        camera->flags |= EntityFlags_FollowTerrainHeight;
        camera->flags |= EntityFlags_LightFollow;
        SetEntityTransform(camera, Translate(100.0f, 1.0f, 100.0f));
    }
}

//~
APP_HOT_LOAD 
{
    os = os_;
}

//~
APP_HOT_UNLOAD
{
    R_CleanupGPUBuffers();
    R_CleanupShaderProgram(&r_static_model_shader);
}

//~
APP_UPDATE
{
    os->fullscreen = 1;
    os->show_cursor = 0;
    
    local_persist iv2 window_size = {0};
    if(window_size.x != os->window_size.x ||
       window_size.y != os->window_size.y)
    {
        window_size = os->window_size;
        GL_Viewport(0, 0, window_size.x, window_size.y);
    }
    
    local_persist KeyState input = {0};
    
    for(u64 event_index = 0;
        event_index < os->event_count;
        event_index += 1)
    {
        if(OS_EventType_KeyPress == os->events[event_index].type)
        {
            input.is_key_down[os->events[event_index].key] = 1;
        }
        else if(OS_EventType_KeyRelease == os->events[event_index].type)
        {
            input.is_key_down[os->events[event_index].key] = 0;
            
            if(Key_Esc == os->events[event_index].key)
            {
                os->fullscreen = !os->fullscreen;
            }
        }
        else if(OS_EventType_MousePress == os->events[event_index].type)
        {
            input.is_mouse_button_down[os->events[event_index].mouse_button] = 1;
        }
        else if(OS_EventType_MouseRelease == os->events[event_index].type)
        {
            input.is_mouse_button_down[os->events[event_index].mouse_button] = 0;
        }
    }
    
    UpdateEntities(&input);
    R_RenderScene();
    os->RefreshScreen();
}
