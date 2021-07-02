#version 400 core

layout(location=0) in vec3 a_position;
layout(location=1) in vec2 a_texture_coords;
layout(location=2) in vec3 a_normal;

out vec2 v_texture_coords;
out vec3 v_surface_normal;
out vec3 v_to_light_vector;
out vec3 v_to_camera_vector;
out float v_visibility;
out vec4 v_light_space_position;

uniform mat4 u_transformation_matrix;
uniform mat4 u_projection_matrix;
uniform mat4 u_light_space_matrix;

uniform mat4 u_view_matrix;
uniform vec3 u_light_position;

uniform float u_fog_density;
uniform float u_fog_gradient;

void main(void)
{
    v_texture_coords = a_texture_coords;
    
    vec4 world_position = u_transformation_matrix * vec4(a_position, 1.0);
    vec4 position_relative_to_camera = u_view_matrix * world_position;
    gl_Position = u_projection_matrix * position_relative_to_camera;
    
    v_surface_normal = (u_transformation_matrix * vec4(a_normal, 0.0)).xyz;
    v_to_light_vector = u_light_position - world_position.xyz;
    
    vec3 camera_position = (inverse(u_view_matrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    v_to_camera_vector = camera_position - world_position.xyz;
    
    float distance = length(position_relative_to_camera.xyz);
    v_visibility = clamp(exp(-pow(distance * u_fog_density, u_fog_gradient)), 0.0, 1.0);
    
    v_light_space_position = u_light_space_matrix * world_position;
}

