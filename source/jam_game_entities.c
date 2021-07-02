typedef u64 EntityFlags;
enum EntityFlags
{
    EntityFlags_DrawModel           = (1 << 0),
    EntityFlags_IsCamera            = (1 << 1),
    EntityFlags_PlayerMovement      = (1 << 2),
    EntityFlags_Pulsate             = (1 << 3),
    EntityFlags_FollowTerrainHeight = (1 << 4),
    EntityFlags_LightFollow         = (1 << 5),
};

typedef struct Entity Entity;
struct Entity
{
    Entity *next;
    EntityFlags flags;
    R_TexturedModel model;
    
    f32 pulse_speed;
    f32 pulse_amount;
    
    v3 velocity;
    v3 rotational_velocity;
};

typedef struct
{
    v3 position;
    v3 rotation;
    f32 scale;
} EntityTransform;
#define Translate(...) ((EntityTransform){ v3(__VA_ARGS__)})
#define Rotate(...) ((EntityTransform){ {0}, v3(__VA_ARGS__)})
#define Scale(s) ((EntityTransform){ {0}, {0}, s })

typedef struct
{
    Entity _;
    EntityTransform transform;
    m4 transformation_matrix; 
} _Entity;

internal Entity *entity_list;


internal Entity *
PushEntity(void)
{
    _Entity *_e = M_ArenaPushZero(&os->permanent_arena, sizeof(*_e));
    _e->transform.scale = 1.0f;
    _e->transformation_matrix = M4InitD(1.0f);
    LLPush(entity_list, (Entity *)_e);
    
    return (Entity *)_e;
}

internal void
_RecalculateEntityTransformationMatrix(_Entity *_e)
{
    v3 position = _e->transform.position;
    v3 rotation = _e->transform.rotation;
    f32 scale = _e->transform.scale;
    
    if(_e->_.flags & EntityFlags_FollowTerrainHeight)
    {
        position.y += TerrainHeightFromPosition(v2(position.x, position.z));
    }
    
    _e->transformation_matrix = R_CreateTransformationMatrix(position, rotation, scale);
    if(_e->_.flags & EntityFlags_IsCamera)
    {
        m4 view_matrix = M4InitD(1.0f);
        view_matrix = M4MultiplyM4(view_matrix, M4RotateV3(v3(1.0f, 0.0f, 0.0f), -rotation.x));
        view_matrix = M4MultiplyM4(view_matrix, M4RotateV3(v3(0.0f, 1.0f, 0.0f), -rotation.y));
        view_matrix = M4MultiplyM4(view_matrix, M4RotateV3(v3(0.0f, 0.0f, 1.0f), -rotation.z));
        view_matrix = M4MultiplyM4(view_matrix, M4TranslateV3(v3(-position.x, -position.y, -position.z)));
        
        String8 u_view_matrix = S8Lit("u_view_matrix");
        R_UploadNamedUniform(&r_static_model_shader, u_view_matrix, &view_matrix);
        // NOTE(tbt): if any additional shaders get added - upload view matrix here
    }
}

internal v3
EntityPosition(Entity *e)
{
    _Entity *_e = (_Entity *)e;
    return _e->transform.position;
}

internal v3
EntityRotation(Entity *e)
{
    _Entity *_e = (_Entity *)e;
    return _e->transform.rotation;
}

internal f32
EntityScale(Entity *e)
{
    _Entity *_e = (_Entity *)e;
    return _e->transform.scale;
}

internal void
SetEntityTransform(Entity *e,
                   EntityTransform transform)
{
    _Entity *_e = (_Entity *)e;
    memcpy(&_e->transform, &transform, sizeof(transform));
    _RecalculateEntityTransformationMatrix(_e);
}

internal EntityTransform
TransformEntity(Entity *e,
                EntityTransform transform)
{
    _Entity *_e = (_Entity *)e;
    _e->transform.position = V3AddV3(_e->transform.position, transform.position);
    _e->transform.rotation = V3AddV3(_e->transform.rotation, transform.rotation);
    _e->transform.scale += transform.scale;
    _RecalculateEntityTransformationMatrix(_e);
    return _e->transform;
}

internal void
UpdateEntities(KeyState *input)
{
    for(LLForEach(Entity, e, entity_list))
    {
        _Entity *_e = (_Entity *)e;
        
        if(e->flags & EntityFlags_PlayerMovement)
        {
            f32 walk_speed = 0.3f;
            f32 turn_speed = 0.01f;
            
            f32 dx = 0.0f;
            f32 dy = 0.0f;
            f32 dz = 0.0f;
            
            {
                f32 distance = (input->is_key_down[Key_W] * -walk_speed +
                                input->is_key_down[Key_S] * +walk_speed);
                dx += distance * Sin(_e->transform.rotation.y);
                dz += distance * Cos(_e->transform.rotation.y);
            }
            
            {
                dy += ((input->is_key_down[Key_Space] * walk_speed) *
                       (input->is_key_down[Key_Shift] ? -1.0f : +1.0f));
            }
            
            {
                f32 distance = (input->is_key_down[Key_A] * -walk_speed +
                                input->is_key_down[Key_D] * +walk_speed);
                
                f32 half_pi = 0.5f * PI;
                dx += distance * Sin(_e->transform.rotation.y + half_pi);
                dz += distance * Cos(_e->transform.rotation.y + half_pi);
            }
            
            {
                e->rotational_velocity.y = os->mouse_delta.x * -turn_speed;
                e->rotational_velocity.x = os->mouse_delta.y * -turn_speed;
                _e->transform.rotation.x = F32Clamp(_e->transform.rotation.x, -1.5f, 1.5f);
            }
            
            e->velocity.x = dx;
            e->velocity.y = dy;
            e->velocity.z = dz;
        }
        
        if(e->flags & EntityFlags_LightFollow)
        {
            R_Light sun =
            {
                .position = v3(_e->transform.position.x, _e->transform.position.y, _e->transform.position.z),
                .colour = v3(0.8f, 0.6f, 0.6f),
            };
            R_SetLight(&sun);
            
        }
        
        if(e->flags & EntityFlags_Pulsate)
        {
            TransformEntity(e, Scale(e->pulse_amount * Sin(os->GetTime() * e->pulse_speed)));
        }
        
        TransformEntity(e, (EntityTransform){ e->velocity, e->rotational_velocity });
        
        if(e->flags & EntityFlags_DrawModel)
        {
            R_CmdPush(&e->model, &_e->transformation_matrix);
        }
    }
}
