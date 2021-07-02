#version 400 core

layout(location=0) in vec3 a_position;
layout(location=1) in vec2 a_texture_coords;
layout(location=2) in vec3 a_normal;

uniform mat4 u_transformation_matrix;
uniform mat4 u_light_space_matrix;

void main(void)
{
    gl_Position = u_light_space_matrix * u_transformation_matrix * vec4(a_position, 1.0);
}

