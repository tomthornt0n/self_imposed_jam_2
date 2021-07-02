#version 400 core

in vec2 v_texture_coords;
in vec3 v_surface_normal;
in vec3 v_to_light_vector;
in vec3 v_to_camera_vector;
in float v_visibility;
in vec4 v_light_space_position;

out vec4 o_colour;

uniform sampler2D u_diffuse_texture;
uniform sampler2D u_reflectivity_texture;
uniform sampler2D u_shine_damping_texture;
uniform sampler2D u_shadow_map_texture;

uniform vec3 u_light_colour;
uniform vec3 u_sky_colour;

const vec3 ambient_light = vec3(0.2, 0.25, 0.4);
const vec3 shadow_colour = vec3(0.2, 0.25, 0.4);

void main(void)
{
    vec3 unit_normal = normalize(v_surface_normal);
    vec3 unit_to_light_vector = normalize(v_to_light_vector);
    vec3 unit_to_camera_vector = normalize(v_to_camera_vector);
    
    vec3 colour = texture(u_diffuse_texture, v_texture_coords).rgb;
    
    vec3 ambient = ambient_light * colour;
    
    vec3 diffuse;
    {
        float n_dot_l = dot(unit_normal, unit_to_light_vector);
        vec3 brightness = max(vec3(n_dot_l), 0.0);
        diffuse = brightness * u_light_colour;
    }
    
    vec3 reflectivity = texture(u_reflectivity_texture, v_texture_coords).rgb;
    vec3 shine_damping = texture(u_shine_damping_texture, v_texture_coords).rgb;
    
    vec3 specular;
    {
        
        vec3 light_direction = -unit_to_light_vector;
        vec3 reflected_light_direction = reflect(light_direction, unit_normal);
        float specular_factor = max(dot(reflected_light_direction, unit_to_camera_vector), 0.0);
        
        specular = pow(vec3(specular_factor), shine_damping) * reflectivity * u_light_colour;
    }
    
    vec3 shadow;
    {
        vec3 projected_coords = (v_light_space_position.xyz / v_light_space_position.w) * 0.5 + 0.5;
        if(projected_coords.x > 1.0)
        {
            shadow = vec3(1.0);
        }
        else
        {
            float closest_depth = texture(u_shadow_map_texture, projected_coords.xy).r;
            float bias = 0.005;
            if(projected_coords.z - bias > closest_depth)
            {
                shadow = shadow_colour;
            }
            else
            {
                shadow = vec3(1.0);
            }
        }
    }
    
    o_colour = vec4(ambient + shadow * (diffuse + specular) * colour, 1.0);
    o_colour = mix(vec4(u_sky_colour, 1.0), o_colour, v_visibility);
    
    o_colour = vec4(diffuse, 1.0);
}
